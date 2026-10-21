/*****************************************************************************
 * vlc-redesign-update.c: applies a staged update of VLC Redesign
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/

/*
 * The interface downloads an update and writes the new files under
 * <app>\.update\staged (checked against their SHA-256 there). VLC can't
 * replace its own DLLs while it runs, so it starts this program (a copy,
 * from <app>\.update) and quits:
 *
 *   vlc-redesign-update.exe --pid <VLC> --app <folder> --version <x.y.z>
 *                           [--relaunch [--resume <mrl> [--time <s>]]]
 *
 * It waits for VLC to end, moves every staged file over the old one (the
 * old ones go to <app>\.update\backup), deletes the files listed in
 * removed.txt. Any failure puts every file back as it was. Then it records
 * the new version for Windows' list of apps (an installed copy), opens VLC
 * again, where it was, and refreshes the plugin cache in the background.
 * What it did is written to <app>\.update\update.log.
 */

#ifndef UNICODE
# define UNICODE
#endif
#ifndef _UNICODE
# define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define MAX_FILES 4096
#define APP_UNINSTALL_KEY \
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{8F3C2A61-5B7E-4D9A-9C1E-6A2F0B4D7E15}_is1"

static wchar_t app[MAX_PATH], work[MAX_PATH], staged[MAX_PATH], backup[MAX_PATH];
static FILE *logfile;

/* the files moved so far, to put back on failure (the new ones and the
 * removed ones) */
static wchar_t *done_rel[2 * MAX_FILES];
static int done_had_old[2 * MAX_FILES];
static int done_count;
/* more files than MAX_FILES: not a part of the update only */
static BOOL too_many;

static void logf_(const wchar_t *fmt, ...)
{
    if (!logfile)
        return;
    SYSTEMTIME t;
    GetLocalTime(&t);
    fwprintf(logfile, L"%04d-%02d-%02d %02d:%02d:%02d  ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute,
             t.wSecond);
    va_list ap;
    va_start(ap, fmt);
    vfwprintf(logfile, fmt, ap);
    va_end(ap);
    fputwc(L'\n', logfile);
    fflush(logfile);
}

static void join(wchar_t *out, const wchar_t *a, const wchar_t *b)
{
    _snwprintf(out, MAX_PATH, L"%ls\\%ls", a, b);
    out[MAX_PATH - 1] = 0;
}

/* creates the folders of a file's path */
static void make_parents(const wchar_t *path)
{
    wchar_t dir[MAX_PATH];
    wcsncpy(dir, path, MAX_PATH - 1);
    dir[MAX_PATH - 1] = 0;
    for (wchar_t *p = dir + 3; *p; ++p) {
        if (*p == L'\\') {
            *p = 0;
            CreateDirectoryW(dir, NULL);
            *p = L'\\';
        }
    }
}

/* a move that waits a little for a file held by an antivirus or an indexer */
static BOOL move_retry(const wchar_t *from, const wchar_t *to)
{
    make_parents(to);
    for (int i = 0; i < 50; ++i) {
        if (MoveFileExW(from, to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH))
            return TRUE;
        Sleep(200);
    }
    return FALSE;
}

static BOOL exists(const wchar_t *path)
{
    return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
}

static void remove_tree(const wchar_t *dir)
{
    wchar_t pattern[MAX_PATH], path[MAX_PATH];
    join(pattern, dir, L"*");
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L".."))
                continue;
            join(path, dir, fd.cFileName);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                remove_tree(path);
            else {
                SetFileAttributesW(path, FILE_ATTRIBUTE_NORMAL);
                DeleteFileW(path);
            }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    RemoveDirectoryW(dir);
}

/* every file under base\rel, as paths relative to base */
static int list_files(const wchar_t *base, const wchar_t *rel, wchar_t **out, int count)
{
    wchar_t dir[MAX_PATH], pattern[MAX_PATH];
    if (*rel)
        join(dir, base, rel);
    else
        wcsncpy(dir, base, MAX_PATH);
    join(pattern, dir, L"*");
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return count;
    do {
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L".."))
            continue;
        wchar_t sub[MAX_PATH];
        if (*rel)
            join(sub, rel, fd.cFileName);
        else
            wcsncpy(sub, fd.cFileName, MAX_PATH);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            count = list_files(base, sub, out, count);
        else if (count < MAX_FILES)
            out[count++] = _wcsdup(sub);
        else
            too_many = TRUE;
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return count;
}

/* the old file of rel goes to the backup (if there is one) */
static BOOL set_aside(const wchar_t *rel)
{
    wchar_t target[MAX_PATH], kept[MAX_PATH];
    join(target, app, rel);
    join(kept, backup, rel);
    if (done_count >= 2 * MAX_FILES)
        return FALSE;
    done_rel[done_count] = _wcsdup(rel);
    done_had_old[done_count] = exists(target);
    if (done_had_old[done_count] && !move_retry(target, kept)) {
        logf_(L"cannot move %ls aside (error %lu)", target, GetLastError());
        free(done_rel[done_count]);
        return FALSE;
    }
    ++done_count;
    return TRUE;
}

/* everything back as it was: the new files back to the staged update (so
 * the next try has all of them), the old ones in place */
static void roll_back(void)
{
    for (int i = done_count - 1; i >= 0; --i) {
        wchar_t target[MAX_PATH], kept[MAX_PATH], again[MAX_PATH];
        join(target, app, done_rel[i]);
        join(kept, backup, done_rel[i]);
        join(again, staged, done_rel[i]);
        if (exists(target) && !exists(again))
            move_retry(target, again);
        if (done_had_old[i])
            move_retry(kept, target);
    }
    logf_(L"rolled back %d files", done_count);
}

static BOOL wait_process(DWORD pid)
{
    if (!pid)
        return TRUE;
    HANDLE p = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!p)
        return TRUE; /* gone already */
    DWORD r = WaitForSingleObject(p, 60000);
    CloseHandle(p);
    return r == WAIT_OBJECT_0;
}

static void run_wait(const wchar_t *cmdline, DWORD ms, DWORD flags)
{
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    PROCESS_INFORMATION pi;
    wchar_t *cmd = _wcsdup(cmdline);
    if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW | flags, NULL, app, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, ms);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    free(cmd);
}

/* Windows' list of apps shows the new version (an installed copy only) */
static void record_version(const wchar_t *version)
{
    HKEY roots[2] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
    for (int i = 0; i < 2; ++i) {
        HKEY key;
        if (RegOpenKeyExW(roots[i], APP_UNINSTALL_KEY, 0, KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_64KEY,
                          &key) != ERROR_SUCCESS)
            continue;
        wchar_t location[MAX_PATH] = L"";
        DWORD size = sizeof location - sizeof(wchar_t);
        RegQueryValueExW(key, L"InstallLocation", NULL, NULL, (BYTE *)location, &size);
        size_t n = wcslen(location);
        if (n && location[n - 1] == L'\\')
            location[n - 1] = 0;
        if (!_wcsicmp(location, app)) {
            RegSetValueExW(key, L"DisplayVersion", 0, REG_SZ, (const BYTE *)version,
                           (DWORD)((wcslen(version) + 1) * sizeof(wchar_t)));
            logf_(L"version %ls recorded for the list of apps", version);
        }
        RegCloseKey(key);
    }
}

/* VLC's process, to close, or NULL */
static HANDLE relaunch(const wchar_t *resume, const wchar_t *time)
{
    wchar_t cmd[4 * MAX_PATH];
    /* an encoded URL and a number of seconds, nothing else on the command line */
    if (resume && wcschr(resume, L'"'))
        resume = NULL;
    if (time && wcsspn(time, L"0123456789") != wcslen(time))
        time = NULL;
    if (resume && *resume) {
        /* not :start-time, which makes the media a clip that starts there */
        if (time && *time)
            _snwprintf(cmd, 4 * MAX_PATH, L"\"%ls\\vlc.exe\" --redesign-resume-time=%ls \"%ls\"", app, time,
                       resume);
        else
            _snwprintf(cmd, 4 * MAX_PATH, L"\"%ls\\vlc.exe\" \"%ls\"", app, resume);
    } else {
        _snwprintf(cmd, 4 * MAX_PATH, L"\"%ls\\vlc.exe\"", app);
    }
    cmd[4 * MAX_PATH - 1] = 0;
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    PROCESS_INFORMATION pi;
    if (!CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, app, &si, &pi)) {
        logf_(L"cannot open VLC Redesign again (error %lu)", GetLastError());
        return NULL;
    }
    CloseHandle(pi.hThread);
    logf_(L"VLC Redesign opened again");
    return pi.hProcess;
}

/* the files the update deletes: one path relative to the app per line */
static int read_removed(wchar_t **out)
{
    wchar_t path[MAX_PATH];
    join(path, work, L"removed.txt");
    FILE *f = _wfopen(path, L"rt, ccs=UTF-8");
    if (!f)
        return 0;
    int count = 0;
    wchar_t line[MAX_PATH];
    while (fgetws(line, MAX_PATH, f)) {
        if (count >= MAX_FILES) {
            too_many = TRUE;
            break;
        }
        line[wcscspn(line, L"\r\n")] = 0;
        for (wchar_t *p = line; *p; ++p)
            if (*p == L'/')
                *p = L'\\';
        /* never outside the app, never the user's data */
        if (*line && !wcsstr(line, L"..") && line[0] != L'\\' && !wcschr(line, L':')
            && _wcsnicmp(line, L"portable", 8))
            out[count++] = _wcsdup(line);
    }
    fclose(f);
    return count;
}

int wmain(int argc, wchar_t **argv)
{
    DWORD pid = 0;
    const wchar_t *version = L"", *resume = NULL, *time = NULL;
    BOOL again = FALSE;
    for (int i = 1; i < argc; ++i) {
        if (!wcscmp(argv[i], L"--pid") && i + 1 < argc)
            pid = wcstoul(argv[++i], NULL, 10);
        else if (!wcscmp(argv[i], L"--app") && i + 1 < argc)
            wcsncpy(app, argv[++i], MAX_PATH - 1);
        else if (!wcscmp(argv[i], L"--version") && i + 1 < argc)
            version = argv[++i];
        else if (!wcscmp(argv[i], L"--relaunch"))
            again = TRUE;
        else if (!wcscmp(argv[i], L"--resume") && i + 1 < argc)
            resume = argv[++i];
        else if (!wcscmp(argv[i], L"--time") && i + 1 < argc)
            time = argv[++i];
    }
    size_t n = wcslen(app);
    if (n && app[n - 1] == L'\\')
        app[n - 1] = 0;
    if (!*app)
        return 2;
    join(work, app, L".update");
    join(staged, work, L"staged");
    join(backup, work, L"backup");

    /* one at a time: VLC may start it twice as it quits */
    wchar_t lockpath[MAX_PATH];
    join(lockpath, work, L"update.lock");
    HANDLE lock = CreateFileW(lockpath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (lock == INVALID_HANDLE_VALUE)
        return 4;

    wchar_t logpath[MAX_PATH];
    join(logpath, work, L"update.log");
    /* the log of the updates before, unless it grew large */
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (GetFileAttributesExW(logpath, GetFileExInfoStandard, &info)
        && (info.nFileSizeHigh || info.nFileSizeLow > 256 * 1024))
        DeleteFileW(logpath);
    logfile = _wfopen(logpath, L"a, ccs=UTF-8");
    logf_(L"update to %ls of %ls", version, app);

    if (!wait_process(pid)) {
        logf_(L"VLC Redesign is still running: the update waits for the next time");
        return 3;
    }

    static wchar_t *files[MAX_FILES], *removed[MAX_FILES];
    int count = list_files(staged, L"", files, 0);
    int gone = read_removed(removed);
    if (too_many) {
        logf_(L"the update has more than %d files: not applied", MAX_FILES);
        HANDLE vlc = again ? relaunch(resume, time) : NULL;
        if (vlc)
            CloseHandle(vlc);
        return 1;
    }
    if (!count && !gone) {
        logf_(L"nothing to update");
        return 0;
    }
    remove_tree(backup);
    CreateDirectoryW(backup, NULL);

    BOOL ok = TRUE;
    for (int i = 0; ok && i < count; ++i) {
        wchar_t from[MAX_PATH], to[MAX_PATH];
        join(from, staged, files[i]);
        join(to, app, files[i]);
        if (!set_aside(files[i]) || !move_retry(from, to)) {
            logf_(L"cannot put %ls in place (error %lu)", files[i], GetLastError());
            ok = FALSE;
        }
    }
    for (int i = 0; ok && i < gone; ++i) {
        wchar_t target[MAX_PATH];
        join(target, app, removed[i]);
        if (exists(target) && !set_aside(removed[i]))
            ok = FALSE;
    }
    if (!ok) {
        roll_back();
        logf_(L"the update failed: VLC Redesign is as it was");
        HANDLE vlc = again ? relaunch(resume, time) : NULL;
        if (vlc)
            CloseHandle(vlc);
        return 1;
    }
    logf_(L"%d files updated, %d removed", count, gone);

    record_version(version);
    remove_tree(backup);
    remove_tree(staged);
    wchar_t list[MAX_PATH];
    join(list, work, L"removed.txt");
    DeleteFileW(list);
    join(list, work, L"ready.txt");
    DeleteFileW(list);
    HANDLE vlc = again ? relaunch(resume, time) : NULL;

    /* VLC trusts plugins.dat only for plugins of the same time and size, and
     * loads a changed one by itself meanwhile: the cache is made again once
     * VLC has read it (its window is up), in the background. vlc-cache-gen
     * loads every plugin, seconds when they aren't in memory. */
    if (vlc) {
        WaitForInputIdle(vlc, 20000);
        CloseHandle(vlc);
    }
    wchar_t cmd[3 * MAX_PATH];
    _snwprintf(cmd, 3 * MAX_PATH, L"\"%ls\\vlc-cache-gen.exe\" \"%ls\\plugins\"", app, app);
    run_wait(cmd, 120000, BELOW_NORMAL_PRIORITY_CLASS);
    logf_(L"done");
    if (logfile)
        fclose(logfile);
    logfile = NULL;
    return 0;
}

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
#include <tlhelp32.h>
#include <io.h>
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
/* a path longer than MAX_PATH: never cut short (another file), the update
 * is not applied */
static BOOL too_long;

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

static BOOL join(wchar_t *out, const wchar_t *a, const wchar_t *b)
{
    const int n = _snwprintf(out, MAX_PATH, L"%ls\\%ls", a, b);
    out[MAX_PATH - 1] = 0;
    if (n < 0 || n >= MAX_PATH) {
        if (!too_long)
            logf_(L"a path is too long: %ls\\%ls", a, b);
        too_long = TRUE;
        return FALSE;
    }
    return TRUE;
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
    /* a junction or a link is removed itself, never what it points to (a
     * junction in place of the backup folder had its target's files deleted) */
    const DWORD attributes = GetFileAttributesW(dir);
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
        RemoveDirectoryW(dir);
        return;
    }
    if (!join(pattern, dir, L"*"))
        return;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L".."))
                continue;
            if (!join(path, dir, fd.cFileName))
                continue;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    RemoveDirectoryW(path);
                else
                    DeleteFileW(path);
            } else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
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
    if (*rel) {
        if (!join(dir, base, rel))
            return count;
    } else
        wcsncpy(dir, base, MAX_PATH);
    if (!join(pattern, dir, L"*"))
        return count;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return count;
    do {
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L".."))
            continue;
        wchar_t sub[MAX_PATH];
        if (*rel) {
            if (!join(sub, rel, fd.cFileName))
                continue;
        } else
            wcsncpy(sub, fd.cFileName, MAX_PATH);
        /* (a junction or a link in a staged update is not followed: only
         * real files of the stage are listed) */
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            continue;
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
/* The journal of a run (applying.txt): a line for each file before it is
 * moved, "1<tab>path" when an old one goes aside to the backup, "0<tab>path"
 * when the update adds it. Written to the disk at once, so a run cut off in
 * the middle (a power cut, Windows shutting down) can be undone by the next
 * one: VLC never stays half old, half new (recover_journal) */
static FILE *journal;
static const wchar_t runonce_key[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce";
static const wchar_t runonce_name[] = L"VLC Redesign update recovery";

static void journal_path(wchar_t *path)
{
    join(path, work, L"applying.txt");
}

static void journal_line(const wchar_t *rel, BOOL had_old)
{
    if (!journal)
        return;
    fwprintf(journal, L"%d\t%ls\n", had_old ? 1 : 0, rel);
    fflush(journal);
    _commit(_fileno(journal));
}

/* the journal and its RunOnce entry gone: the run ended as it should */
static void journal_end(void)
{
    if (journal) {
        fclose(journal);
        journal = NULL;
    }
    wchar_t path[MAX_PATH];
    journal_path(path);
    DeleteFileW(path);
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, runonce_key, 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
        RegDeleteValueW(key, runonce_name);
        RegCloseKey(key);
    }
}

/* A path of the journal: one of VLC's folder, as read_removed takes them */
static BOOL safe_rel(const wchar_t *rel)
{
    if (!*rel || rel[0] == L'\\' || rel[0] == L'/' || wcsstr(rel, L"..") || wcschr(rel, L':')
        || !_wcsnicmp(rel, L"portable", 8) || !_wcsnicmp(rel, L".update", 7))
        return FALSE;
    for (const wchar_t *p = rel; *p; ++p)
        if (*p == L'~')
            return FALSE;
    return TRUE;
}

/* The journal of a run cut off in the middle: every old file back from the
 * backup, every file the update added gone. VLC is then as it was before
 * the update (which VLC downloads again). False when one couldn't be. */
static BOOL recover_journal(void)
{
    wchar_t path[MAX_PATH];
    journal_path(path);
    FILE *f = _wfopen(path, L"rt, ccs=UTF-8");
    if (!f)
        return TRUE;
    wchar_t line[MAX_PATH + 8];
    BOOL ok = TRUE;
    int count = 0;
    while (fgetws(line, MAX_PATH + 8, f)) {
        line[wcscspn(line, L"\r\n")] = 0;
        if ((line[0] != L'0' && line[0] != L'1') || line[1] != L'\t' || !safe_rel(line + 2))
            continue;
        const wchar_t *rel = line + 2;
        wchar_t target[MAX_PATH], kept[MAX_PATH];
        if (!join(target, app, rel) || !join(kept, backup, rel)) {
            ok = FALSE;
            continue;
        }
        if (line[0] == L'1') {
            /* (not yet aside when the run stopped: the old one is in place) */
            if (exists(kept) && !move_retry(kept, target)) {
                logf_(L"cannot put %ls back (error %lu)", target, GetLastError());
                ok = FALSE;
            }
        } else if (exists(target)) {
            SetFileAttributesW(target, FILE_ATTRIBUTE_NORMAL);
            if (!DeleteFileW(target)) {
                logf_(L"cannot remove %ls (error %lu)", target, GetLastError());
                ok = FALSE;
            }
        }
        ++count;
    }
    fclose(f);
    logf_(ok ? L"an update cut off in the middle: %d files put back as they were"
             : L"an update cut off in the middle: %d files, not all of them put back", count);
    return ok;
}

/* The stage as VLC wrote it (manifest.txt: "size<tab>path" for each of its
 * files): every one there with its size, and no other. False when it isn't
 * (one taken away since, by an antivirus): nothing is put in place. */
static BOOL stage_whole(int listed_count)
{
    wchar_t path[MAX_PATH];
    join(path, work, L"manifest.txt");
    FILE *f = _wfopen(path, L"rt, ccs=UTF-8");
    if (!f)
        return TRUE; /* (the stage of a VLC before this list: as before) */
    wchar_t line[MAX_PATH + 32];
    int count = 0;
    BOOL ok = TRUE;
    while (ok && fgetws(line, MAX_PATH + 32, f)) {
        line[wcscspn(line, L"\r\n")] = 0;
        if (!*line)
            continue;
        wchar_t *tab = wcschr(line, L'\t');
        if (!tab) {
            ok = FALSE;
            break;
        }
        *tab = 0;
        const unsigned long long size = _wcstoui64(line, NULL, 10);
        wchar_t file[MAX_PATH];
        WIN32_FILE_ATTRIBUTE_DATA data;
        if (!safe_rel(tab + 1) || !join(file, staged, tab + 1)
            || !GetFileAttributesExW(file, GetFileExInfoStandard, &data)
            || ((((unsigned long long)data.nFileSizeHigh) << 32) | data.nFileSizeLow) != size) {
            logf_(L"the staged update lacks %ls as it was written", tab + 1);
            ok = FALSE;
        }
        ++count;
    }
    fclose(f);
    if (ok && count != listed_count) {
        logf_(L"the staged update has %d files, its list %d", listed_count, count);
        ok = FALSE;
    }
    return ok;
}

static BOOL set_aside(const wchar_t *rel)
{
    wchar_t target[MAX_PATH], kept[MAX_PATH];
    if (!join(target, app, rel) || !join(kept, backup, rel))
        return FALSE;
    if (done_count >= 2 * MAX_FILES)
        return FALSE;
    done_rel[done_count] = _wcsdup(rel);
    done_had_old[done_count] = exists(target);
    journal_line(rel, done_had_old[done_count]);
    if (done_had_old[done_count] && !move_retry(target, kept)) {
        logf_(L"cannot move %ls aside (error %lu)", target, GetLastError());
        free(done_rel[done_count]);
        return FALSE;
    }
    ++done_count;
    return TRUE;
}

/* Written when an old file couldn't be put back: it is still in the backup,
 * which the next run puts back first and never deletes before */
static void restore_mark(wchar_t *path)
{
    join(path, work, L"restore.txt");
}

/* everything back as it was: the new files back to the staged update (so
 * the next try has all of them), the old ones in place. False if an old
 * one stays in the backup. */
static BOOL roll_back(void)
{
    int failed = 0;
    for (int i = done_count - 1; i >= 0; --i) {
        wchar_t target[MAX_PATH], kept[MAX_PATH], again[MAX_PATH];
        if (!join(target, app, done_rel[i]) || !join(kept, backup, done_rel[i])
            || !join(again, staged, done_rel[i])) {
            ++failed;
            continue;
        }
        if (exists(target) && !exists(again))
            move_retry(target, again);
        if (done_had_old[i] && !move_retry(kept, target)) {
            logf_(L"cannot put %ls back (error %lu)", target, GetLastError());
            ++failed;
        }
    }
    if (failed) {
        wchar_t mark[MAX_PATH];
        restore_mark(mark);
        FILE *f = _wfopen(mark, L"w");
        if (f)
            fclose(f);
        logf_(L"rolled back %d files, %d of them are still in %ls", done_count, failed, backup);
        return FALSE;
    }
    logf_(L"rolled back %d files", done_count);
    return TRUE;
}

/* After a roll back that couldn't put everything back: every file of the
 * backup in place again. False if one still can't be. */
static BOOL restore_backup(void)
{
    static wchar_t *kept[MAX_FILES];
    const int count = list_files(backup, L"", kept, 0);
    BOOL ok = !too_many && !too_long;
    for (int i = 0; i < count; ++i) {
        wchar_t from[MAX_PATH], to[MAX_PATH];
        if (!join(from, backup, kept[i]) || !join(to, app, kept[i]) || !move_retry(from, to)) {
            logf_(L"cannot put %ls back (error %lu)", kept[i], GetLastError());
            ok = FALSE;
        }
        free(kept[i]);
    }
    logf_(ok ? L"%d files of the last update's backup put back" : L"the backup is kept: %d files", count);
    return ok;
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

/* Another VLC of this folder still runs (a second window, a copy Explorer
 * opened): the files it uses can't be replaced under it */
static BOOL folder_in_use(void)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return FALSE;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    BOOL found = FALSE;
    const size_t n = wcslen(app);
    for (BOOL more = Process32FirstW(snap, &pe); more && !found; more = Process32NextW(snap, &pe)) {
        if (pe.th32ProcessID == GetCurrentProcessId() || _wcsicmp(pe.szExeFile, L"vlc.exe"))
            continue;
        HANDLE p = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
        if (!p)
            continue;
        wchar_t image[MAX_PATH];
        DWORD length = MAX_PATH;
        if (QueryFullProcessImageNameW(p, 0, image, &length) && length > n
            && !_wcsnicmp(image, app, n) && image[n] == L'\\')
            found = TRUE;
        CloseHandle(p);
    }
    CloseHandle(snap);
    return found;
}

/* every VLC of this folder gone, waited for a minute at most */
static BOOL wait_folder(void)
{
    for (int i = 0; i < 600 && folder_in_use(); ++i)
        Sleep(100);
    return !folder_in_use();
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
    int n = -1;
    if (resume && *resume) {
        /* not :start-time, which makes the media a clip that starts there */
        if (time && *time)
            n = _snwprintf(cmd, 4 * MAX_PATH, L"\"%ls\\vlc.exe\" --redesign-resume-time=%ls \"%ls\"", app, time,
                           resume);
        else
            n = _snwprintf(cmd, 4 * MAX_PATH, L"\"%ls\\vlc.exe\" \"%ls\"", app, resume);
        /* a long URL (a deep path in another script, each letter %XX%XX)
         * would be cut and opened as another file: VLC opens without it */
        if (n < 0 || n >= 4 * MAX_PATH) {
            logf_(L"the media to resume is too long for the command line: VLC Redesign opens without it");
            n = -1;
        }
    }
    if (n < 0)
        _snwprintf(cmd, 4 * MAX_PATH, L"\"%ls\\vlc.exe\"", app);
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
        /* a line cut by the buffer: its end would be read as a path */
        if (!wcschr(line, L'\n') && !feof(f)) {
            logf_(L"a path of removed.txt is too long");
            too_long = TRUE;
            break;
        }
        line[wcscspn(line, L"\r\n")] = 0;
        for (wchar_t *p = line; *p; ++p)
            if (*p == L'/')
                *p = L'\\';
        /* never outside the app, never the user's data, never the update's
         * own files (as safeRelativePath of the interface): each part of it,
         * none empty or ".", no short 8.3 name, none ending in a dot or a
         * space; and a file, never a whole folder */
        BOOL parts_ok = *line && line[wcslen(line) - 1] != L'\\';
        for (const wchar_t *part = line; parts_ok && *part; ) {
            const wchar_t *end = wcschr(part, L'\\');
            const size_t n = end ? (size_t)(end - part) : wcslen(part);
            if (n == 0 || (n == 1 && part[0] == L'.') || wmemchr(part, L'~', n)
                || part[n - 1] == L'.' || part[n - 1] == L' ')
                parts_ok = FALSE;
            part = end ? end + 1 : part + n;
        }
        wchar_t target[MAX_PATH];
        if (parts_ok && join(target, app, line)) {
            const DWORD attributes = GetFileAttributesW(target);
            if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY))
                parts_ok = FALSE;
        }
        if (parts_ok && !wcsstr(line, L"..") && line[0] != L'\\' && !wcschr(line, L':')
            && _wcsnicmp(line, L"portable", 8) && _wcsnicmp(line, L".update", 7))
            out[count++] = _wcsdup(line);
    }
    fclose(f);
    return count;
}

int wmain(int argc, wchar_t **argv)
{
    DWORD pid = 0;
    const wchar_t *version = L"", *resume = NULL, *time = NULL;
    BOOL again = FALSE, recover_only = FALSE;
    for (int i = 1; i < argc; ++i) {
        if (!wcscmp(argv[i], L"--pid") && i + 1 < argc)
            pid = wcstoul(argv[++i], NULL, 10);
        else if (!wcscmp(argv[i], L"--recover"))
            recover_only = TRUE; /* (from RunOnce, at the next sign-in) */
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
    /* (no name in the working folder is longer than update.lock's) */
    wchar_t lockpath[MAX_PATH];
    if (!join(work, app, L".update") || !join(staged, work, L"staged") || !join(backup, work, L"backup")
        || !join(lockpath, work, L"update.lock"))
        return 2;

    /* one at a time: VLC may start it twice as it quits */
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

    if (!wait_process(pid) || !wait_folder()) {
        logf_(L"VLC Redesign is still running: the update waits for the next time");
        /* (asked to open VLC again: it does when none of this folder runs
         * any more; after "Restart now", nothing came back) */
        HANDLE vlc = again && !folder_in_use() ? relaunch(resume, time) : NULL;
        if (vlc)
            CloseHandle(vlc);
        return 3;
    }

    wchar_t list[MAX_PATH];
    /* a run cut off in the middle: VLC as it was before it, the update's
     * stage gone (VLC downloads it again) */
    wchar_t journal_file[MAX_PATH];
    journal_path(journal_file);
    if (exists(journal_file)) {
        if (recover_journal()) {
            remove_tree(backup);
            remove_tree(staged);
            join(list, work, L"removed.txt");
            DeleteFileW(list);
            join(list, work, L"ready.txt");
            DeleteFileW(list);
            join(list, work, L"manifest.txt");
            DeleteFileW(list);
            join(list, work, L"restore.txt");
            DeleteFileW(list);
            journal_end();
        }
        HANDLE vlc = again ? relaunch(resume, time) : NULL;
        if (vlc)
            CloseHandle(vlc);
        return 1;
    }
    if (recover_only)
        return 0;
    /* the last roll back left old files in the backup: they go back first.
     * The staged update may then lack a new file it couldn't take back, so
     * it goes too (VLC downloads it again). */
    wchar_t mark[MAX_PATH];
    restore_mark(mark);
    if (exists(mark)) {
        if (restore_backup()) {
            remove_tree(backup);
            remove_tree(staged);
            join(list, work, L"removed.txt");
            DeleteFileW(list);
            join(list, work, L"ready.txt");
            DeleteFileW(list);
            join(list, work, L"manifest.txt");
            DeleteFileW(list);
            DeleteFileW(mark);
            logf_(L"VLC Redesign is as it was before the last update, which is downloaded again");
        }
        HANDLE vlc = again ? relaunch(resume, time) : NULL;
        if (vlc)
            CloseHandle(vlc);
        return 1;
    }

    static wchar_t *files[MAX_FILES], *removed[MAX_FILES];
    int count = list_files(staged, L"", files, 0);
    int gone = read_removed(removed);
    if (too_many || too_long) {
        if (too_many)
            logf_(L"the update has more than %d files: not applied", MAX_FILES);
        else
            logf_(L"the update has a path that is too long: not applied");
        HANDLE vlc = again ? relaunch(resume, time) : NULL;
        if (vlc)
            CloseHandle(vlc);
        return 1;
    }
    if (!stage_whole(count)) {
        /* (VLC downloads it again) */
        logf_(L"the staged update is not whole: not applied");
        remove_tree(staged);
        join(list, work, L"removed.txt");
        DeleteFileW(list);
        join(list, work, L"manifest.txt");
        DeleteFileW(list);
        join(list, work, L"ready.txt");
        DeleteFileW(list);
        HANDLE vlc = again ? relaunch(resume, time) : NULL;
        if (vlc)
            CloseHandle(vlc);
        return 1;
    }
    if (!count && !gone) {
        /* (applied already, by another VLC's updater): VLC opens again all
         * the same, and the next exit doesn't run this again */
        logf_(L"nothing to update");
        join(list, work, L"removed.txt");
        DeleteFileW(list);
        join(list, work, L"ready.txt");
        DeleteFileW(list);
        join(list, work, L"manifest.txt");
        DeleteFileW(list);
        HANDLE vlc = again ? relaunch(resume, time) : NULL;
        if (vlc)
            CloseHandle(vlc);
        return 0;
    }
    remove_tree(backup);
    CreateDirectoryW(backup, NULL);

    /* the journal of this run, and a RunOnce entry that undoes a run cut off
     * in the middle at the next sign-in, even when VLC could not start then
     * (its core moved aside before the new one was in place) */
    journal = _wfopen(journal_file, L"w, ccs=UTF-8");
    if (!journal) {
        logf_(L"cannot write %ls: the update waits for the next time", journal_file);
        HANDLE vlc = again ? relaunch(resume, time) : NULL;
        if (vlc)
            CloseHandle(vlc);
        return 1;
    }
    wchar_t self[MAX_PATH], recover_cmd[3 * MAX_PATH];
    const DWORD self_length = GetModuleFileNameW(NULL, self, MAX_PATH);
    if (self_length > 0 && self_length < MAX_PATH
        && _snwprintf(recover_cmd, 3 * MAX_PATH, L"\"%ls\" --recover --app \"%ls\"", self, app) > 0) {
        recover_cmd[3 * MAX_PATH - 1] = 0;
        RegSetKeyValueW(HKEY_CURRENT_USER, runonce_key, runonce_name, REG_SZ, recover_cmd,
                        (DWORD)((wcslen(recover_cmd) + 1) * sizeof(wchar_t)));
    }

    BOOL ok = TRUE;
    for (int i = 0; ok && i < count; ++i) {
        wchar_t from[MAX_PATH], to[MAX_PATH];
        if (!join(from, staged, files[i]) || !join(to, app, files[i]) || !set_aside(files[i])
            || !move_retry(from, to)) {
            logf_(L"cannot put %ls in place (error %lu)", files[i], GetLastError());
            ok = FALSE;
        }
    }
    for (int i = 0; ok && i < gone; ++i) {
        /* a file renamed in its case only (Foo.dll to foo.dll) is the new
         * one on Windows: never removed */
        BOOL renamed = FALSE;
        for (int j = 0; !renamed && j < count; ++j)
            renamed = !_wcsicmp(removed[i], files[j]);
        if (renamed)
            continue;
        wchar_t target[MAX_PATH];
        if (!join(target, app, removed[i]) || (exists(target) && !set_aside(removed[i])))
            ok = FALSE;
    }
    if (!ok) {
        if (roll_back()) {
            journal_end();
            logf_(L"the update failed: VLC Redesign is as it was");
        } else
            logf_(L"the update failed, and VLC Redesign could not be put back as it was: the next run tries again");
        HANDLE vlc = again ? relaunch(resume, time) : NULL;
        if (vlc)
            CloseHandle(vlc);
        return 1;
    }
    /* (the journal ends before the backup goes: a run cut off after this is
     * a whole update, never undone) */
    journal_end();
    logf_(L"%d files updated, %d removed", count, gone);

    record_version(version);
    remove_tree(backup);
    remove_tree(staged);
    join(list, work, L"removed.txt");
    DeleteFileW(list);
    join(list, work, L"ready.txt");
    DeleteFileW(list);
    join(list, work, L"manifest.txt");
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

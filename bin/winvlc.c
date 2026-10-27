/*****************************************************************************
 * winvlc.c: the Windows VLC media player
 *****************************************************************************
 * Copyright (C) 1998-2011 the VideoLAN team
 *
 * Authors: Vincent Seguin <seguin@via.ecp.fr>
 *          Samuel Hocevar <sam@zoy.org>
 *          Gildas Bazin <gbazin@videolan.org>
 *          Derk-Jan Hartman <hartman at videolan dot org>
 *          Lots of other people, see the libvlc AUTHORS file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef UNICODE
#define UNICODE
#endif

#include <vlc/vlc.h>
#include <windows.h>
#include <shellapi.h>

#include <fcntl.h>
#include <io.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "../lib/libvlc_internal.h"

#ifdef HAVE_BREAKPAD
void CheckCrashDump( const wchar_t* crashdump_path );
void* InstallCrashHandler( const wchar_t* crashdump_path );
void ReleaseCrashHandler( void* handler );
#endif

static char *FromWide (const wchar_t *wide)
{
    size_t len;
    len = WideCharToMultiByte (CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);

    char *out = (char *)malloc (len);
    if (out)
        WideCharToMultiByte (CP_UTF8, 0, wide, -1, out, len, NULL, NULL);
    return out;
}

static void PrioritizeSystem32(void)
{
#if _WIN32_WINNT < _WIN32_WINNT_WIN8
    BOOL (WINAPI *SetProcessMitigationPolicy)(PROCESS_MITIGATION_POLICY, PVOID, SIZE_T);
    HINSTANCE h_Kernel32 = GetModuleHandle(TEXT("kernel32.dll"));
    if ( !h_Kernel32 )
        return;
    SetProcessMitigationPolicy = (BOOL (WINAPI *)(PROCESS_MITIGATION_POLICY, PVOID, SIZE_T))
                                   GetProcAddress(h_Kernel32, "SetProcessMitigationPolicy");
    if (SetProcessMitigationPolicy == NULL)
        return;
#endif
    PROCESS_MITIGATION_IMAGE_LOAD_POLICY m = { .Flags = 0 };
    m.PreferSystem32Images = 1;
    SetProcessMitigationPolicy( ProcessImageLoadPolicy, &m, sizeof( m ) );
}

/* (not wcscpy_s and wcscat_s: a path too long for them would end VLC) */
static BOOL CopyPath(wchar_t *path, size_t size, const wchar_t *from)
{
    const size_t len = wcslen(from);
    if (len >= size)
        return FALSE;
    memcpy(path, from, (len + 1) * sizeof(*path));
    return TRUE;
}

static BOOL JoinPath(wchar_t *path, size_t size, const wchar_t *folder, const wchar_t *name)
{
    const size_t folder_len = wcslen(folder), name_len = wcslen(name);
    if (folder_len + 1 + name_len >= size)
        return FALSE;
    memmove(path, folder, folder_len * sizeof(*path));
    path[folder_len] = L'\\';
    memcpy(path + folder_len + 1, name, (name_len + 1) * sizeof(*path));
    return TRUE;
}

/* The profile of a portable copy (the installed copy is one too): the
 * "portable" folder next to vlc.exe, as config_GetAppDir() finds it */
static BOOL PortableProfile(wchar_t *path, DWORD size)
{
    DWORD read = GetModuleFileNameW(NULL, path, size);
    if (read == 0 || read == size)
        return FALSE;
    wchar_t *last = wcsrchr(path, L'\\');
    if (last == NULL)
        return FALSE;
    *last = L'\0';
    if (!JoinPath(path, size, path, L"portable"))
        return FALSE;
    const DWORD attributes = GetFileAttributesW(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static BOOL ExistsIn(const wchar_t *folder, const wchar_t *name)
{
    wchar_t path[MAX_PATH];
    return JoinPath(path, MAX_PATH, folder, name)
        && GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
}

static ULONGLONG WriteTime(const wchar_t *folder, const wchar_t *name)
{
    wchar_t path[MAX_PATH];
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!JoinPath(path, MAX_PATH, folder, name)
     || !GetFileAttributesExW(path, GetFileExInfoStandard, &data))
        return 0;
    return ((ULONGLONG)data.ftLastWriteTime.dwHighDateTime << 32) | data.ftLastWriteTime.dwLowDateTime;
}

/* When the settings of a profile were last written, 0 if it has none */
static ULONGLONG SettingsTime(const wchar_t *profile)
{
    const ULONGLONG core = WriteTime(profile, L"vlcrc");
    const ULONGLONG qt = WriteTime(profile, L"vlc\\vlc-qt-interface.ini");
    return core > qt ? core : qt;
}

static BOOL HasSettings(const wchar_t *profile)
{
    return SettingsTime(profile) != 0;
}

#define PROFILES_KEY L"Software\\VLC Redesign\\Profiles"
/* (the AppId of the installer, extras/juxtopposed/installer/vlc-redesign.iss) */
#define UNINSTALL_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" \
                      L"{8F3C2A61-5B7E-4D9A-9C1E-6A2F0B4D7E15}_is1"

struct last_profile
{
    const wchar_t *self;
    ULONGLONG newest;
    wchar_t path[MAX_PATH];
};

/* A profile with settings, newer than the last one found */
static void ConsiderProfile(struct last_profile *last, const wchar_t *profile, ULONGLONG time)
{
    if (_wcsicmp(profile, last->self) == 0 || ExistsIn(profile, L"standalone"))
        return;
    const ULONGLONG written = SettingsTime(profile);
    if (written == 0)
        return;
    if (written > time)
        time = written;
    if (time > last->newest && CopyPath(last->path, MAX_PATH, profile))
        last->newest = time;
}

/* The profile of a copy of VLC Redesign that a version older than this one
 * did not note: its updater tells it from VLC */
static void ConsiderCopy(struct last_profile *last, const wchar_t *folder)
{
    wchar_t copy[MAX_PATH], profile[MAX_PATH];
    if (!CopyPath(copy, MAX_PATH, folder))
        return;
    size_t len = wcslen(copy);
    while (len > 3 && copy[len - 1] == L'\\')
        copy[--len] = L'\0';
    if (!ExistsIn(copy, L"vlc.exe")
     || (!ExistsIn(copy, L"vlc-redesign-update.exe") && !ExistsIn(copy, L"vlc-waveform.exe"))
     || !JoinPath(profile, MAX_PATH, copy, L"portable"))
        return;
    const DWORD long_len = GetLongPathNameW(profile, copy, MAX_PATH);
    ConsiderProfile(last, long_len > 0 && long_len < MAX_PATH ? copy : profile, 0);
}

/* The copies in the folders of a folder (an unpacked zip: its folder, or the
 * folder in it) that have VLC in their name */
static void ConsiderFolders(struct last_profile *last, const wchar_t *folder, int depth)
{
    wchar_t pattern[MAX_PATH];
    if (!JoinPath(pattern, MAX_PATH, folder, L"*"))
        return;
    WIN32_FIND_DATAW data;
    HANDLE find = FindFirstFileExW(pattern, FindExInfoBasic, &data, FindExSearchLimitToDirectories,
                                   NULL, FIND_FIRST_EX_LARGE_FETCH);
    if (find == INVALID_HANDLE_VALUE)
        return;
    do
    {
        if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
         || (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
         || wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0)
            continue;
        wchar_t child[MAX_PATH];
        if (!JoinPath(child, MAX_PATH, folder, data.cFileName))
            continue;
        if (depth > 0)
        {
            ConsiderCopy(last, child);
            continue;
        }
        wchar_t name[MAX_PATH];
        if (!CopyPath(name, MAX_PATH, data.cFileName) || wcsstr(_wcslwr(name), L"vlc") == NULL)
            continue;
        ConsiderCopy(last, child);
        ConsiderFolders(last, child, depth + 1);
    } while (FindNextFileW(find, &data));
    FindClose(find);
}

/* The profile with settings written last, this one aside: of the copies
 * noted, the installed one, the copies next to this one */
static BOOL LastProfile(HKEY key, const wchar_t *self, wchar_t *path, size_t size)
{
    struct last_profile last = { .self = self };
    for (DWORD i = 0; ; i++)
    {
        wchar_t name[MAX_PATH];
        DWORD name_len = MAX_PATH, type, time_size = sizeof(ULONGLONG);
        ULONGLONG time;
        LONG ret = RegEnumValueW(key, i, name, &name_len, NULL, &type, (BYTE *)&time, &time_size);
        if (ret == ERROR_NO_MORE_ITEMS)
            break;
        if (ret == ERROR_SUCCESS && type == REG_QWORD && time_size == sizeof(time))
            ConsiderProfile(&last, name, time);
    }

    const HKEY roots[] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
    for (size_t i = 0; i < sizeof(roots) / sizeof(roots[0]); i++)
    {
        wchar_t folder[MAX_PATH];
        DWORD folder_size = sizeof(folder);
        if (RegGetValueW(roots[i], UNINSTALL_KEY, L"InstallLocation", RRF_RT_REG_SZ, NULL,
                         folder, &folder_size) == ERROR_SUCCESS)
            ConsiderCopy(&last, folder);
    }

    /* (a zip unpacked by Windows has a folder in its folder) */
    wchar_t parent[MAX_PATH];
    if (CopyPath(parent, MAX_PATH, self))
    {
        for (int up = 0; up < 3; up++)
        {
            wchar_t *end = wcsrchr(parent, L'\\');
            if (end == NULL || end - parent < 2)
                break;
            *end = L'\0';
            if (up > 0)
                ConsiderFolders(&last, parent, 0);
        }
    }

    return last.newest != 0 && CopyPath(path, size, last.path);
}

/* Copies a profile into an empty one, never over a file there; the lock
 * files of a VLC that runs from it, and the caches (the podcasts the library
 * keeps, up to 1 GB: it downloads them again) stay behind */
static void CopyProfile(const wchar_t *from, const wchar_t *to, int depth)
{
    wchar_t pattern[MAX_PATH];
    if (depth > 8 || !JoinPath(pattern, MAX_PATH, from, L"*"))
        return;
    WIN32_FIND_DATAW data;
    HANDLE find = FindFirstFileExW(pattern, FindExInfoBasic, &data, FindExSearchNameMatch,
                                   NULL, FIND_FIRST_EX_LARGE_FETCH);
    if (find == INVALID_HANDLE_VALUE)
        return;
    do
    {
        if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0
         || (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
            continue;
        wchar_t source[MAX_PATH], target[MAX_PATH];
        if (!JoinPath(source, MAX_PATH, from, data.cFileName)
         || !JoinPath(target, MAX_PATH, to, data.cFileName))
            continue;
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (_wcsicmp(data.cFileName, L"cache") == 0)
                continue;
            CreateDirectoryW(target, NULL);
            CopyProfile(source, target, depth + 1);
            continue;
        }
        const size_t len = wcslen(data.cFileName);
        if (len >= 5 && _wcsicmp(data.cFileName + len - 5, L".lock") == 0)
            continue;
        CopyFileW(source, target, TRUE);
    } while (FindNextFileW(find, &data));
    FindClose(find);
}

/* A path as the settings write it (UTF-8): as is, with its \ doubled (the
 * interface's ini), with / */
static char *PathText(const wchar_t *path, int form)
{
    char *utf8 = FromWide(path);
    if (utf8 == NULL || form == 0)
        return utf8;
    char *text = malloc(2 * strlen(utf8) + 1);
    if (text != NULL)
    {
        char *out = text;
        for (const char *in = utf8; *in != '\0'; in++)
        {
            if (*in != '\\')
                *out++ = *in;
            else if (form == 1)
            {
                *out++ = '\\';
                *out++ = '\\';
            }
            else
                *out++ = '/';
        }
        *out = '\0';
    }
    free(utf8);
    return text;
}

/* Replaces a folder's path, where it is a whole folder name, in a text;
 * NULL if it is not in it */
static char *ReplacePath(const char *text, size_t *size, const char *from, const char *to)
{
    const size_t from_len = strlen(from), to_len = strlen(to);
    size_t count = 0;
    for (size_t i = 0; i + from_len <= *size; i++)
        if (_strnicmp(text + i, from, from_len) == 0)
            count++;
    if (count == 0)
        return NULL;
    char *result = malloc(*size + count * (to_len > from_len ? to_len - from_len : 0) + 1);
    if (result == NULL)
        return NULL;
    size_t len = 0, i = 0;
    BOOL replaced = FALSE;
    while (i < *size)
    {
        const char next = i + from_len < *size ? text[i + from_len] : '\0';
        if (i + from_len <= *size && _strnicmp(text + i, from, from_len) == 0
         && strchr("\\/\"\r\n", next) != NULL)
        {
            memcpy(result + len, to, to_len);
            len += to_len;
            i += from_len;
            replaced = TRUE;
        }
        else
            result[len++] = text[i++];
    }
    if (!replaced)
    {
        free(result);
        return NULL;
    }
    *size = len;
    return result;
}

/* Drops the lines that hold a text; FALSE if there is none */
static BOOL DropLines(char *text, size_t *size, const char *what)
{
    const size_t what_len = strlen(what);
    size_t len = 0;
    BOOL dropped = FALSE;
    for (size_t start = 0; start < *size; )
    {
        size_t end = start;
        while (end < *size && text[end] != '\n')
            end++;
        if (end < *size)
            end++;
        BOOL found = FALSE;
        for (size_t i = start; i + what_len <= end && !found; i++)
            found = memcmp(text + i, what, what_len) == 0;
        if (found)
            dropped = TRUE;
        else
        {
            memmove(text + len, text + start, end - start);
            len += end - start;
        }
        start = end;
    }
    *size = len;
    return dropped;
}

/* The settings name files of their own profile (the subtitles downloaded for
 * a video): in the copy, they name the files of the copy. What only the
 * other copy can use (the playlist it would restore after a crash) is
 * dropped. */
static void Repoint(const wchar_t *profile, const wchar_t *name, const wchar_t *from, const wchar_t *to,
                    const char *drop)
{
    wchar_t path[MAX_PATH], temp[MAX_PATH];
    size_t path_len;
    if (!JoinPath(path, MAX_PATH, profile, name) || (path_len = wcslen(path)) + 4 >= MAX_PATH)
        return;
    memcpy(temp, path, path_len * sizeof(*temp));
    memcpy(temp + path_len, L".new", sizeof(L".new"));
    FILE *file = _wfopen(path, L"rb");
    if (file == NULL)
        return;
    char *text = NULL;
    long size = -1;
    if (fseek(file, 0, SEEK_END) == 0)
        size = ftell(file);
    if (size > 0 && size < (32 << 20) && fseek(file, 0, SEEK_SET) == 0
     && (text = malloc(size)) != NULL && fread(text, 1, size, file) != (size_t)size)
    {
        free(text);
        text = NULL;
    }
    fclose(file);
    if (text == NULL)
        return;

    size_t len = size;
    BOOL changed = FALSE;
    for (int form = 0; form < 3; form++)
    {
        char *before = PathText(from, form), *after = PathText(to, form);
        char *result = before && after ? ReplacePath(text, &len, before, after) : NULL;
        if (result != NULL)
        {
            free(text);
            text = result;
            changed = TRUE;
        }
        free(before);
        free(after);
    }
    if (drop != NULL && DropLines(text, &len, drop))
        changed = TRUE;
    if (changed && (file = _wfopen(temp, L"wb")) != NULL)
    {
        const BOOL written = fwrite(text, 1, len, file) == len;
        if (fclose(file) == 0 && written)
            MoveFileExW(temp, path, MOVEFILE_REPLACE_EXISTING);
        else
            DeleteFileW(temp);
    }
    free(text);
}

/* The folder of the copy installed for all users (HKLM), whose users each
 * keep their own profile in %APPDATA% (the installer makes no portable
 * folder there) */
static BOOL InstalledForEveryone(const wchar_t *folder)
{
    wchar_t installed[MAX_PATH];
    DWORD installed_size = sizeof(installed);
    if (RegGetValueW(HKEY_LOCAL_MACHINE, UNINSTALL_KEY, L"InstallLocation", RRF_RT_REG_SZ, NULL,
                     installed, &installed_size) != ERROR_SUCCESS)
        return FALSE;
    size_t len = wcslen(installed);
    while (len > 3 && installed[len - 1] == L'\\')
        installed[--len] = L'\0';
    return _wcsicmp(installed, folder) == 0;
}

/* VLC Redesign keeps its settings apart from those of VLC (%APPDATA%\vlc and
 * HKCU\Software\VideoLAN\VLC, which an installed VLC uses): in its portable
 * folder, made when it is missing, else (a place it can't write to) in
 * %APPDATA%\VLC Redesign, as the core does (src/win32/dirs.c) */
static BOOL ProfileDir(wchar_t *path, DWORD size)
{
    if (PortableProfile(path, size))
        return TRUE;
    const DWORD read = GetModuleFileNameW(NULL, path, size);
    wchar_t *last = (read != 0 && read != size) ? wcsrchr(path, L'\\') : NULL;
    if (last != NULL)
    {
        *last = L'\0';
        /* Never for an install for everyone: a VLC run as administrator
         * would make one profile for all its users */
        if (!InstalledForEveryone(path)
         && JoinPath(path, size, path, L"portable") && CreateDirectoryW(path, NULL))
            return TRUE;
    }
    const wchar_t *appdata = _wgetenv(L"APPDATA");
    if (appdata == NULL || !JoinPath(path, size, appdata, L"VLC Redesign"))
        return FALSE;
    CreateDirectoryW(path, NULL);
    return TRUE;
}

/* A folder that is gone from a drive that is there (not a USB stick taken
 * out, not a network share out of reach) */
static BOOL FolderGone(const wchar_t *folder)
{
    if (wcslen(folder) < 3 || folder[1] != L':' || folder[2] != L'\\')
        return FALSE;
    const wchar_t root[] = { folder[0], L':', L'\\', L'\0' };
    if (GetFileAttributesW(root) == INVALID_FILE_ATTRIBUTES)
        return FALSE;
    if (GetFileAttributesW(folder) != INVALID_FILE_ATTRIBUTES)
        return FALSE;
    const DWORD error = GetLastError();
    return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
}

/* The noted copies whose folder is gone (deleted, or a zip unpacked in a
 * temporary folder): their note goes too */
static void PruneProfiles(HKEY key)
{
    wchar_t (*gone)[MAX_PATH] = malloc(32 * sizeof(*gone));
    if (gone == NULL)
        return;
    DWORD count = 0;
    for (DWORD i = 0; count < 32; i++)
    {
        DWORD name_len = MAX_PATH;
        LONG ret = RegEnumValueW(key, i, gone[count], &name_len, NULL, NULL, NULL, NULL);
        if (ret == ERROR_NO_MORE_ITEMS)
            break;
        if (ret == ERROR_SUCCESS && FolderGone(gone[count]))
            count++;
    }
    for (DWORD i = 0; i < count; i++)
        RegDeleteValueW(key, gone[i]);
    free(gone);
}

/* An update keeps the settings. Each portable copy is noted with the time it
 * last started (HKCU\Software\VLC Redesign\Profiles), and one that starts
 * with an empty profile (a new version unpacked into a new folder, an install
 * in another place) begins with a copy of the profile that started last. The
 * profile of VLC itself (%APPDATA%\vlc, VLC 3's too) is never one of them; a
 * copy with a "standalone" file in its profile keeps to itself. */
static void CarryOverSettings(void)
{
    wchar_t profile[MAX_PATH], path[MAX_PATH];
    if (!ProfileDir(profile, MAX_PATH) || ExistsIn(profile, L"standalone"))
        return;
    /* a portable copy's profile, or the user's own of the copy installed
     * for everyone; not that of a copy that can't write next to itself */
    if (!PortableProfile(path, MAX_PATH))
    {
        const DWORD read = GetModuleFileNameW(NULL, path, MAX_PATH);
        wchar_t *last = (read != 0 && read != MAX_PATH) ? wcsrchr(path, L'\\') : NULL;
        if (last == NULL)
            return;
        *last = L'\0';
        if (!InstalledForEveryone(path))
            return;
    }
    /* (one name for a folder however VLC was started) */
    const DWORD len = GetLongPathNameW(profile, path, MAX_PATH);
    if (len > 0 && len < MAX_PATH)
        CopyPath(profile, MAX_PATH, path);
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, PROFILES_KEY, 0, NULL, 0, KEY_QUERY_VALUE | KEY_SET_VALUE,
                        NULL, &key, NULL) != ERROR_SUCCESS)
        return;
    PruneProfiles(key);
    wchar_t last[MAX_PATH];
    if (!HasSettings(profile) && !ExistsIn(profile, L"ml\\ml.db")
     && LastProfile(key, profile, last, MAX_PATH))
    {
        CopyProfile(last, profile, 0);
        Repoint(profile, L"vlcrc", last, profile, NULL);
        Repoint(profile, L"vlc\\vlc-qt-interface.ini", last, profile, "RecoveryFilePath=");
    }
    FILETIME now;
    GetSystemTimeAsFileTime(&now);
    const ULONGLONG time = ((ULONGLONG)now.dwHighDateTime << 32) | now.dwLowDateTime;
    RegSetValueExW(key, profile, 0, REG_QWORD, (const BYTE *)&time, sizeof(time));
    RegCloseKey(key);
}

/* The interface language is kept in the profile (portable\language: an ISO
 * code, "auto" for the system's), not in the registry it would share with an
 * installed VLC. Until one is chosen it is English, whatever the language of
 * Windows. */
static wchar_t *ProfileLanguage(void)
{
    wchar_t path[MAX_PATH];
    const DWORD size = sizeof(path) / sizeof(path[0]);
    if (!ProfileDir(path, size) || !JoinPath(path, size, path, L"language"))
        return _wcsdup(L"en");
    FILE *file = _wfopen(path, L"rb");
    if (file == NULL)
        return _wcsdup(L"en");
    char code[32] = { 0 };
    size_t len = fread(code, 1, sizeof(code) - 1, file);
    fclose(file);
    while (len > 0 && (code[len - 1] == '\n' || code[len - 1] == '\r' || code[len - 1] == ' '))
        code[--len] = '\0';
    if (len == 0)
        return _wcsdup(L"en");
    wchar_t *wide = malloc((len + 1) * sizeof(*wide));
    if (wide == NULL)
        return NULL;
    for (size_t i = 0; i <= len; i++)
        wide[i] = (unsigned char)code[i];
    return wide;
}

/* The interface (Qt 6.8) needs Windows 10 version 1809 (build 17763), as the
 * installer does: older versions get a message instead of an interface that
 * does not load. RtlGetVersion tells the version whatever the manifest
 * says. */
static BOOL WindowsSupported(void)
{
    typedef LONG (WINAPI *RtlGetVersion_t)(OSVERSIONINFOW *);
    HMODULE ntdll = GetModuleHandle(TEXT("ntdll.dll"));
    RtlGetVersion_t get_version = ntdll == NULL ? NULL
        : (RtlGetVersion_t)(void *)GetProcAddress(ntdll, "RtlGetVersion");
    OSVERSIONINFOW info = { .dwOSVersionInfoSize = sizeof(info) };
    if (get_version == NULL || get_version(&info) != 0)
        return TRUE; /* (unknown: let it try) */
    return info.dwMajorVersion > 10
        || (info.dwMajorVersion == 10 && info.dwBuildNumber >= 17763);
}

static void vlc_kill(void *data)
{
    HANDLE *semp = data;

    ReleaseSemaphore(*semp, 1, NULL);
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine,
                    int nCmdShow )
{
    int argc;

    /* VLC does not change the thread locale, so gettext/libintil will use the
     * user default locale as reference. */
    /* gettext versions 0.18-0.18.1 will use the Windows Vista locale name
     * if the GETTEXT_MUI environment variable is set. If not set or if running
     * on Windows 2000/XP/2003 an hard-coded language ID list is used. This
     * putenv() call may become redundant with later versions of gettext. */
    putenv("GETTEXT_MUI=1");
#ifdef TOP_BUILDDIR
    putenv("VLC_PLUGIN_PATH=Z:"TOP_BUILDDIR"/modules");
    putenv("VLC_DATA_PATH=Z:"TOP_SRCDIR"/share");
#endif

#ifndef NDEBUG
    /* Disable stderr buffering. Indeed, stderr can be buffered on Windows (if
     * connected to a pipe). */
    setvbuf (stderr, NULL, _IONBF, BUFSIZ);
#endif

    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    /* Enable DEP */
    SetProcessDEPPolicy(PROCESS_DEP_ENABLE);

    /* Do NOT load any library from cwd. */
    SetDllDirectory(TEXT(""));

    /***
     * The LoadLibrary* calls from the modules and the 3rd party code
     * will search in SYSTEM32 only
     * */
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
    /***
     * Load DLLs from system32 before any other folder (when possible)
     */
    PrioritizeSystem32();

    if (!WindowsSupported())
    {
        MessageBox(NULL, TEXT("This version of VLC media player needs Windows 10 version 1809 or later."),
                   TEXT("VLC media player"), MB_OK|MB_ICONERROR);
        return 1;
    }

    /* Args */
    wchar_t **wargv = CommandLineToArgvW (GetCommandLine (), &argc);
    if (wargv == NULL)
        return 1;

    /* The installer runs it once so that Windows checks the new program then,
     * not when the first file is opened: nothing else to do (--version opens
     * a console and waits for a key) */
    if (argc == 2 && wcscmp (wargv[1], L"--prewarm") == 0)
    {
        LocalFree (wargv);
        return 0;
    }

    char *argv[argc + 3];
    BOOL crash_handling = TRUE;
    int j = 0;
    WCHAR *lang = NULL;

    argv[j++] = strdup("--media-library");
    argv[j++] = strdup("--no-ignore-config");
    for (int i = 1; i < argc; i++)
    {
        if(!wcscmp(wargv[i], L"--no-crashdump"))
        {
            crash_handling = FALSE;
            continue; /* don't give argument to libvlc */
        }
        if (!wcsncmp(wargv[i], L"--language", 10) )
        {
            if (i < argc - 1 && wcsncmp( wargv[i + 1], L"--", 2 ))
                lang = _wcsdup (wargv[++i]);
            continue;
        }

        argv[j++] = FromWide (wargv[i]);
    }

    argc = j;
    argv[argc] = NULL;
    LocalFree (wargv);

#ifdef HAVE_BREAKPAD
    void* eh = NULL;
    if(crash_handling)
    {
        PWSTR copath;
        if( S_OK != SHGetKnownFolderPath(&FOLDERID_RoamingAppData, KF_FLAG_CREATE, 0, &copath) )
            fprintf( stderr, "Can't open the vlc conf PATH\n" );
        else
        {
            const size_t pathlen = wcslen( copath );
            wchar_t *path = malloc( ( pathlen + wcslen(L"\\vlc\\crashdump") + 1 ) * sizeof(*path) );
            if ( path != NULL)
            {
                memcpy( path, copath, pathlen * sizeof(*path) );
                memcpy( &path[pathlen], L"\\vlc\\crashdump", (wcslen(L"\\vlc\\crashdump") + 1)  * sizeof(*path) );
                CheckCrashDump( path );
                eh = InstallCrashHandler( path );
                free( path );
            }
            CoTaskMemFree( copath );
        }
    }
#else
    (void)crash_handling;
#endif

    _setmode( _fileno( stdin ), _O_BINARY ); /* Needed for pipes */

    /* (before the language: it is one of the settings) */
    CarryOverSettings();

    /* */
    if (!lang)
        lang = ProfileLanguage();

    if (lang && wcsncmp( lang, L"auto", 4 ) )
    {
        char tmp[11];
        snprintf(tmp, 11, "LANG=%ls", lang);
        putenv(tmp);
    }
    free(lang);

    /* Initialize libvlc */
    libvlc_instance_t *vlc;
    vlc = libvlc_new (argc, (const char **)argv);
    if (vlc != NULL)
    {
        HANDLE sem = CreateSemaphore(NULL, 0, 1, NULL);

        libvlc_SetExitHandler(vlc->p_libvlc_int, vlc_kill, &sem);
        libvlc_set_app_id (vlc, "org.VideoLAN.VLC", PACKAGE_VERSION,
                           PACKAGE_NAME);
        libvlc_set_user_agent (vlc, "VLC media player", "VLC/"PACKAGE_VERSION);
        libvlc_InternalAddIntf (vlc->p_libvlc_int, NULL);
        libvlc_InternalPlay (vlc->p_libvlc_int);

        WaitForSingleObject(sem, INFINITE);
        CloseHandle(sem);

        libvlc_release (vlc);
    }
    else
        MessageBox (NULL, TEXT("VLC media player could not start.\n"
                    "Either the command line options were invalid or no plugins were found.\n"),
                    TEXT("VLC media player"),
                    MB_OK|MB_ICONERROR);


#ifdef HAVE_BREAKPAD
    ReleaseCrashHandler( eh );
#endif
    for (int i = 0; i < argc; i++)
        free (argv[i]);

    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
    return 0;
}

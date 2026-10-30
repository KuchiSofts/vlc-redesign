/*****************************************************************************
 * breakpad.cpp: Wrapper to breakpad crash handler
 *****************************************************************************
 * Copyright (C) 1998-2017 VLC authors
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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

#include <windows.h>
#include "winvlc_message.h"
#include "client/windows/handler/exception_handler.h"
#include "common/windows/http_upload.h"
#include <memory>
#include <map>
#include <string>
#include <sstream>

using google_breakpad::ExceptionHandler;

static bool FilterCallback(void*, EXCEPTION_POINTERS*, MDRawAssertionInfo*)
{
    // Don't spam breakpad if we're debugging
    return !IsDebuggerPresent();
}

extern "C"
{

#define WIDEN_(x) L ## x
#define WIDEN(x) WIDEN_(x)

void CheckCrashDump( const wchar_t* path )
{
    std::wstringstream pattern;
    WIN32_FIND_DATAW data;
    pattern << path << L"/*.dmp";
    HANDLE h = FindFirstFileW( pattern.str().c_str(), &data );
    if (h == INVALID_HANDLE_VALUE)
        return;
    /* drawn as the dialogs of the interface (before it starts) */
    const bool send = RedesignMessageBox( L"VLC Redesign",
        L"VLC Redesign closed unexpectedly the last time",
        L"Would you like to send a report of the crash to the developers?",
        L"Send report", L"Don't send" ) == 1;
    std::map<std::wstring, std::wstring> params;
    params[L"prod"] = L"VLC";
    params[L"ver"] = WIDEN(PACKAGE_VERSION);
    do
    {
        std::wstringstream fullPath;
        fullPath << path << L'/' << data.cFileName;
        if( send )
        {
            std::map<std::wstring, std::wstring> files;
            files[L"upload_file_minidump"] = fullPath.str();
            google_breakpad::HTTPUpload::SendRequest(
                            WIDEN( BREAKPAD_URL "/reports" ), params, files,
                            NULL, NULL, NULL );
        }
        DeleteFileW( fullPath.str().c_str() );
    } while ( FindNextFileW( h, &data ) );
    FindClose(h);
}

void* InstallCrashHandler( const wchar_t* crashdump_path )
{
    // Breakpad needs the folder to exist to generate the crashdump
    CreateDirectoryW( crashdump_path, NULL );
    return new(std::nothrow) ExceptionHandler( crashdump_path, FilterCallback,
                                NULL, NULL, ExceptionHandler::HANDLER_ALL);
}

void ReleaseCrashHandler( void* handler )
{
    ExceptionHandler* eh = reinterpret_cast<ExceptionHandler*>( handler );
    delete eh;
}

}

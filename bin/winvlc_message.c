/*****************************************************************************
 * winvlc_message.c: the messages vlc.exe shows before the interface exists
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
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

/* Drawn with GDI only, in the sizes of RedesignDialog.qml (Figma "Frame
 * 120", "Frame 124", "Frame 225") and the colours of its tokens; Windows 11
 * rounds the window and gives it its shadow (DWM). The interface's font
 * (Inter) lives in the Qt plugin: Segoe UI Variable, else Segoe UI. */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <dwmapi.h>

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "winvlc_message.h"

/* (not in every SDK) */
#define REDESIGN_DWMWA_WINDOW_CORNER_PREFERENCE 33
#define REDESIGN_DWMWA_BORDER_COLOR 34
#define REDESIGN_DWMWCP_ROUND 2
#define REDESIGN_DPI_PER_MONITOR_AWARE_V2 ((HANDLE)(LONG_PTR)-4)

enum
{
    PART_NONE = 0,
    PART_CLOSE,
    PART_CANCEL,
    PART_ACCEPT,
};

typedef struct
{
    COLORREF card, title_bar, title, close_hover;
    COLORREF heading, text;
    COLORREF button, button_hover, button_text;
    COLORREF accent, accent_background, accent_hover;
    COLORREF border;
} message_colors;

typedef struct
{
    const wchar_t *title, *heading, *text, *accept, *cancel;
    HWND hwnd;
    UINT dpi;
    message_colors colors;
    HFONT font, font_bold, font_heading;
    int width, height;
    RECT title_rect, close_rect, heading_rect, text_rect, cancel_rect, accept_rect;
    int hover, pressed, focus;
    BOOL tracking;
    int result;
    BOOL done;
} message_box;

static int Scale(const message_box *box, int px)
{
    return MulDiv(px, (int)box->dpi, 96);
}

/* The Windows theme of the applications: light, or dark */
static BOOL LightTheme(void)
{
    DWORD value = 0, size = sizeof (value);
    if (RegGetValueW(HKEY_CURRENT_USER,
                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                     L"AppsUseLightTheme", RRF_RT_REG_DWORD, NULL, &value, &size) != ERROR_SUCCESS)
        return FALSE;
    return value != 0;
}

/* The values of RedesignDialog.qml in dark, and what VLCStyle.redesign.color()
 * gives in light; the default accent (orange), the settings not read yet */
static void SetColors(message_colors *c, BOOL light)
{
    if (!light)
    {
        c->card = RGB(0x19, 0x19, 0x19);
        c->title_bar = RGB(0x09, 0x09, 0x09);
        c->title = RGB(0x8F, 0x8F, 0x8F);
        c->close_hover = RGB(0x22, 0x22, 0x22);
        c->heading = RGB(0xC6, 0xC6, 0xC6);
        c->text = RGB(0x6B, 0x6B, 0x6B);
        c->button = RGB(0x1B, 0x1B, 0x1B);
        c->button_hover = RGB(0x22, 0x22, 0x22);
        c->button_text = RGB(0x8F, 0x8F, 0x8F);
        c->accent_background = RGB(0x37, 0x1F, 0x06);
        c->accent_hover = RGB(0x3F, 0x24, 0x07);
        /* the frame of the main window (color.dark.windowBorder) */
        c->border = RGB(0x4C, 0x4A, 0x48);
    }
    else
    {
        c->card = RGB(0xAB, 0xAB, 0xAA);
        c->title_bar = RGB(0x9C, 0x9C, 0x9B);
        c->title = RGB(0x2E, 0x2E, 0x2E);
        c->close_hover = RGB(0x9C, 0x9C, 0x9B);
        c->heading = RGB(0x2E, 0x2E, 0x2E);
        c->text = RGB(0x4D, 0x4D, 0x4D);
        c->button = RGB(0xB8, 0xB8, 0xB7);
        c->button_hover = RGB(0x9C, 0x9C, 0x9B);
        c->button_text = RGB(0x4D, 0x4D, 0x4D);
        c->accent_background = RGB(0xDA, 0xBB, 0x99);
        c->accent_hover = RGB(0xFA, 0xD6, 0xB0);
        /* (color.light.windowBorder) */
        c->border = RGB(0x4C, 0x4A, 0x48);
    }
    c->accent = RGB(0xD3, 0x79, 0x18);
}

static int CALLBACK FontFound(const LOGFONTW *font, const TEXTMETRICW *metric,
                              DWORD type, LPARAM data)
{
    (void)font; (void)metric; (void)type;
    *(BOOL *)data = TRUE;
    return 0;
}

static const wchar_t *FontFamily(void)
{
    static const wchar_t *family = NULL;
    if (family)
        return family;
    family = L"Segoe UI";
    HDC dc = GetDC(NULL);
    if (dc)
    {
        LOGFONTW query = { 0 };
        query.lfCharSet = DEFAULT_CHARSET;
        wcscpy(query.lfFaceName, L"Segoe UI Variable Text");
        BOOL found = FALSE;
        EnumFontFamiliesExW(dc, &query, FontFound, (LPARAM)&found, 0);
        if (found)
            family = L"Segoe UI Variable Text";
        ReleaseDC(NULL, dc);
    }
    return family;
}

static HFONT MakeFont(const message_box *box, int px, int weight)
{
    return CreateFontW(-Scale(box, px), 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       /* greyscale, as the interface draws its text */
                       ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, FontFamily());
}

static void FreeFonts(message_box *box)
{
    if (box->font)
        DeleteObject(box->font);
    if (box->font_bold)
        DeleteObject(box->font_bold);
    if (box->font_heading)
        DeleteObject(box->font_heading);
    box->font = box->font_bold = box->font_heading = NULL;
}

static int TextHeight(HDC dc, HFONT font, const wchar_t *text, int width)
{
    RECT rect = { 0, 0, width, 0 };
    HGDIOBJ old = SelectObject(dc, font);
    DrawTextW(dc, text, -1, &rect, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL);
    SelectObject(dc, old);
    return rect.bottom - rect.top;
}

static int TextWidth(HDC dc, HFONT font, const wchar_t *text)
{
    RECT rect = { 0, 0, 0, 0 };
    HGDIOBJ old = SelectObject(dc, font);
    DrawTextW(dc, text, -1, &rect, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, old);
    return rect.right - rect.left;
}

/* The design's sizes at the DPI of the window: 560 wide; the title bar
 * (padding 9, the row of 32: the title 10 in, the close button 32 x 32);
 * the content (padding 22 / 20; the heading, Inter 16 on lines of 28, and
 * 5 below it the explanation, Inter 14); the footer (72 high, padding 20,
 * buttons 100 x 32 at least) */
static void Layout(message_box *box)
{
    FreeFonts(box);
    box->font = MakeFont(box, 14, FW_NORMAL);
    box->font_bold = MakeFont(box, 14, FW_SEMIBOLD);
    box->font_heading = MakeFont(box, 16, FW_NORMAL);

    const int width = Scale(box, 560);
    const int pad = Scale(box, 20);
    const int title_bar = Scale(box, 9 + 32 + 9);

    SetRect(&box->close_rect, width - Scale(box, 9 + 32), Scale(box, 9),
            width - Scale(box, 9), Scale(box, 9 + 32));
    SetRect(&box->title_rect, Scale(box, 9 + 10), Scale(box, 9),
            box->close_rect.left - Scale(box, 4), Scale(box, 9 + 32));

    HDC dc = GetDC(box->hwnd);
    int y = title_bar + Scale(box, 22);
    const int text_width = width - 2 * pad;
    int heading = TextHeight(dc, box->font_heading, box->heading, text_width);
    if (heading < Scale(box, 28))
    {
        /* one line: centred in its 28 */
        SetRect(&box->heading_rect, pad, y + (Scale(box, 28) - heading) / 2,
                width - pad, y + (Scale(box, 28) - heading) / 2 + heading);
        heading = Scale(box, 28);
    }
    else
        SetRect(&box->heading_rect, pad, y, width - pad, y + heading);
    y += heading;
    if (box->text && box->text[0])
    {
        y += Scale(box, 5);
        const int text = TextHeight(dc, box->font, box->text, text_width);
        SetRect(&box->text_rect, pad, y, width - pad, y + text);
        y += text;
    }
    else
        SetRectEmpty(&box->text_rect);
    y += Scale(box, 22);

    const int button_height = Scale(box, 32);
    const int button_top = y + Scale(box, 20);
    int accept = TextWidth(dc, box->font_bold, box->accept) + 2 * Scale(box, 16);
    if (accept < Scale(box, 100))
        accept = Scale(box, 100);
    SetRect(&box->accept_rect, width - pad - accept, button_top, width - pad, button_top + button_height);
    if (box->cancel)
    {
        int cancel = TextWidth(dc, box->font_bold, box->cancel) + 2 * Scale(box, 16);
        if (cancel < Scale(box, 100))
            cancel = Scale(box, 100);
        SetRect(&box->cancel_rect, pad, button_top, pad + cancel, button_top + button_height);
    }
    else
        SetRectEmpty(&box->cancel_rect);
    ReleaseDC(box->hwnd, dc);

    box->width = width;
    box->height = y + Scale(box, 72);
}

static void FillRounded(HDC dc, const RECT *rect, int radius, COLORREF color, BOOL outline, COLORREF outline_color)
{
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = outline ? CreatePen(PS_SOLID, 1, outline_color) : (HPEN)GetStockObject(NULL_PEN);
    HGDIOBJ old_brush = SelectObject(dc, brush);
    HGDIOBJ old_pen = SelectObject(dc, pen);
    /* (a null pen leaves the right and bottom edges out) */
    RoundRect(dc, rect->left, rect->top, rect->right + (outline ? 0 : 1),
              rect->bottom + (outline ? 0 : 1), 2 * radius, 2 * radius);
    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    DeleteObject(brush);
    if (outline)
        DeleteObject(pen);
}

static void DrawButton(message_box *box, HDC dc, const RECT *rect, const wchar_t *text, int part, BOOL accent)
{
    const message_colors *c = &box->colors;
    const BOOL hot = box->hover == part || box->pressed == part;
    COLORREF background = accent ? (hot ? c->accent_hover : c->accent_background)
                                 : (hot ? c->button_hover : c->button);
    FillRounded(dc, rect, Scale(box, 6), background, box->focus == part, c->accent);
    SetTextColor(dc, accent ? c->accent : c->button_text);
    HGDIOBJ old = SelectObject(dc, box->font_bold);
    RECT text_rect = *rect;
    DrawTextW(dc, text, -1, &text_rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
    SelectObject(dc, old);
}

static void Paint(message_box *box, HDC target)
{
    const message_colors *c = &box->colors;
    RECT client;
    GetClientRect(box->hwnd, &client);

    HDC dc = CreateCompatibleDC(target);
    HBITMAP bitmap = CreateCompatibleBitmap(target, client.right, client.bottom);
    HGDIOBJ old_bitmap = SelectObject(dc, bitmap);
    SetBkMode(dc, TRANSPARENT);

    HBRUSH card = CreateSolidBrush(c->card);
    FillRect(dc, &client, card);
    DeleteObject(card);

    RECT bar = { 0, 0, client.right, Scale(box, 9 + 32 + 9) };
    HBRUSH title_bar = CreateSolidBrush(c->title_bar);
    FillRect(dc, &bar, title_bar);
    DeleteObject(title_bar);

    /* the title, muted, cut short with an ellipsis */
    HGDIOBJ old_font = SelectObject(dc, box->font);
    SetTextColor(dc, c->title);
    RECT title = box->title_rect;
    DrawTextW(dc, box->title, -1, &title, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);

    /* the close button: a cross of the size of the icon of the design
     * (24, the cross in its middle 12) */
    if (box->hover == PART_CLOSE || box->pressed == PART_CLOSE || box->focus == PART_CLOSE)
        FillRounded(dc, &box->close_rect, Scale(box, 4),
                    (box->hover == PART_CLOSE || box->pressed == PART_CLOSE) ? c->close_hover : c->title_bar,
                    box->focus == PART_CLOSE, c->accent);
    {
        const int cx = (box->close_rect.left + box->close_rect.right) / 2;
        const int cy = (box->close_rect.top + box->close_rect.bottom) / 2;
        const int arm = Scale(box, 5);
        int stroke = Scale(box, 3) / 2;
        if (stroke < 1)
            stroke = 1;
        HPEN pen = CreatePen(PS_SOLID, stroke, c->title);
        HGDIOBJ old_pen = SelectObject(dc, pen);
        MoveToEx(dc, cx - arm, cy - arm, NULL);
        LineTo(dc, cx + arm + 1, cy + arm + 1);
        MoveToEx(dc, cx + arm, cy - arm, NULL);
        LineTo(dc, cx - arm - 1, cy + arm + 1);
        SelectObject(dc, old_pen);
        DeleteObject(pen);
    }

    SelectObject(dc, box->font_heading);
    SetTextColor(dc, c->heading);
    RECT heading = box->heading_rect;
    DrawTextW(dc, box->heading, -1, &heading, DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL);

    if (!IsRectEmpty(&box->text_rect))
    {
        SelectObject(dc, box->font);
        SetTextColor(dc, c->text);
        RECT text = box->text_rect;
        DrawTextW(dc, box->text, -1, &text, DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL);
    }
    SelectObject(dc, old_font);

    if (box->cancel)
        DrawButton(box, dc, &box->cancel_rect, box->cancel, PART_CANCEL, FALSE);
    DrawButton(box, dc, &box->accept_rect, box->accept, PART_ACCEPT, TRUE);

    BitBlt(target, 0, 0, client.right, client.bottom, dc, 0, 0, SRCCOPY);
    SelectObject(dc, old_bitmap);
    DeleteObject(bitmap);
    DeleteDC(dc);
}

static int HitTest(const message_box *box, POINT point)
{
    if (PtInRect(&box->close_rect, point))
        return PART_CLOSE;
    if (box->cancel && PtInRect(&box->cancel_rect, point))
        return PART_CANCEL;
    if (PtInRect(&box->accept_rect, point))
        return PART_ACCEPT;
    return PART_NONE;
}

static void Finish(message_box *box, int result)
{
    if (box->done)
        return;
    box->result = result;
    box->done = TRUE;
    DestroyWindow(box->hwnd);
}

static void Activate(message_box *box, int part)
{
    if (part == PART_ACCEPT)
        Finish(box, 1);
    else if (part == PART_CANCEL || part == PART_CLOSE)
        Finish(box, 0);
}

static void Place(message_box *box, const RECT *suggested)
{
    int x, y;
    if (suggested)
    {
        x = suggested->left;
        y = suggested->top;
    }
    else
    {
        /* centred in the screen where the pointer is */
        POINT cursor;
        GetCursorPos(&cursor);
        MONITORINFO info = { .cbSize = sizeof (info) };
        GetMonitorInfoW(MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY), &info);
        x = (info.rcWork.left + info.rcWork.right - box->width) / 2;
        y = (info.rcWork.top + info.rcWork.bottom - box->height) / 2;
    }
    SetWindowPos(box->hwnd, NULL, x, y, box->width, box->height, SWP_NOZORDER | SWP_NOACTIVATE);
}

static LRESULT CALLBACK MessageProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    message_box *box = (message_box *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (msg == WM_NCCREATE)
    {
        const CREATESTRUCTW *create = (const CREATESTRUCTW *)lparam;
        box = create->lpCreateParams;
        box->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)box);
    }
    if (!box)
        return DefWindowProcW(hwnd, msg, wparam, lparam);

    switch (msg)
    {
    case WM_NCHITTEST:
    {
        /* the title bar moves the window */
        POINT point = { (short)LOWORD(lparam), (short)HIWORD(lparam) };
        ScreenToClient(hwnd, &point);
        if (point.y < Scale(box, 9 + 32 + 9) && !PtInRect(&box->close_rect, point))
            return HTCAPTION;
        return HTCLIENT;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        Paint(box, dc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEMOVE:
    {
        POINT point = { (short)LOWORD(lparam), (short)HIWORD(lparam) };
        const int hover = HitTest(box, point);
        if (!box->tracking)
        {
            TRACKMOUSEEVENT track = { .cbSize = sizeof (track), .dwFlags = TME_LEAVE, .hwndTrack = hwnd };
            box->tracking = TrackMouseEvent(&track);
        }
        if (hover != box->hover)
        {
            box->hover = hover;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_MOUSELEAVE:
        box->tracking = FALSE;
        if (box->hover != PART_NONE)
        {
            box->hover = PART_NONE;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    case WM_LBUTTONDOWN:
    {
        POINT point = { (short)LOWORD(lparam), (short)HIWORD(lparam) };
        box->pressed = HitTest(box, point);
        if (box->pressed != PART_NONE)
            SetCapture(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_LBUTTONUP:
    {
        POINT point = { (short)LOWORD(lparam), (short)HIWORD(lparam) };
        const int pressed = box->pressed;
        box->pressed = PART_NONE;
        if (GetCapture() == hwnd)
            ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);
        if (pressed != PART_NONE && HitTest(box, point) == pressed)
            Activate(box, pressed);
        return 0;
    }
    case WM_KEYDOWN:
        switch (wparam)
        {
        case VK_ESCAPE:
            Finish(box, 0);
            return 0;
        case VK_RETURN:
        case VK_SPACE:
            if (box->focus != PART_NONE)
                Activate(box, box->focus);
            else if (wparam == VK_RETURN)
                Activate(box, PART_ACCEPT);
            return 0;
        case VK_TAB:
        case VK_LEFT:
        case VK_RIGHT:
        {
            /* the buttons in their order: close, cancel, the action */
            int order[3], count = 0, at = -1;
            order[count++] = PART_CLOSE;
            if (box->cancel)
                order[count++] = PART_CANCEL;
            order[count++] = PART_ACCEPT;
            for (int i = 0; i < count; i++)
                if (order[i] == box->focus)
                    at = i;
            const BOOL back = wparam == VK_LEFT || (wparam == VK_TAB && GetKeyState(VK_SHIFT) < 0);
            if (at < 0)
                at = back ? count - 1 : (box->cancel ? 1 : count - 1);
            else
                at = (at + (back ? count - 1 : 1)) % count;
            box->focus = order[at];
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        }
        break;
    case WM_DPICHANGED:
    {
        box->dpi = HIWORD(wparam);
        Layout(box);
        Place(box, (const RECT *)lparam);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_CLOSE:
        Finish(box, 0);
        return 0;
    case WM_DESTROY:
        box->done = TRUE;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

int RedesignMessageBox(const wchar_t *title, const wchar_t *heading,
                       const wchar_t *text, const wchar_t *accept,
                       const wchar_t *cancel)
{
    message_box box = { 0 };
    box.title = title ? title : L"VLC Redesign";
    box.heading = heading ? heading : L"";
    box.text = text;
    box.accept = accept ? accept : L"OK";
    box.cancel = cancel;
    SetColors(&box.colors, LightTheme());

    HINSTANCE instance = GetModuleHandleW(NULL);
    static const wchar_t class_name[] = L"VLCRedesignMessage";
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof (wc);
    if (!GetClassInfoExW(instance, class_name, &wc))
    {
        memset(&wc, 0, sizeof (wc));
        wc.cbSize = sizeof (wc);
        /* the shadow of Windows 10 (Windows 11 draws its own) */
        wc.style = CS_DROPSHADOW;
        wc.lpfnWndProc = MessageProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
        wc.hIcon = LoadIconW(instance, L"VLC_ICON");
        wc.hIconSm = wc.hIcon;
        wc.lpszClassName = class_name;
        if (!RegisterClassExW(&wc))
            return MessageBoxW(NULL, box.text ? box.text : box.heading, box.title,
                               box.cancel ? MB_OKCANCEL : MB_OK) == IDOK;
    }

    /* drawn at the DPI of its screen (vlc.exe does not declare it, Qt sets
     * it for itself later) */
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    typedef HANDLE (WINAPI *SetThreadDpiAwarenessContext_t)(HANDLE);
    typedef UINT (WINAPI *GetDpiForWindow_t)(HWND);
    SetThreadDpiAwarenessContext_t set_awareness = user32
        ? (SetThreadDpiAwarenessContext_t)(void *)GetProcAddress(user32, "SetThreadDpiAwarenessContext") : NULL;
    GetDpiForWindow_t get_dpi = user32
        ? (GetDpiForWindow_t)(void *)GetProcAddress(user32, "GetDpiForWindow") : NULL;
    HANDLE previous = set_awareness ? set_awareness(REDESIGN_DPI_PER_MONITOR_AWARE_V2) : NULL;

    HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, class_name, box.title,
                                WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX,
                                0, 0, 1, 1, NULL, NULL, instance, &box);
    if (!hwnd)
    {
        if (set_awareness && previous)
            set_awareness(previous);
        return MessageBoxW(NULL, box.text ? box.text : box.heading, box.title,
                           box.cancel ? MB_OKCANCEL : MB_OK) == IDOK;
    }

    if (get_dpi)
        box.dpi = get_dpi(hwnd);
    if (!box.dpi)
    {
        HDC dc = GetDC(NULL);
        box.dpi = dc ? (UINT)GetDeviceCaps(dc, LOGPIXELSY) : 96;
        if (dc)
            ReleaseDC(NULL, dc);
    }
    Layout(&box);
    Place(&box, NULL);

    /* Windows 11: the corners rounded, the 1 px border of the main window */
    const DWORD corner = REDESIGN_DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, REDESIGN_DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof (corner));
    const COLORREF border = box.colors.border;
    DwmSetWindowAttribute(hwnd, REDESIGN_DWMWA_BORDER_COLOR, &border, sizeof (border));

    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    UpdateWindow(hwnd);

    MSG msg;
    BOOL quit = FALSE;
    while (!box.done)
    {
        const BOOL got = GetMessageW(&msg, NULL, 0, 0);
        if (got == -1)
            break;
        if (got == 0)
        {
            quit = TRUE;
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (!box.done && IsWindow(hwnd))
        DestroyWindow(hwnd);
    if (quit)
        PostQuitMessage((int)msg.wParam);

    FreeFonts(&box);
    if (set_awareness && previous)
        set_awareness(previous);
    return box.result;
}

static wchar_t *Widen(const char *text)
{
    if (!text)
        return NULL;
    const int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (length <= 0)
        return NULL;
    wchar_t *wide = malloc(length * sizeof (*wide));
    if (wide)
        MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, length);
    return wide;
}

int RedesignMessageBoxUtf8(const char *title, const char *heading,
                           const char *text, const char *accept,
                           const char *cancel)
{
    wchar_t *wtitle = Widen(title), *wheading = Widen(heading), *wtext = Widen(text);
    wchar_t *waccept = Widen(accept), *wcancel = Widen(cancel);
    const int result = RedesignMessageBox(wtitle, wheading, wtext, waccept, wcancel);
    free(wtitle);
    free(wheading);
    free(wtext);
    free(waccept);
    free(wcancel);
    return result;
}

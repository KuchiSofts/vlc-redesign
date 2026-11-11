/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#include "screen_fit.hpp"

#include <QCoreApplication>
#include <QEvent>
#include <QPointer>
#include <QScreen>
#include <QTimer>
#include <QWidget>
#include <QWindow>

namespace
{

bool held(Qt::WindowStates states)
{
    return states & (Qt::WindowMaximized | Qt::WindowFullScreen | Qt::WindowMinimized);
}

// a dialog of widgets (Media Information, Messages, the advanced
// preferences...): QVLCFrame is a plain window, the others dialogs
bool wanted(const QWidget *widget)
{
    if (!widget->isWindow())
        return false;
    switch (widget->windowType())
    {
    case Qt::Dialog:
    case Qt::Sheet:
    case Qt::Tool:
        return true;
    case Qt::Window:
        return widget->inherits("QVLCFrame");
    default: // menus, tool tips, the lists of combo boxes: Qt places them
        return false;
    }
}

// a window of the QML interface: its dialogs, and the windows that belong
// to the main one (the detached play queue). Not the main window (its own
// geometry is restored and corrected by InterfaceWindowHandler), not a
// video window.
bool wanted(const QWindow *window)
{
    if (!window->isTopLevel() || window->inherits("QWidgetWindow"))
        return false;
    switch (window->type())
    {
    case Qt::Dialog:
    case Qt::Sheet:
    case Qt::Tool:
        return true;
    case Qt::Window:
        return window->transientParent() != nullptr;
    default:
        return false;
    }
}

void fit(QWidget *widget)
{
    if (!widget->isVisible() || held(widget->windowState()))
        return;
    const QScreen *screen = widget->screen();
    if (!screen)
        return;
    const QRect available = screen->availableGeometry();
    const QRect client = widget->geometry();
    const QRect outer = widget->frameGeometry();
    const QMargins frame(client.left() - outer.left(), client.top() - outer.top(),
                         outer.right() - client.right(), outer.bottom() - client.bottom());
    const QRect target = ScreenFit::fitted(client, available, frame);
    if (target == client)
        return;
    // a smallest size the screen cannot hold would refuse the new one
    if (widget->minimumWidth() > target.width())
        widget->setMinimumWidth(target.width());
    if (widget->minimumHeight() > target.height())
        widget->setMinimumHeight(target.height());
    widget->setGeometry(target);
}

void fit(QWindow *window)
{
    if (!window->isVisible() || held(window->windowStates()))
        return;
    const QScreen *screen = window->screen();
    if (!screen)
        return;
    const QRect client = window->geometry();
    const QRect target = ScreenFit::fitted(client, screen->availableGeometry(),
                                           window->frameMargins());
    if (target == client)
        return;
    if (window->minimumWidth() > target.width())
        window->setMinimumWidth(target.width());
    if (window->minimumHeight() > target.height())
        window->setMinimumHeight(target.height());
    window->setGeometry(target);
}

class Filter : public QObject
{
public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() != QEvent::Show)
            return false;
        // once shown: its frame is known then, and a geometry restored
        // right after show() is in place
        if (object->isWidgetType())
        {
            auto widget = static_cast<QWidget *>(object);
            if (wanted(widget))
                QTimer::singleShot(0, widget, [widget = QPointer<QWidget>(widget)]() {
                    if (widget)
                        fit(widget);
                });
        }
        else if (object->isWindowType())
        {
            auto window = static_cast<QWindow *>(object);
            if (wanted(window))
                QTimer::singleShot(0, window, [window = QPointer<QWindow>(window)]() {
                    if (window)
                        fit(window);
                });
        }
        return false;
    }
};

}

void ScreenFit::install(QObject *parent)
{
    if (QCoreApplication *app = QCoreApplication::instance())
        app->installEventFilter(new Filter(parent ? parent : app));
}

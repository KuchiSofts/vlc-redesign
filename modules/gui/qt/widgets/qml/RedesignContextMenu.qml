/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQml
import QtQuick
import QtQuick.Templates as T
import QtQuick.Window

// Context menu of the library as the design's menu card (Figma "context
// menu", "file context menu", "playlist context menu", "folder context
// menu"): a RedesignMenu of rows made from 'actions' once the data of the
// items asked comes. Same API as NativeMenu (popup(indexes, point, options),
// requestData / setData), plus toggleFrom() for "more" buttons.
QtObject {
    id: root

    /**
      actions - list of the rows, in order; each one an Object:

        "text" - (string or function(dataList, options, indexes) -> string)

        "icon" - (string) (optional) icon of the redesign set (RedesignMenuItem.iconName)

        "action" - (function(dataList, options, indexes)) called once the card is closed

        "visible" - (Boolean or function(dataList, options, indexes) -> Boolean) (optional)
                    whether the row is in the card

        "enabled" - (Boolean or function(dataList, options, indexes) -> Boolean) (optional)
                    a row that does not apply to these items is shown greyed out

        "separator" - (Boolean) (optional) a line, kept only between two rows
    **/

    property var actions: []

    // The item the card opens in, for popup(): by default the library's display
    property Item parentItem: null

    property alias visible: menu.visible

    readonly property int currentRequest: _currentRequest

    signal requestData(var requestID, var indexes)


    property var _options: null

    property var _indexes: []

    property var _dataList: null

    property int _currentRequest: 0

    // where the card opens: at a point of the screen, or of a button
    property point _popupPoint
    property Item _button: null

    // focused before the card took the focus, given back when it closes
    property Item _focusItem: null

    // the row chosen, run once the card is closed (and the focus back)
    property var _chosen: null

    // requests data for 'indexes' and shows the card at 'point' (global)
    // with the actions of 'options'; returns the request id
    function popup(indexes, point, options) {
        return _request(indexes, options, null, point)
    }

    // From the click of a "more" button: opens the card at x, y of the
    // button (by default under it), or closes it. A press outside the card,
    // the button too, only closes it (modal).
    function toggleFrom(button, x, y, indexes, options) {
        if (menu.opened) {
            menu.close()
            return _currentRequest
        }
        return _request(indexes ?? [], options, button,
                        Qt.point(x ?? 0, y ?? button.height))
    }

    function close() {
        menu.close()
    }

    function setData(id, data) {
        if (_currentRequest !== id)
            return

        _dataList = data

        const rows = []
        for (const action of actions) {
            if (!_test(action, "visible", true))
                continue
            if (action.separator) {
                // never first, nor two in a row
                if (rows.length > 0 && !rows[rows.length - 1].separator)
                    rows.push(action)
                continue
            }
            rows.push(action)
        }
        // nor last
        while (rows.length > 0 && rows[rows.length - 1].separator)
            rows.pop()
        if (rows.length === 0)
            return

        const host = _button ?? _host()
        if (!host) {
            console.warn("RedesignContextMenu: no item to open in")
            return
        }

        if (menu.opened)
            menu.close()

        while (menu.count > 0)
            menu.removeItem(menu.itemAt(0))

        for (const action of rows) {
            let item
            if (action.separator) {
                item = _separatorComponent.createObject(menu.contentItem)
            } else {
                const text = (typeof action.text === "function")
                             ? action.text(_dataList, _options, _indexes) : action.text
                item = _rowComponent.createObject(menu.contentItem, {
                    text: text,
                    iconName: action.icon ?? "",
                    enabled: _test(action, "enabled", true),
                    entry: action
                })
            }
            menu.addItem(item)
        }

        _chosen = null
        _focusItem = host.Window.activeFocusItem

        if (_button) {
            menu.popupFrom(_button, _popupPoint.x, _popupPoint.y)
        } else {
            const pos = host.mapFromGlobal(_popupPoint.x, _popupPoint.y)
            // right to left: on the left of the pointer, as Windows does
            menu.popup(host, menu._rtl ? pos.x - menu.width : pos.x, pos.y)
        }
    }

    function _request(indexes, options, button, point) {
        _options = options
        _indexes = indexes
        _dataList = null
        _button = button
        _popupPoint = point

        const requestID = ++_currentRequest
        requestData(requestID, indexes)
        return requestID
    }

    function _host() {
        if (parentItem)
            return parentItem
        return (typeof g_mainDisplay !== "undefined") ? g_mainDisplay : null
    }

    // a Boolean key of an action, or its function
    function _test(action, key, fallback) {
        if (!action.hasOwnProperty(key))
            return fallback
        const value = action[key]
        if (typeof value === "function")
            return !!value(_dataList, _options, _indexes)
        return !!value
    }


    readonly property RedesignMenu _menu: RedesignMenu {
        id: menu

        // a press outside only closes it, and goes no further (as a native
        // menu); no dim
        modal: true
        dim: false
        closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside

        onClosed: {
            const chosen = root._chosen
            root._chosen = null

            if (root._focusItem)
                root._focusItem.forceActiveFocus(Qt.PopupFocusReason)
            root._focusItem = null

            if (chosen)
                chosen.action(root._dataList, root._options, root._indexes)
        }
    }

    readonly property Component _rowComponent: Component {
        RedesignMenuItem {
            property var entry: null

            onTriggered: root._chosen = entry
        }
    }

    readonly property Component _separatorComponent: Component {
        RedesignMenuSeparator { }
    }
}

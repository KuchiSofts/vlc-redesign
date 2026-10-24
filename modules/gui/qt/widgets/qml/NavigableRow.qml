/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

import QtQuick
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.Util

T.Container  {
    id: root

    // Properties

    property int indexFocus: -1

    property int _countEnabled: 0

    // Aliases

    property alias delegate: repeater.delegate

    // Settings

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    Navigation.navigable: (_countEnabled > 0)

    // Events

    onIndexFocusChanged: if (_hasFocus()) _applyFocus()

    onActiveFocusChanged: {
        // NOTE: We try to restore the preferred focused item.
        if (!activeFocus || _applyFocus())
            return;

        // Next item
        if (focusReason === Qt.TabFocusReason) {
            for (let i = 0; i < count; i++) {
                const item = repeater.itemAt(i);

                if (root._isNavigable(item)) {
                    Helpers.transferFocus(item, Qt.TabFocusReason)

                    return;
                }
            }
        }
        // Previous item
        else if (focusReason === Qt.BacktabFocusReason) {
            for (let i = count -1; i >= 0; i--) {
                const item = repeater.itemAt(i);

                if (root._isNavigable(item)) {
                    Helpers.transferFocus(item, Qt.BacktabFocusReason)

                    return;
                }
            }
        }
        // NOTE: We make sure that one item has the focus.
        else {
            let itemFocus = undefined;

            for (let i = 0 ; i < count; i++) {
                const item = repeater.itemAt(i);

                if (root._isNavigable(item)) {
                    // NOTE: We already have a focused item, so we keep it this way.
                    if (item.activeFocus)
                        return;

                    if (itemFocus == undefined)
                        itemFocus = item;
                }
            }

            if (itemFocus)
                Helpers.transferFocus(itemFocus, focusReason)
        }
    }

    // Keys

    Keys.priority: Keys.AfterItem

    Keys.onPressed: (event) => root.Navigation.defaultKeyAction(event)

    // Functions

    function _isNavigable(item) {
        return item.visible && item.enabled && item.Navigation.navigable
    }

    function _applyFocus() {
        if (indexFocus < 0 || indexFocus >= count) return false;

        const item = repeater.itemAt(indexFocus);

        if (root._isNavigable(item)) {
            Helpers.transferFocus(itemFocus, focusReason)

            return true;
        }

        return false;
    }

    function _hasFocus() {
        for (let i = 0 ; i < count; i++) {
            if (repeater.itemAt(i).activeFocus)
                return true;
        }

        return false;
    }

    // Childs

    contentItem: Row {
        spacing: root.spacing

        Repeater{
            id: repeater

            model: root.contentModel

            onItemAdded: (index, item) => {
                if (item.enabled) root._countEnabled += 1;

                item.onEnabledChanged.connect(() => {
                    if(root && item)
                        root._countEnabled += (item.enabled ? 1 : -1);
                });

                item.Navigation.parentItem = root;

                // the item on the left: the next one in a right to left
                // row (Hebrew, Arabic...), the previous one otherwise
                item.Navigation.leftAction = function() {
                    const step = root.mirrored ? 1 : -1;
                    let i = index;

                    do {
                        i += step;
                    } while (i >= 0 && i < count
                             &&
                             !root._isNavigable(repeater.itemAt(i)));

                    if (i < 0 || i >= count)
                        root.Navigation.defaultNavigationLeft();
                    else
                        Helpers.transferFocus(repeater.itemAt(i), root.mirrored ? Qt.TabFocusReason : Qt.BacktabFocusReason)
                };

                item.Navigation.rightAction = function() {
                    const step = root.mirrored ? -1 : 1;
                    let i = index;

                    do {
                        i += step;
                    } while (i >= 0 && i < count
                             &&
                             !root._isNavigable(repeater.itemAt(i)));

                    if (i < 0 || i >= count)
                        root.Navigation.defaultNavigationRight();
                    else
                        Helpers.transferFocus(repeater.itemAt(i), root.mirrored ? Qt.BacktabFocusReason : Qt.TabFocusReason)
                };
            }

            onItemRemoved: (index, item) => {
                if (item.enabled) root._countEnabled -= 1
            }
        }
    }
}

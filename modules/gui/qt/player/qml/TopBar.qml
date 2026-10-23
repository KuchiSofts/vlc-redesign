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
import QtQuick.Layouts
import QtQuick.Window


import VLC.MainInterface
import VLC.Style
import VLC.PlayerControls
import VLC.Playlist
import VLC.Widgets as Widgets
import VLC.Menus as Menus
import VLC.Util

FocusScope{
    id: root

    // Properties

    required property int textWidth

    // For now, used for d&d functionality
    // Not strictly necessary to set
    property Item plListView

    property int reservedHeight: 0

    property int topMargin: 0
    property int sideMargin: 0

    property string title

    property bool showCSD: false
    property bool showToolbar: false
    property bool pinControls: false

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        palette: VLCStyle.palette
        colorSet: ColorContext.Window
    }

    // Private

    property bool _showTopBar: (pinControls === false || showToolbar === false)

    property bool _showCenterText: showToolbar === false

    readonly property int _sideMargin: VLCStyle.margin_small + sideMargin

    // The resume prompt is next to the playback controls (Player.qml)
    readonly property bool resumeVisible: false

    property bool playlistVisible: false
    property bool libraryVisible: false
    // the icon of the library filled: the panel shows the library (Figma
    // 187:328), not a folder (the folder button, Figma "Player - Light")
    property bool libraryActive: libraryVisible

    // the layout of the control bar shown under it (video or music player)
    property int barIdentifier: Player.hasVideoOutput ? PlayerControlbarModel.Videoplayer
                                                      : PlayerControlbarModel.Audioplayer

    // Figma "Player - customize": the items moved into the control bar of
    // the player leave the top bar
    readonly property PlayerControlbarModel _barModel: MainCtx.controlbarProfileModel.currentModel
        ? MainCtx.controlbarProfileModel.currentModel.getModel(root.barIdentifier) : null
    readonly property int _barRevision: _barModel ? _barModel.left.count + 100 * _barModel.center.count
                                                    + 10000 * _barModel.right.count : 0
    function _inBar(control) {
        void root._barRevision
        return !!root._barModel && (root._barModel.left.contains(control)
                                    || root._barModel.center.contains(control)
                                    || root._barModel.right.contains(control))
    }
    readonly property bool _menuInBar: { void _barRevision; return _inBar(ControlListModel.MENU_BUTTON) }
    readonly property bool _libraryInBar: { void _barRevision; return _inBar(ControlListModel.BACK_BUTTON) }
    readonly property bool _folderInBar: { void _barRevision; return _inBar(ControlListModel.FOLDER_NAME_WIDGET) }
    readonly property bool _titleInBar: { void _barRevision; return _inBar(ControlListModel.FILE_NAME_WIDGET) }
    readonly property bool _fullscreenInBar: { void _barRevision; return _inBar(ControlListModel.FULLSCREEN_BUTTON) }

    // Signals

    signal togglePlaylistVisibility()
    signal requestLockUnlockAutoHide(bool lock)
    signal backRequested()
    // Library next to the video, see CompactLibrary
    signal libraryToggled()
    signal folderRequested()

    ///incomming signal to request all menu and popup to close
    signal forceUnlock()

    // Settings

    Accessible.role: Accessible.ToolBar
    Accessible.name: qsTr("Player topbar")
    AccessibleCompat.id: "playerTopBar"

    // Events

    Component.onCompleted: _layout()

    onShowCSDChanged: _layout()
    onPinControlsChanged: _layout()
    onShowToolbarChanged: _layout()
    onTopMarginChanged: _layout()
    onSideMarginChanged: _layout()

    on_ShowCenterTextChanged: _layout()

    // Functions

    // FIXME: Imperative layouting in QML is stupid, this needs to be gone.
    function _layoutLine(c1, c2, offset)
    {
        let c1Height = c1?.implicitHeight ?? 0
        let c2Height = c2?.implicitHeight ?? 0

        if (c2 === csdDecorations) {
            //csdDecorations.implicitHeight gets overwritten when the height is set,
            //VLCStyle.redesign.topbarHeight is its initial value
            c2Height = VLCStyle.redesign.topbarHeight
        }

        const lineHeight = Math.max(c1Height, c2Height)

        if (c1) {
            c1.height = lineHeight
            c1.anchors.topMargin = offset
        }

        if (c2) {
            c2.height = lineHeight
            c2.anchors.topMargin = offset
        }
        return lineHeight
    }

    // FIXME: Imperative layouting in QML is stupid, this needs to be gone.
    //FIXME: if CSD will be weirdly placed if application safe-area are used,
    //nota that if you need a safe area (kiosk mode), you probably don't need CSD
    function _layout() {
        let offset = topMargin

        if (_showCenterText) {
            //place everything on one line
            //csdDecorations.implicitHeight gets overwritten when the height is set,
            //VLCStyle.redesign.topbarHeight is its initial value
            const lineHeight = Math.max(VLCStyle.redesign.topbarHeight, logoOrResume.implicitHeight)

            centerTitleText.y = topMargin
            centerTitleText.height = lineHeight

            csdDecorations.height  = lineHeight
            csdDecorations.anchors.topMargin = topMargin

            logoOrResume.height = lineHeight
            logoOrResume.anchors.topMargin = topMargin


            implicitHeight = lineHeight
            offset += lineHeight

        } else {
            let left = undefined
            let right = undefined
            let logoPlaced = false

            if (showToolbar) {
                left = menubar
            }

            if (showCSD) {
                right = csdDecorations
                if (!left) {
                    left = logoOrResume
                    logoPlaced = true
                }
            }

            if (!!left || !!right) {
                offset += _layoutLine(left, right, offset)

                if (showCSD) {
                    tapNDrag.height = offset
                }
            }

            if (!logoPlaced) {
                left = logoOrResume
            } else {
                left = undefined
            }

            right = undefined

            offset += _layoutLine(left, right, offset)
        }

        implicitHeight = offset
        reservedHeight = offset
    }

    // Children

    Rectangle {
        anchors.fill: parent
        z: -0.5
        color: VLCStyle.redesign.color(VLCStyle.redesign.playerTopbarBackground, "#ababaa", VLCStyle.palette.isDark)
    }

    //drag and dbl click the titlebar in CSD mode
    Loader {
        id: tapNDrag

        anchors.fill: parent
        active: root.showCSD
        source: "qrc:///qt/qml/VLC/Widgets/CSDTitlebarTapNDrapHandler.qml"

        Connections {
            target: tapNDrag.item
            enabled: tapNDrag.status === Loader.Ready
            function onCsdMenuVisibleChanged() {
                root.requestLockUnlockAutoHide(tapNDrag.item.csdMenuVisible)
            }
        }
    }

    // Components -
    Menus.Menubar {
        id: menubar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: root.sideMargin
        anchors.right: root.showCSD ? csdDecorations.left : parent.right


        visible: root.showToolbar
        enabled: root.showToolbar

        onHoveredChanged: menu => root.requestLockUnlockAutoHide(hovered)
        onMenuOpenedChanged: menu => root.requestLockUnlockAutoHide(menuOpened)
        onImplicitHeightChanged: root._layout()
    }

    Item {
        id: logoOrResume

        anchors.left: root.left
        anchors.top: root.top
        // the 6 px padding of the topbar of the design
        anchors.leftMargin: root.sideMargin + VLCStyle.dp(6, VLCStyle.scale)

        implicitWidth: logoGroup.implicitWidth

        implicitHeight: _showTopBar ? logoGroup.implicitHeight : 0

        onImplicitHeightChanged: root._layout()

        // Figma "Player" topbar (padding 6): the cone (an icon in a 38 x 28
        // frame, 7 px in), 7 px, the ⋯ menu and the library sidebar (icons
        // 28 x 28, 10 px apart), the divider icon and the folder of the
        // media (its icon 28 x 28, 4 px, its name, 4 px)
        Row {
            id: logoGroup

            anchors.fill: parent

            visible: root._showTopBar

            Item {
                anchors.verticalCenter: parent.verticalCenter
                width: VLCStyle.dp(38, VLCStyle.scale)
                height: VLCStyle.dp(28, VLCStyle.scale)

                Widgets.BannerCone {
                    id: logo
                    source: "qrc:///redesign/logo.svg"

                    x: VLCStyle.dp(7, VLCStyle.scale)
                    anchors.verticalCenter: parent.verticalCenter
                    width: VLCStyle.redesign.iconSize
                    height: VLCStyle.redesign.iconSize

                    color: VLCStyle.redesign.logoColor
                }
            }

            Item {
                width: VLCStyle.dp(7, VLCStyle.scale)
                height: 1
            }

            TopBarButton {
                id: menuButton

                iconName: "more"
                description: qsTr("Menu")
                AccessibleCompat.id: "topBarMenuButton"

                focus: true
                Navigation.parentItem: root
                Navigation.rightItem: libraryButton
                onHoveredChanged: root.requestLockUnlockAutoHide(hovered)

                onClicked: mainMenu.toggleFrom(menuButton, 0, menuButton.height)
                visible: !root._menuInBar

                Widgets.DashedRect {
                    anchors.fill: parent
                    visible: VLCStyle.customizing
                    color: VLCStyle.redesign.color("#6B6B6B", "#5a5a59")
                }
                Widgets.CustomizeDragArea {
                    controlId: ControlListModel.MENU_BUTTON
                    iconSource: "qrc:///redesign/icons/more/default.png"
                }

                Menus.MainMenu {
                    id: mainMenu
                    onOpened: root.requestLockUnlockAutoHide(true)
                    onClosed: root.requestLockUnlockAutoHide(false)
                }
            }

            Item {
                width: VLCStyle.dp(10, VLCStyle.scale)
                height: 1
                visible: menuButton.visible && libraryButton.visible
            }

            TopBarButton {
                id: libraryButton

                iconName: "sidebar"
                // the panel of the icon filled while the library is shown
                iconState: root.libraryActive ? "active" : "default"
                description: qsTr("Library")
                AccessibleCompat.id: "topBarLibraryButton"

                Navigation.parentItem: root
                Navigation.leftItem: menuButton
                Navigation.rightItem: folderButton
                onHoveredChanged: root.requestLockUnlockAutoHide(hovered)

                onClicked: root.libraryToggled()
                visible: !root._libraryInBar

                Widgets.DashedRect {
                    anchors.fill: parent
                    visible: VLCStyle.customizing
                    color: VLCStyle.redesign.color("#6B6B6B", "#5a5a59")
                }
                Widgets.CustomizeDragArea {
                    controlId: ControlListModel.BACK_BUTTON
                    iconSource: "qrc:///redesign/icons/sidebar/default.png"
                }
            }

            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: VLCStyle.redesign.iconSize
                height: width
                sourceSize: Qt.size(width, height)
                source: "qrc:///redesign/icons/divider/default.png"
                visible: folderButton.visible && (menuButton.visible || libraryButton.visible)

                Widgets.DashedRect {
                    anchors.centerIn: parent
                    width: parent.width
                    height: VLCStyle.dp(28, VLCStyle.scale)
                    visible: VLCStyle.customizing
                    color: VLCStyle.redesign.color("#6B6B6B", "#5a5a59")
                }
            }

            Widgets.IconToolButton {
                id: folderButton

                // Parent folder of a local media: the name users recognize
                readonly property string folderName: {
                    const url = Player.url.toString()
                    if (!url.startsWith("file://"))
                        return ""
                    const parts = decodeURIComponent(url).split("/")
                    return parts.length > 1 ? parts[parts.length - 2] : ""
                }

                anchors.verticalCenter: parent.verticalCenter
                visible: folderName !== "" && !root._folderInBar
                width: folderText.x + Math.ceil(folderText.implicitWidth) + VLCStyle.dp(4, VLCStyle.scale)
                height: VLCStyle.dp(28, VLCStyle.scale)

                text: description
                description: qsTr("Show %1 in the library").arg(folderName)
                AccessibleCompat.id: "topBarFolderButton"

                backgroundColor: VLCStyle.redesign.color(hovered || down ? VLCStyle.redesign.controlHoverBackground : "transparent",
                                                         colorContext.bg.primary)
                radius: VLCStyle.redesign.controlRadius

                contentItem: Item {
                    Image {
                        x: VLCStyle.dp(2, VLCStyle.scale)
                        anchors.verticalCenter: parent.verticalCenter
                        width: VLCStyle.redesign.iconSize
                        height: width
                        sourceSize: Qt.size(width, height)
                        source: "qrc:///redesign/icons/folder/default.png"
                    }
                    Text {
                        id: folderText
                        x: VLCStyle.dp(32, VLCStyle.scale)
                        // the text box of the design centred in the button,
                        // on a whole pixel as Figma draws it
                        anchors.baseline: parent.top
                        anchors.baselineOffset: VLCStyle.redesign.textBaseline(
                            Math.round((parent.height - Math.round(1.2109375 * font.pixelSize)) / 2), font.pixelSize)
                        text: folderButton.folderName
                        color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, theme.fg.primary)
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.redesign.fontNormal
                    }
                }

                Navigation.parentItem: root
                Navigation.leftItem: libraryButton

                onClicked: root.folderRequested()
                onHoveredChanged: root.requestLockUnlockAutoHide(hovered)

                Widgets.DashedRect {
                    anchors.fill: parent
                    visible: VLCStyle.customizing
                    color: VLCStyle.redesign.color("#6B6B6B", "#5a5a59")
                }
                Widgets.CustomizeDragArea {
                    controlId: ControlListModel.FOLDER_NAME_WIDGET
                    iconSource: "qrc:///redesign/icons/folder/default.png"
                }
            }
        }
    }

    Title {
        id: centerTitleText
        centerWithinBounds: true
        leftLimit: logoOrResume.x + logoOrResume.width
        rightLimit: root.showCSD ? csdDecorations.x : root.width
        visible: root._showCenterText && !root.resumeVisible && !root._titleInBar
        enabled: visible
        text: root.title || (Player.isStarted && Player.name.length ? Player.name : qsTr("VLC Media Player"))
        Accessible.name: root.title || text
        colorContext.palette: theme.palette

        // The text box of the design (17 px for 14 px) centred in the bar, on
        // a whole pixel as Figma draws it
        verticalAlignment: Text.AlignTop
        topPadding: VLCStyle.redesign.textBaseline(Math.round((height - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                   font.pixelSize) - titleMetrics.ascent
        FontMetrics {
            id: titleMetrics
            font: centerTitleText.font
        }

        // Figma "Player - customize": the text outlined, 8 around, 28 high
        Widgets.DashedRect {
            readonly property real textX: centerTitleText.horizontalAlignment === Text.AlignHCenter
                                           ? (parent.width - centerTitleText.contentWidth) / 2 : 0
            x: Math.round(textX - VLCStyle.dp(8, VLCStyle.scale))
            y: Math.round((parent.height - height) / 2)
            width: Math.round(centerTitleText.contentWidth + VLCStyle.dp(16, VLCStyle.scale))
            height: VLCStyle.dp(28, VLCStyle.scale)
            visible: VLCStyle.customizing
            color: VLCStyle.redesign.color("#6B6B6B", "#5a5a59")

            Widgets.CustomizeDragArea {
                controlId: ControlListModel.FILE_NAME_WIDGET
                iconSource: "qrc:///redesign/icons/file/default.png"
            }
        }
    }

    Title {
        id: leftTitleText
        anchors.left: parent.left
        anchors.top: logoOrResume.bottom
        anchors.leftMargin: root._sideMargin
        width: root.textWidth - VLCStyle.margin_normal
        visible: !root.pinControls && !root._showCenterText
        enabled: visible
        topPadding: VLCStyle.margin_large
        leftPadding: logo.x
        text: root.title
        colorContext.palette: theme.palette
    }

    Loader {
        id: csdDecorations

        anchors.top: parent.top
        anchors.right: parent.right
        // the 6 px padding of the topbar of the design
        anchors.rightMargin: VLCStyle.dp(6, VLCStyle.scale)

        focus: false
        height: VLCStyle.redesign.topbarHeight
        active: root.showCSD
        enabled: root.showCSD
        visible: root.showCSD
        source: "qrc:///qt/qml/VLC/Widgets/CSDWindowButtonSet.qml"

        Binding {
            target: csdDecorations.item
            when: !!csdDecorations.item
            property: "fullscreenHidden"
            value: root._fullscreenInBar
        }
        Binding {
            target: csdDecorations.item
            when: !!csdDecorations.item
            property: "fullscreenControlId"
            value: ControlListModel.FULLSCREEN_BUTTON
        }

        Connections {
            target: csdDecorations.item
            enabled: csdDecorations.status === Loader.Ready
            function onButtonHoveredChanged() { root.requestLockUnlockAutoHide(csdDecorations.item.buttonHovered) }
        }
    }

    // Icon button of the top bar showing an icon of pixmaps/redesign/icons
    // Figma "Player - customize": the items of the bar dropped on the top bar
    // go back there (after the drop: their zone may empty)
    DropArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: VLCStyle.redesign.topbarHeight
        enabled: VLCStyle.customizing
        onEntered: (drag) => {
            const source = drag.source
            drag.accepted = !!source && source.objectName === "playerControl"
                            && [ControlListModel.MENU_BUTTON, ControlListModel.BACK_BUTTON,
                                ControlListModel.FOLDER_NAME_WIDGET, ControlListModel.FILE_NAME_WIDGET,
                                ControlListModel.FULLSCREEN_BUTTON].indexOf(source.controlId) >= 0
        }
        onDropped: (drop) => {
            const zone = drop.source.zoneModel
            const row = drop.source.sourceRow
            if (!zone)
                return
            Qt.callLater(() => {
                if (row >= 0 && row < zone.count)
                    zone.remove(row)
            })
            drop.accept(Qt.MoveAction)
        }
    }

    component TopBarButton: Widgets.IconToolButton {
        id: topBarButton

        property string iconName
        property string iconState: "default"

        // Drawn from an image, the text only names the button
        text: description
        anchors.verticalCenter: parent?.verticalCenter
        width: VLCStyle.dp(28, VLCStyle.scale)
        height: VLCStyle.dp(28, VLCStyle.scale)

        radius: VLCStyle.redesign.controlRadius
        backgroundColor: VLCStyle.redesign.color(hovered || down ? VLCStyle.redesign.controlHoverBackground
                                                                 : "transparent", "#9c9c9b")

        contentItem: Image {
            source: "qrc:///redesign/icons/" + topBarButton.iconName + "/" + topBarButton.iconState + ".png"
            sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
            fillMode: Image.Pad
        }
    }
}

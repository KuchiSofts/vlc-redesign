/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Controls
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.Player
import VLC.Widgets as Widgets
import VLC.Style
import VLC.Dialogs

Controls.Popup {
    id: root
    objectName: "toolbarEditorDialog"
    parent: Controls.Overlay.overlay
    // Figma "Add Subtitles to Playlist/Folder" (the dialog of the design):
    // 1048 x 655, 23 above the middle. Only the panel: a popup takes every
    // press inside its bounds, and the player under it is edited in place
    // (its controls dragged around, Figma "Player - customize")
    width: Math.min(VLCStyle.dp(1048, VLCStyle.scale), (parent ? parent.width : 0) - VLCStyle.dp(32, VLCStyle.scale))
    height: Math.min(VLCStyle.dp(94 + 489 + 72, VLCStyle.scale), (parent ? parent.height : 0) - VLCStyle.dp(32, VLCStyle.scale))
    x: Math.round(((parent ? parent.width : 0) - width) / 2)
    y: Math.max(VLCStyle.dp(16, VLCStyle.scale),
                Math.round(((parent ? parent.height : 0) - height) / 2 - VLCStyle.dp(23, VLCStyle.scale)))
    margins: 0
    padding: 0
    modal: false
    dim: false
    focus: true
    closePolicy: Controls.Popup.CloseOnEscape
    background: Item {}

    signal unload()
    property bool saved: false
    property bool previousGlobalShortcuts: true
    // Held by the content item: the popup is not an item, the contexts of
    // the controls (and of the editor views reparented here) look for one
    // up their parent items, and without one they draw in magenta
    readonly property ColorContext colorContext: scene.colorContext

    onOpened: {
        VLCStyle.customizing = true
        saved = false
        previousGlobalShortcuts = MainCtx.useGlobalShortcuts
        MainCtx.useGlobalShortcuts = false
        // Retain the current profile as the rollback point for Cancel/Escape.
        MainCtx.controlbarProfileModel.save(false)
        MainCtx.controlbarProfileModel.editing = true
        closeButton.forceActiveFocus(Qt.PopupFocusReason)
        // "New profile" of a menu opened the editor: the name first
        if (MainCtx.controlbarProfileModel.takeNewProfileRequest())
            namePrompt.openFor("new")
    }
    onVisibleChanged: if (!visible && opened) close()
    onClosed: {
        // before the shortcuts come back: the prompts give them back too
        namePrompt.close()
        deleteConfirmation.close()
        VLCStyle.customizing = false
        MainCtx.controlbarProfileModel.editing = false
        if (!saved)
            MainCtx.controlbarProfileModel.reload()
        MainCtx.useGlobalShortcuts = previousGlobalShortcuts
        // opened from the library: back to it (unless something plays now)
        if (VLCStyle.customizeFromLibrary) {
            VLCStyle.customizeFromLibrary = false
            if (Player.playingState === Player.PLAYING_STATE_STOPPED)
                MainCtx.playerView = false
        }
        unload()
    }

    function saveEditor() {
        if (!MainCtx.controlbarProfileModel.save())
            return
        saved = true
        close()
    }

    // What New, Rename and Clone do with the name given: the profile made
    // (or renamed) is the one edited, kept by Save, dropped by Cancel
    function applyName(mode, name) {
        const model = MainCtx.controlbarProfileModel
        if (mode === "rename")
            return model.renameSelectedProfile(name)
        if (mode === "clone")
            return model.cloneSelectedProfile(name) >= 0
        return model.createProfile(name) >= 0
    }

    function markedName(name, dirty) { return dirty ? name + " *" : name }

    contentItem: Item {
        id: scene

        readonly property ColorContext colorContext: ColorContext {
            palette: VLCStyle.palette
            colorSet: ColorContext.Window
        }

        // This strip sits over the actual player's control bar. Its editor
        // views are reparented here; no static screenshot or parallel model.
        Rectangle {
            id: playerDropHost
            objectName: "toolbarEditorPlayerDropHost"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            // the bar of the player is edited in place: no strip
            visible: false
            height: 0
            color: VLCStyle.redesign.color("#1b1b1b", root.colorContext.bg.primary)
            border.color: VLCStyle.redesign.color("#343434", root.colorContext.border)
            border.width: 1
        }

        // the panel: the whole popup
        Item {
            id: panel
            objectName: "toolbarEditorPanel"
            anchors.fill: parent

            readonly property real radius: VLCStyle.dp(14, VLCStyle.scale)

            MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons }

            // #0E0E0E 95 %, shadow 0 8 blur 15 #000000 46 %
            Rectangle {
                anchors.fill: parent
                radius: panel.radius
                color: VLCStyle.redesign.color(Qt.rgba(14 / 255, 14 / 255, 14 / 255, 0.95), root.colorContext.bg.primary)

                Widgets.RoundedRectangleShadow {
                    yOffset: VLCStyle.dp(8, VLCStyle.scale)
                    blurRadius: 7.5 * VLCStyle.scale // (dp() rounds to whole pixels)
                    color: Qt.rgba(0, 0, 0, 0.46)
                }
            }

            // "Frame 120": #090909, radius 14 on top, padding 9, gap 12
            Rectangle {
                id: head
                width: parent.width
                height: VLCStyle.dp(94, VLCStyle.scale)
                radius: panel.radius
                color: VLCStyle.redesign.color("#090909", root.colorContext.bg.secondary)
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: panel.radius
                    color: parent.color
                }

                // the title and the close button
                Item {
                    x: VLCStyle.dp(9, VLCStyle.scale)
                    y: VLCStyle.dp(9, VLCStyle.scale)
                    width: parent.width - 2 * x
                    height: VLCStyle.dp(32, VLCStyle.scale)

                    Text {
                        id: titleText
                        x: VLCStyle.dp(10, VLCStyle.scale)
                        width: Math.max(0, profileControls.x - x - VLCStyle.dp(16, VLCStyle.scale))
                        height: parent.height
                        elide: Text.ElideRight
                        text: qsTr("Customize Player")
                        color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, root.colorContext.fg.primary)
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                        verticalAlignment: Text.AlignTop
                        topPadding: VLCStyle.redesign.textBaseline(Math.round((height - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                                   font.pixelSize) - titleMetrics.ascent
                        FontMetrics {
                            id: titleMetrics
                            font: titleText.font
                        }
                    }

                    // The profile edited (the controls of the settings: the
                    // dropdown, then New, Rename, Clone and Delete), between
                    // the title and the close button; narrower when the room
                    // after the title is short
                    Row {
                        id: profileControls

                        readonly property real gap: VLCStyle.dp(16, VLCStyle.scale)
                        readonly property real room: closeButton.x - gap - titleText.x - titleText.implicitWidth - gap
                        readonly property real buttonWidth: Math.max(VLCStyle.dp(64, VLCStyle.scale),
                                                                     Math.min(VLCStyle.dp(100, VLCStyle.scale),
                                                                              (room - VLCStyle.dp(160, VLCStyle.scale) - 4 * spacing) / 4))
                        readonly property real pickerWidth: Math.max(VLCStyle.dp(120, VLCStyle.scale),
                                                                     Math.min(VLCStyle.dp(240, VLCStyle.scale),
                                                                              room - 4 * buttonWidth - 4 * spacing))

                        anchors.right: closeButton.left
                        anchors.rightMargin: gap
                        height: parent.height
                        spacing: VLCStyle.dp(8, VLCStyle.scale)

                        SettingsDropdown {
                            id: profilePicker
                            objectName: "toolbarEditorProfilePicker"
                            width: profileControls.pickerWidth
                            height: parent.height
                            model: MainCtx.controlbarProfileModel
                            textRole: "name"
                            currentIndex: MainCtx.controlbarProfileModel.selectedProfile
                            // follows a rename (the profile names itself)
                            displayText: MainCtx.controlbarProfileModel.currentModel
                                         ? MainCtx.controlbarProfileModel.currentModel.name : ""
                            Accessible.name: qsTr("Profile")
                            onActivated: (index) => {
                                if (index !== MainCtx.controlbarProfileModel.selectedProfile)
                                    MainCtx.controlbarProfileModel.setSelectedProfile(index)
                                // the selected profile, whether it changed or not
                                currentIndex = Qt.binding(() => MainCtx.controlbarProfileModel.selectedProfile)
                            }

                            // the rows of the profiles (a list model: its roles)
                            delegate: T.ItemDelegate {
                                required property string name
                                required property int index

                                width: ListView.view ? ListView.view.width : profilePicker.width
                                implicitHeight: profilePicker.px(32)
                                leftPadding: profilePicker.px(14)
                                rightPadding: profilePicker.px(14)
                                highlighted: profilePicker.highlightedIndex === index

                                contentItem: Text {
                                    id: profileOptionText
                                    text: name
                                    color: index === MainCtx.controlbarProfileModel.selectedProfile
                                           ? VLCStyle.redesign.accent : profilePicker.c("#8F8F8F")
                                    font: profilePicker.font
                                    verticalAlignment: Text.AlignTop
                                    topPadding: height / 2 + VLCStyle.redesign.centeredBaseline(font.pixelSize)
                                                - profileOptionMetrics.ascent
                                    FontMetrics { id: profileOptionMetrics; font: profileOptionText.font }
                                    elide: Text.ElideRight
                                }
                                background: Rectangle {
                                    radius: profilePicker.px(4)
                                    color: parent.highlighted || parent.hovered ? profilePicker.c("#222222") : "transparent"
                                }
                            }
                        }

                        SettingsButton {
                            objectName: "toolbarEditorNewProfile"
                            width: profileControls.buttonWidth
                            height: parent.height
                            text: qsTr("New")
                            Accessible.name: qsTr("New profile")
                            onClicked: namePrompt.openFor("new")
                        }
                        SettingsButton {
                            objectName: "toolbarEditorRenameProfile"
                            width: profileControls.buttonWidth
                            height: parent.height
                            enabled: !!MainCtx.controlbarProfileModel.currentModel
                            text: qsTr("Rename")
                            Accessible.name: qsTr("Rename the profile")
                            onClicked: namePrompt.openFor("rename")
                        }
                        SettingsButton {
                            objectName: "toolbarEditorCloneProfile"
                            width: profileControls.buttonWidth
                            height: parent.height
                            enabled: !!MainCtx.controlbarProfileModel.currentModel
                            text: qsTr("Clone")
                            Accessible.name: qsTr("Clone the profile")
                            onClicked: namePrompt.openFor("clone")
                        }
                        // the last profile stays
                        SettingsButton {
                            objectName: "toolbarEditorDeleteProfile"
                            width: profileControls.buttonWidth
                            height: parent.height
                            enabled: MainCtx.controlbarProfileModel.count > 1
                            destructive: true
                            text: qsTr("Delete")
                            Accessible.name: qsTr("Delete the profile")
                            onClicked: deleteConfirmation.open()
                        }
                    }

                    T.Button {
                        id: closeButton
                        objectName: "toolbarEditorClose"
                        anchors.right: parent.right
                        width: VLCStyle.dp(32, VLCStyle.scale)
                        height: VLCStyle.dp(32, VLCStyle.scale)
                        padding: VLCStyle.dp(4, VLCStyle.scale)
                        Accessible.name: qsTr("Cancel and close")
                        onClicked: root.close()
                        background: Rectangle {
                            radius: VLCStyle.dp(4, VLCStyle.scale)
                            color: (closeButton.hovered || closeButton.down)
                                   ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
                                   : "transparent"
                            border.width: closeButton.visualFocus ? 1 : 0
                            border.color: VLCStyle.redesign.focusColor
                        }
                        contentItem: Image {
                            source: "qrc:///redesign/icons/close/default.png"
                            sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                            fillMode: Image.Pad
                            smooth: true
                        }
                    }
                }

                // the description
                Text {
                    id: descriptionText
                    x: VLCStyle.dp(9 + 10, VLCStyle.scale)
                    y: VLCStyle.dp(9 + 32 + 12, VLCStyle.scale)
                    width: parent.width - x - VLCStyle.dp(9, VLCStyle.scale)
                    height: VLCStyle.dp(32, VLCStyle.scale)
                    text: qsTr("Drag items around and out of the box below to re-order them. Drag items into the box to remove them from the player.")
                    elide: Text.ElideRight
                    color: VLCStyle.redesign.color("#6B6B6B", root.colorContext.fg.secondary)
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    verticalAlignment: Text.AlignTop
                    topPadding: VLCStyle.redesign.textBaseline(Math.round((height - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                               font.pixelSize) - descriptionMetrics.ascent
                    FontMetrics {
                        id: descriptionMetrics
                        font: descriptionText.font
                    }
                }
            }

            // "Frame 124": the palette, padding 34
            ToolbarEditor {
                id: toolbarEditor
                objectName: "toolbarEditorContents"
                x: VLCStyle.dp(34, VLCStyle.scale)
                y: head.height + VLCStyle.dp(34, VLCStyle.scale)
                width: parent.width - 2 * x
                height: parent.height - y - foot.height - VLCStyle.dp(34, VLCStyle.scale)
                previewHost: playerDropHost
            }

            // "Frame 225": padding 20, Cancel and Save
            Item {
                id: foot
                anchors.bottom: parent.bottom
                width: parent.width
                height: VLCStyle.dp(72, VLCStyle.scale)

                readonly property bool changed: !!MainCtx.controlbarProfileModel.currentModel
                                                && MainCtx.controlbarProfileModel.currentModel.dirty

                T.Button {
                    id: cancelButton
                    objectName: "toolbarEditorCancel"
                    x: VLCStyle.dp(20, VLCStyle.scale)
                    y: VLCStyle.dp(20, VLCStyle.scale)
                    width: VLCStyle.dp(100, VLCStyle.scale)
                    height: VLCStyle.dp(32, VLCStyle.scale)
                    text: qsTr("Cancel")
                    activeFocusOnTab: true
                    onClicked: root.close()
                    background: Rectangle {
                        radius: VLCStyle.dp(6, VLCStyle.scale)
                        color: cancelButton.down || cancelButton.hovered
                               ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
                               : VLCStyle.redesign.color(VLCStyle.redesign.playerControlBackground, "#ababaa")
                        border.width: cancelButton.visualFocus ? 1 : 0
                        border.color: VLCStyle.redesign.focusColor
                    }
                    contentItem: Text {
                        text: cancelButton.text
                        color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#4d4d4d")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Text {
                    anchors.left: cancelButton.right
                    anchors.leftMargin: VLCStyle.dp(16, VLCStyle.scale)
                    anchors.right: saveButton.left
                    anchors.rightMargin: VLCStyle.dp(16, VLCStyle.scale)
                    anchors.verticalCenter: cancelButton.verticalCenter
                    visible: !!MainCtx.controlbarProfileModel.lastError
                    text: MainCtx.controlbarProfileModel.lastError
                    color: "#e4a580"
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.redesign.fontSmall
                    elide: Text.ElideRight
                    Accessible.role: Accessible.AlertMessage
                }

                // Figma: #371F06 and the accent; until something changed, at
                // half its luminosity (the greys it gives over the dialog)
                T.Button {
                    id: saveButton
                    objectName: "toolbarEditorSave"
                    x: parent.width - width - VLCStyle.dp(20, VLCStyle.scale)
                    y: VLCStyle.dp(20, VLCStyle.scale)
                    width: VLCStyle.dp(100, VLCStyle.scale)
                    height: VLCStyle.dp(32, VLCStyle.scale)
                    text: qsTr("Save")
                    // a layout changed, or profiles added, renamed, deleted
                    // or chosen
                    enabled: foot.changed || MainCtx.controlbarProfileModel.modified
                    activeFocusOnTab: true
                    onClicked: root.saveEditor()
                    background: Rectangle {
                        radius: VLCStyle.dp(6, VLCStyle.scale)
                        color: !saveButton.enabled ? VLCStyle.redesign.color("#191919", "#b5b5b4")
                               : saveButton.down ? Qt.lighter(VLCStyle.redesign.accentBackground, 1.15)
                                                 : VLCStyle.redesign.accentBackground
                        border.width: saveButton.visualFocus ? 1 : 0
                        border.color: VLCStyle.redesign.focusColor
                    }
                    contentItem: Text {
                        text: saveButton.text
                        color: saveButton.enabled ? VLCStyle.redesign.accent
                                                  : VLCStyle.redesign.color("#4C4C4C", "#8e8e8d")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        // "New profile" of a menu while the editor is open
        Connections {
            target: MainCtx.controlbarProfileModel
            function onNewProfileRequested() {
                if (root.opened && MainCtx.controlbarProfileModel.takeNewProfileRequest())
                    namePrompt.openFor("new")
            }
        }

        // New, Rename and Clone of the header: the name of the profile, in the
        // dialog of the design that names things (Figma "new playlist": a
        // heading, then the field of the dialogs)
        Widgets.RedesignDialog {
            id: namePrompt
            objectName: "toolbarEditorProfileName"

            // "new", "rename" or "clone"
            property string mode: "new"
            // what went wrong making the profile
            property string failure: ""
            // one line, whatever was pasted
            readonly property string name: nameField.text.replace(/\s+/g, " ").trim()
            readonly property string nameError: name.length > 0
                                                ? MainCtx.controlbarProfileModel.nameError(name, mode === "rename") : ""

            title: mode === "rename" ? qsTr("Rename Profile")
                                     : mode === "clone" ? qsTr("Clone Profile") : qsTr("New Profile")
            acceptText: mode === "rename" ? qsTr("Rename") : mode === "clone" ? qsTr("Clone") : qsTr("Create")
            acceptEnabled: name.length > 0 && nameError.length === 0
            preferredWidth: VLCStyle.dp(713, VLCStyle.scale)
            closeOnAccept: false

            // the field holds a name to start from, selected: typing replaces it
            function openFor(newMode) {
                const model = MainCtx.controlbarProfileModel
                const current = model.currentModel ? model.currentModel.name : ""
                mode = newMode
                nameField.text = newMode === "rename" ? current
                               : newMode === "clone" ? model.uniqueName(current)
                               : model.uniqueName(qsTr("Profile %1").arg(model.count + 1))
                failure = ""
                open()
            }

            onOpened: {
                nameField.forceActiveFocus()
                nameField.selectAll()
            }
            onAccepted: {
                if (root.applyName(mode, name))
                    close()
                else
                    failure = MainCtx.controlbarProfileModel.lastError
            }

            Widgets.RedesignDialogHeading {
                text: qsTr("Profile Name")
                description: namePrompt.mode === "rename" ? qsTr("The profile keeps its layout and its settings.")
                             : namePrompt.mode === "clone" ? qsTr("The copy starts with the layout and the settings of this profile.")
                             : qsTr("The new profile starts with the default layout and the settings in use.")
            }

            Widgets.RedesignTextArea {
                id: nameField
                objectName: "toolbarEditorProfileNameField"
                placeholderText: qsTr("e.g., Late Night")
                Accessible.name: qsTr("Profile name")
                onAccepted: namePrompt.accept()
                onTextChanged: namePrompt.failure = ""
            }

            PromptError {
                text: namePrompt.failure || namePrompt.nameError
            }
        }

        // Delete of the header
        Widgets.RedesignDialog {
            id: deleteConfirmation
            objectName: "toolbarEditorDeleteConfirmation"

            property string failure: ""
            readonly property string profileName: MainCtx.controlbarProfileModel.currentModel
                                                  ? MainCtx.controlbarProfileModel.currentModel.name : ""

            title: qsTr("Delete Profile")
            acceptText: qsTr("Delete")
            preferredWidth: VLCStyle.dp(713, VLCStyle.scale)
            closeOnAccept: false

            onOpened: failure = ""
            onAccepted: {
                if (MainCtx.controlbarProfileModel.deleteSelectedProfile())
                    close()
                else
                    failure = MainCtx.controlbarProfileModel.lastError
            }

            Widgets.RedesignDialogHeading {
                text: qsTr("Delete %1?").arg(deleteConfirmation.profileName)
                description: qsTr("Its layout and its settings go with it.")
            }

            PromptError {
                text: deleteConfirmation.failure
            }
        }
    }

    // a line of error of the prompts, as the footer shows them
    component PromptError: Text {
        Layout.fillWidth: true
        visible: text.length > 0
        color: "#e4a580"
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.redesign.fontSmall
        wrapMode: Text.WordWrap
        Accessible.role: Accessible.AlertMessage
    }
}

/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl as Impl
import VLC.MainInterface
import VLC.Style
import VLC.Widgets as Widgets

// Figma 2372:8834 "Setting Preset": the field of the preset in use (42 high,
// #090909, radius 6, its name 14 px #8F8F8F 16 in, the chevron 16 from the
// end, up while open) and, 35 under it, its list (as wide, #191919, radius
// 6, padding 2, rows of 38 two apart): "New Preset" dimmed (#6B6B6B) after
// its plus, the presets made after their "more" (both 14 in, the names 48
// in), "Default Preset" last, 14 in; #1F1F1F under the one chosen (the check
// of the accent 14 from its end) or the pointer. As the dropdown of the
// settings (SettingsDropdown) where the design does not differ.
ColumnLayout {
    id: root

    // AudioEffects and VideoEffects of the window
    required property var audio
    required property var video

    // the preset chosen, "" for the default one
    property string chosen: ""
    // chosen in this window: kept as the controls change the effects; until
    // then, the preset the effects are (refresh())
    property bool userChose: false
    // why the last action did nothing or not all of it (under the field)
    property string message: ""
    // the preset whose "more" opened the actions
    property string actionsFor: ""

    readonly property EffectPresets presets: EffectPresets {
        audio: root.audio
        video: root.video
    }
    // "New Preset", the presets made, "Default Preset"
    readonly property var entries: [{ kind: "new", name: "" }]
        .concat(presets.names.map(name => ({ kind: "custom", name: name })))
        .concat([{ kind: "default", name: "" }])

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(dark, light) { return VLCStyle.redesign.color(dark, light) }

    // the preset the effects playing are, until one is chosen here
    function refresh() {
        if (!userChose)
            chosen = presets.matchingName()
    }

    function openList(byKeyboard) {
        message = ""
        list.byKeyboard = byKeyboard
        list.open()
    }

    // the row of the preset chosen
    function chosenRow() {
        return chosen ? 1 + presets.indexOf(chosen) : entries.length - 1
    }

    function focusRow(index, byKeyboard) {
        const row = rowRepeater.itemAt(Math.max(0, Math.min(index, rowRepeater.count - 1)))
        if (!row)
            return
        row.forceActiveFocus(byKeyboard ? Qt.TabFocusReason : Qt.OtherFocusReason)
        if (row.y < rowsView.contentY)
            rowsView.contentY = row.y
        else if (row.y + row.height > rowsView.contentY + rowsView.height)
            rowsView.contentY = row.y + row.height - rowsView.height
    }

    // applied as the controls would set the effects: Save of the window
    // keeps them, Cancel drops them
    function choose(name) {
        list.close()
        if (!presets.apply(name)) {
            message = presets.lastError
            return
        }
        chosen = name
        userChose = true
        message = presets.lastError
    }

    function startNew() {
        list.close()
        if (!presets.canCapture()) {
            message = qsTr("Play a media to save its effects as a preset.")
            return
        }
        namePrompt.openFor("new", "")
    }

    function openActions(button, name) {
        actionsFor = name
        actions.canUpdate = presets.canCapture()
        actions.toggleFrom(button, 0, button.height)
    }

    function updatePreset(name) {
        list.close()
        if (!presets.update(name)) {
            message = presets.lastError
            return
        }
        chosen = name
        userChose = true
        message = ""
    }

    spacing: 0

    T.Button {
        id: field
        objectName: "effectPresetField"

        Layout.fillWidth: true
        implicitHeight: root.px(42)
        padding: 0
        focusPolicy: Qt.StrongFocus
        text: root.chosen || root.presets.defaultName

        Accessible.role: Accessible.ComboBox
        Accessible.name: qsTr("Effect preset: %1").arg(text)

        // (Space clicks it: the list then shows the focus as for Return)
        onClicked: root.openList(field.visualFocus)
        Keys.onReturnPressed: root.openList(true)
        Keys.onEnterPressed: root.openList(true)
        Keys.onDownPressed: root.openList(true)

        background: Rectangle {
            radius: root.px(6)
            color: root.c("#090909", "#dedede")
            border.width: field.visualFocus ? 1 : 0
            border.color: VLCStyle.redesign.focusColor
        }

        contentItem: Item {
            Text {
                id: fieldText
                x: root.px(16)
                width: parent.width - x - root.px(16 + 24 + 10)
                height: parent.height
                text: field.text
                color: root.c("#8F8F8F", "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(14)
                elide: Text.ElideRight
                verticalAlignment: Text.AlignTop
                // the baseline where the design centres a line of Inter
                topPadding: height / 2 + VLCStyle.redesign.centeredBaseline(font.pixelSize) - fieldMetrics.ascent
                FontMetrics { id: fieldMetrics; font: fieldText.font }
            }

            // up while the list is open
            Impl.ColorImage {
                x: parent.width - root.px(16) - width
                anchors.verticalCenter: parent.verticalCenter
                width: root.px(24)
                height: width
                sourceSize: Qt.size(width, height)
                source: "qrc:///redesign/icons/chevron-down/default.png"
                color: root.c("#8F8F8F", "#2e2e2e")
                rotation: list.visible ? 180 : 0
            }
        }
    }

    // (not in the design: what the action could not do, under the field)
    Text {
        Layout.fillWidth: true
        Layout.topMargin: root.px(8)
        visible: root.message.length > 0
        text: root.message
        color: VLCStyle.redesign.color("#FF8989", "#B3261E")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(12)
        wrapMode: Text.WordWrap
        Accessible.role: Accessible.AlertMessage
    }

    T.Popup {
        id: list
        objectName: "effectPresetList"

        // opened from the keyboard: the row focused shows it
        property bool byKeyboard: false

        parent: field
        y: field.height + root.px(35)
        width: field.width
        // nine rows at most, the others scrolled to
        implicitHeight: Math.min(rows.implicitHeight, 9 * root.px(38) + 8 * root.px(2)) + topPadding + bottomPadding
        padding: root.px(2)
        margins: root.px(8)
        // a press outside only closes it, and goes no further
        modal: true
        dim: false
        focus: true
        closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside

        onOpened: root.focusRow(root.chosenRow(), byKeyboard)
        onClosed: field.forceActiveFocus()

        background: Rectangle {
            radius: root.px(6)
            color: root.c("#191919", "#ababaa")

            // Figma 2372:8834: a shadow all around, 0 0 blur 20 #000000 70 %
            // (measured, wider than the one of the menus)
            Widgets.RoundedRectangleShadow {
                blurRadius: 10 * VLCStyle.scale // (dp() rounds to whole pixels)
                color: Qt.rgba(0, 0, 0, 0.7)
            }
        }

        contentItem: Flickable {
            id: rowsView
            clip: true
            contentWidth: width
            contentHeight: rows.implicitHeight
            interactive: contentHeight > height
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: rows
                width: rowsView.width
                spacing: root.px(2)
                Accessible.role: Accessible.List
                Accessible.name: qsTr("Effect presets")

                Repeater {
                    id: rowRepeater
                    model: root.entries

                    delegate: T.ItemDelegate {
                        id: row
                        required property var modelData
                        required property int index

                        readonly property string kind: modelData.kind
                        readonly property string name: modelData.name
                        readonly property bool chosen: kind === "custom" ? name === root.chosen
                                                                         : kind === "default" && root.chosen === ""
                        readonly property string label: kind === "new" ? qsTr("New Preset")
                                                       : kind === "default" ? root.presets.defaultName : name

                        objectName: "effectPresetRow_" + index
                        width: rows.width
                        height: root.px(38)
                        padding: 0
                        leftPadding: kind === "default" ? root.px(14) : root.px(14 + 24 + 10)
                        rightPadding: root.px(14 + 24 + 14)
                        focusPolicy: Qt.StrongFocus

                        Accessible.name: label
                        Accessible.description: chosen ? qsTr("Chosen preset") : ""

                        onClicked: {
                            if (kind === "new")
                                root.startNew()
                            else
                                root.choose(kind === "custom" ? name : "")
                        }
                        Keys.onReturnPressed: row.clicked()
                        Keys.onEnterPressed: row.clicked()
                        Keys.onUpPressed: root.focusRow(index - 1, true)
                        Keys.onDownPressed: root.focusRow(index + 1, true)
                        // the menu key: the actions of the preset
                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Menu && kind === "custom") {
                                root.openActions(more, name)
                                event.accepted = true
                            }
                        }

                        background: Rectangle {
                            radius: root.px(4)
                            color: row.chosen || row.hovered || row.visualFocus ? root.c("#1F1F1F", "#9c9c9b") : "transparent"
                            border.width: row.visualFocus ? 1 : 0
                            border.color: VLCStyle.redesign.focusColor

                            // the plus of "New Preset", dimmed as its name
                            Impl.ColorImage {
                                visible: row.kind === "new"
                                x: root.px(14)
                                anchors.verticalCenter: parent.verticalCenter
                                width: root.px(24)
                                height: width
                                source: visible ? "qrc:///redesign/icons/add/default.png" : ""
                                color: root.c("#6B6B6B", "#6b6b6b")
                            }

                            Impl.ColorImage {
                                visible: row.chosen
                                x: parent.width - root.px(14) - width
                                anchors.verticalCenter: parent.verticalCenter
                                width: root.px(24)
                                height: width
                                source: "qrc:///redesign/track-check-active.png"
                                color: VLCStyle.redesign.accentTint
                            }
                        }

                        contentItem: Text {
                            id: rowText
                            text: row.label
                            color: row.kind === "new" ? root.c("#6B6B6B", "#6b6b6b") : root.c("#8F8F8F", "#2e2e2e")
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: root.px(14)
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignTop
                            // the baseline where the design centres a line of Inter
                            topPadding: height / 2 + VLCStyle.redesign.centeredBaseline(font.pixelSize) - rowMetrics.ascent
                            FontMetrics { id: rowMetrics; font: rowText.font }
                        }

                        // the "more" of a preset made: Rename, Update, Delete
                        T.Button {
                            id: more
                            // (RedesignMenu.toggleFrom: a press that closed the
                            // menu does not open it again)
                            property double pressedAt: 0

                            visible: row.kind === "custom"
                            x: root.px(14 - 4)
                            anchors.verticalCenter: parent.verticalCenter
                            width: root.px(32)
                            height: width
                            padding: root.px(4)
                            focusPolicy: Qt.TabFocus
                            Accessible.name: qsTr("Actions of %1").arg(row.name)

                            onPressed: pressedAt = Date.now()
                            onClicked: root.openActions(more, row.name)

                            background: Rectangle {
                                radius: root.px(6)
                                color: more.hovered || more.down || (actions.opened && root.actionsFor === row.name)
                                       ? root.c("#2E2E2E", "#8e8e8d") : "transparent"
                                border.width: more.visualFocus ? 1 : 0
                                border.color: VLCStyle.redesign.focusColor
                            }
                            contentItem: Impl.ColorImage {
                                source: "qrc:///redesign/icons/more/default.png"
                                sourceSize: Qt.size(root.px(24), root.px(24))
                                fillMode: Image.Pad
                                color: root.c("#8F8F8F", "#2e2e2e")
                            }
                        }
                    }
                }
            }
        }
    }

    // the actions of a preset made (as the profiles: ProfilesMenu)
    Widgets.RedesignMenu {
        id: actions
        // nothing plays: no effects to update the preset with
        property bool canUpdate: true

        Widgets.RedesignMenuItem {
            objectName: "effectPresetRename"
            text: qsTr("Rename")
            iconName: "edit"
            onTriggered: {
                list.close()
                namePrompt.openFor("rename", root.actionsFor)
            }
        }
        Widgets.RedesignMenuItem {
            objectName: "effectPresetUpdate"
            text: qsTr("Update with current effects")
            iconName: "save"
            enabled: actions.canUpdate
            onTriggered: root.updatePreset(root.actionsFor)
        }
        Widgets.RedesignMenuItem {
            objectName: "effectPresetDelete"
            text: qsTr("Delete")
            iconName: "remove"
            onTriggered: {
                list.close()
                deleteConfirmation.openFor(root.actionsFor)
            }
        }
    }

    // New Preset and Rename: the name, in the dialog of the design that names
    // things (as the profiles of Customize Player)
    Widgets.RedesignDialog {
        id: namePrompt
        objectName: "effectPresetName"

        // "new" or "rename"
        property string mode: "new"
        // the preset renamed
        property string original: ""
        // why the name was refused
        property string failure: ""
        // a name taken shows as it is typed
        readonly property string nameError: nameField.text.trim().length > 0
                                            ? root.presets.nameError(nameField.text, original) : ""

        title: mode === "rename" ? qsTr("Rename Preset") : qsTr("New Preset")
        acceptText: mode === "rename" ? qsTr("Rename") : qsTr("Create")
        preferredWidth: VLCStyle.dp(713, VLCStyle.scale)
        closeOnAccept: false

        // the field holds a name to start from, selected: typing replaces it
        function openFor(newMode, name) {
            mode = newMode
            original = name
            nameField.text = newMode === "rename" ? name : root.presets.suggestedName()
            failure = ""
            open()
        }

        onOpened: {
            nameField.forceActiveFocus()
            nameField.selectAll()
        }
        onClosed: field.forceActiveFocus()
        // an empty name or one taken is refused, the dialog stays
        onAccepted: {
            const done = mode === "rename" ? root.presets.rename(original, nameField.text)
                                           : root.presets.create(nameField.text)
            if (!done) {
                failure = root.presets.lastError
                return
            }
            const name = root.presets.normalName(nameField.text)
            // the new preset is the one chosen
            if (mode === "new") {
                root.chosen = name
                root.userChose = true
            } else if (root.chosen === original) {
                root.chosen = name
            }
            root.message = ""
            close()
        }

        Widgets.RedesignDialogHeading {
            text: qsTr("Preset Name")
            description: namePrompt.mode === "rename" ? qsTr("The preset keeps its effects.")
                                                      : qsTr("The preset keeps the audio and video effects in use.")
        }

        Widgets.RedesignTextArea {
            id: nameField
            objectName: "effectPresetNameField"
            placeholderText: qsTr("e.g., Late Night Movies")
            Accessible.name: qsTr("Preset name")
            onAccepted: namePrompt.accept()
            onTextChanged: namePrompt.failure = ""
        }

        PromptError {
            text: namePrompt.failure || namePrompt.nameError
        }
    }

    // Delete of the actions
    Widgets.RedesignDialog {
        id: deleteConfirmation
        objectName: "effectPresetDeleteConfirmation"

        property string name: ""
        property string failure: ""

        title: qsTr("Delete Preset")
        acceptText: qsTr("Delete")
        preferredWidth: VLCStyle.dp(713, VLCStyle.scale)
        closeOnAccept: false

        function openFor(presetName) {
            name = presetName
            failure = ""
            open()
        }

        onClosed: field.forceActiveFocus()
        onAccepted: {
            if (!root.presets.remove(name)) {
                failure = root.presets.lastError
                return
            }
            // the effects stay: the preset they are now, if one
            if (root.chosen === name)
                root.chosen = root.presets.matchingName()
            close()
        }

        Widgets.RedesignDialogHeading {
            text: qsTr("Delete %1?").arg(deleteConfirmation.name)
            description: qsTr("The effects in use stay as they are.")
        }

        PromptError {
            text: deleteConfirmation.failure
        }
    }

    // a line of error of the prompts (as ToolbarEditorDialog)
    component PromptError: Text {
        Layout.fillWidth: true
        visible: text.length > 0
        color: VLCStyle.redesign.color("#e4a580", "#c62828")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.redesign.fontSmall
        wrapMode: Text.WordWrap
        Accessible.role: Accessible.AlertMessage
    }
}

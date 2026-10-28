/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl as Impl
import VLC.Dialogs
import VLC.MainInterface
import VLC.Style
import VLC.Widgets as Widgets

// An option of a video filter as the design draws them (Figma 536:1855,
// 614:1430): a slider (ValueSlider), a 32 px row of a name (14 px,
// #6B6B6B) and a toggle or a color field, a 48 px dropdown; compact, the
// color field only (in the row of its filter: "Color Extraction")
ColumnLayout {
    id: root

    required property var effects
    required property var option
    // The name of the design, else the one of VLC
    property string label: option.name
    // In the units of the option
    property var ticks: []
    // Its filter on (the sliders in the accent, else grey)
    property bool active: true
    // At the end of the row of a slider (the toggle of a filter of one setting)
    property Component trailing: null
    property bool compact: false

    readonly property var currentValue: {
        effects.revision
        return effects.parameterValue(option.id)
    }
    readonly property bool number: option.type === "float" || option.type === "integer"
    readonly property bool bounded: option.minimum !== undefined && option.maximum !== undefined
    // The component to extract is an RGB mask too (three of them named)
    readonly property bool colorValue: option.type === "color" || option.id === "extract-component"
    readonly property var choiceList: option.type === "choice" || colorValue ? effects.choices(option.id) : []

    spacing: 0

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value) }
    function hex(value) {
        const number = Number(value)
        return "#" + (Number.isFinite(number) ? number : 0).toString(16).padStart(6, "0").toUpperCase()
    }

    Widgets.FilePicker { id: filePicker; ctx: MainCtx }

    ValueSlider {
        Layout.fillWidth: true
        visible: root.number && root.bounded && !root.compact
        label: root.label
        from: root.bounded ? root.option.minimum : 0
        to: root.bounded ? root.option.maximum : 1
        modelValue: Number(root.currentValue)
        decimals: root.option.type === "integer" ? 0 : 2
        stepSize: root.option.type === "integer" ? 1 : 0
        ticks: root.ticks
        active: root.active
        trailing: root.trailing
        onEdited: (value) => root.effects.setParameter(root.option.id, root.option.type === "integer" ? Math.round(value) : value)
    }

    // A number without bounds: the name beside the field of the design
    Item {
        visible: root.number && !root.bounded && !root.compact
        Layout.fillWidth: true
        implicitHeight: root.px(36)
        OptionLabel {
            anchors.left: parent.left
            anchors.right: numberField.left
            anchors.rightMargin: root.px(10)
            anchors.baseline: parent.verticalCenter
            anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
        }
        SettingsTextField {
            id: numberField
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: root.px(140)
            height: root.px(36)
            font.pixelSize: root.px(12)
            text: String(root.currentValue)
            Accessible.name: root.label
            onEditingFinished: {
                const number = Number(text)
                if (Number.isFinite(number) && (root.option.type !== "integer" || Number.isInteger(number)))
                    root.effects.setParameter(root.option.id, number)
                text = String(root.effects.parameterValue(root.option.id))
            }
            Connections {
                target: root.effects
                function onStateChanged() {
                    if (!numberField.activeFocus)
                        numberField.text = String(root.currentValue)
                }
            }
        }
    }

    // On or off: the name and the toggle (Figma 614:1430 "Color", "Cartoon")
    Item {
        visible: root.option.type === "bool" && !root.compact
        Layout.fillWidth: true
        implicitHeight: root.px(32)
        OptionLabel {
            anchors.left: parent.left
            anchors.right: boolOption.left
            anchors.rightMargin: root.px(10)
            anchors.baseline: parent.verticalCenter
            anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
        }
        Widgets.CheckButton {
            id: boolOption
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            focusPolicy: Qt.StrongFocus
            Accessible.name: root.label
            Binding on checked {
                when: !boolOption.pressed
                value: Boolean(root.currentValue)
            }
            onClicked: root.effects.setParameter(root.option.id, checked)
        }
    }

    // A color: Figma 614:1430, a 116 x 32 field (#090909, radius 6), the
    // value (14 px, #6B6B6B) 12 px in, its 22 px swatch 5 px from the end;
    // the swatch lists the colors VLC names
    Item {
        visible: root.colorValue
        Layout.fillWidth: !root.compact
        implicitWidth: root.compact ? colorField.width : 0
        implicitHeight: root.px(32)
        OptionLabel {
            visible: !root.compact
            anchors.left: parent.left
            anchors.right: colorField.left
            anchors.rightMargin: root.px(10)
            anchors.baseline: parent.verticalCenter
            anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
        }
        Rectangle {
            id: colorField
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: root.px(116)
            height: root.px(32)
            radius: root.px(6)
            color: root.c("#090909")
            border.width: colorInput.activeFocus ? 1 : 0
            border.color: VLCStyle.redesign.accentDim

            // the value 12 in, the swatch at the end (right to left: mirrored)
            TextInput {
                id: colorInput
                x: LayoutMirroring.enabled ? parent.width - width - root.px(12) : root.px(12)
                width: parent.width - root.px(12) - root.px(4) - swatch.width - root.px(5)
                horizontalAlignment: TextInput.AlignLeft
                anchors.baseline: parent.verticalCenter
                anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
                clip: true
                text: root.hex(root.currentValue)
                color: root.c("#6B6B6B")
                selectionColor: VLCStyle.redesign.accentBackground
                selectedTextColor: VLCStyle.redesign.accent
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(14)
                selectByMouse: true
                maximumLength: 7
                Accessible.name: root.label
                onEditingFinished: {
                    if (/^#?[0-9a-fA-F]{6}$/.test(text))
                        root.effects.setParameter(root.option.id, parseInt(text.replace("#", ""), 16))
                    text = root.hex(root.effects.parameterValue(root.option.id))
                }
                Connections {
                    target: root.effects
                    function onStateChanged() {
                        if (!colorInput.activeFocus)
                            colorInput.text = root.hex(root.currentValue)
                    }
                }
            }

            T.Button {
                id: swatch
                objectName: "colorChoices-" + root.option.id
                anchors.right: parent.right
                anchors.rightMargin: root.px(5)
                anchors.verticalCenter: parent.verticalCenter
                width: root.px(22)
                height: width
                enabled: root.choiceList.length > 0
                focusPolicy: Qt.StrongFocus
                Accessible.name: qsTr("Colors of %1").arg(root.label)
                onClicked: colorChoices.open()
                background: Rectangle {
                    radius: root.px(2)
                    color: root.hex(root.currentValue)
                    border.width: swatch.visualFocus ? root.px(2) : 1
                    border.color: swatch.visualFocus ? VLCStyle.redesign.accent : Qt.rgba(1, 1, 1, 0.2)
                }
            }

            // The list of the dropdowns of the design (Figma 2372:8300): 20
            // px under the field, padding 2, rows of 44 2 px apart, #1F1F1F
            // under the one chosen or the pointer, the check of the accent
            T.Popup {
                id: colorChoices
                // at the end of the field (right to left: its left edge)
                x: colorField.LayoutMirroring.enabled ? 0 : parent.width - width
                y: parent.height + root.px(20)
                width: root.px(200)
                implicitHeight: implicitContentHeight + topPadding + bottomPadding
                padding: root.px(2)
                margins: root.px(8)
                modal: true
                dim: false
                closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside

                contentItem: Column {
                    // a popup does not inherit the mirroring of the window (right to left)
                    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
                    LayoutMirroring.childrenInherit: true
                    spacing: root.px(2)
                    Repeater {
                        model: root.choiceList
                        T.ItemDelegate {
                            id: colorChoice
                            required property var modelData
                            readonly property bool chosen: Number(modelData.id) === Number(root.currentValue)
                            width: colorChoices.availableWidth
                            implicitHeight: root.px(44)
                            // the swatch before the name, the check after it
                            leftPadding: LayoutMirroring.enabled ? root.px(14 + 24 + 13) : root.px(14 + 22 + 10)
                            rightPadding: LayoutMirroring.enabled ? root.px(14 + 22 + 10) : root.px(14 + 24 + 13)
                            focusPolicy: Qt.StrongFocus
                            Accessible.name: modelData.name
                            onClicked: {
                                if (root.effects.setParameter(root.option.id, modelData.id))
                                    colorChoices.close()
                            }
                            contentItem: Text {
                                text: colorChoice.modelData.name
                                color: root.c("#8F8F8F")
                                font.family: VLCStyle.redesign.fontFamily
                                font.pixelSize: root.px(14)
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            background: Rectangle {
                                radius: root.px(4)
                                color: colorChoice.chosen || colorChoice.hovered || colorChoice.visualFocus
                                       ? root.c("#1F1F1F") : "transparent"
                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.leftMargin: root.px(14)
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: root.px(22)
                                    height: width
                                    radius: root.px(2)
                                    color: root.hex(colorChoice.modelData.id)
                                    border.width: 1
                                    border.color: Qt.rgba(1, 1, 1, 0.2)
                                }
                                Impl.ColorImage {
                                    visible: colorChoice.chosen
                                    anchors.right: parent.right
                                    anchors.rightMargin: root.px(13)
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: root.px(24)
                                    height: width
                                    source: "qrc:///redesign/track-check-active.png"
                                    color: VLCStyle.redesign.accentTint
                                }
                            }
                        }
                    }
                }
                background: Rectangle {
                    radius: root.px(6)
                    color: root.c("#191919")

                    // Figma: drop shadow 0 8 blur 15 #000000 46 % (as RedesignMenu)
                    Widgets.RoundedRectangleShadow {
                        yOffset: root.px(8)
                        blurRadius: 7.5 * VLCStyle.scale
                        color: Qt.rgba(0, 0, 0, 0.46)
                    }
                }
            }
        }
    }

    // A typed string (the text of the marquee), a file also picked (the
    // logo): the field and the button of the settings
    ColumnLayout {
        Layout.fillWidth: true
        visible: (root.option.type === "text" || root.option.type === "file") && !root.compact
        spacing: root.px(10)
        OptionLabel {
            Layout.fillWidth: true
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: root.px(8)
            SettingsTextField {
                id: textInput
                Layout.fillWidth: true
                Layout.minimumWidth: root.px(100)
                Layout.preferredWidth: root.px(100)
                text: String(root.currentValue)
                Accessible.name: root.label
                // (also emitted on a mere loss of focus: the mask of erase
                // set again would open the filters again)
                onEditingFinished: {
                    if (text !== String(root.currentValue))
                        root.effects.setParameter(root.option.id, text)
                    text = String(root.effects.parameterValue(root.option.id))
                }
                Connections {
                    target: root.effects
                    function onStateChanged() {
                        if (!textInput.activeFocus)
                            textInput.text = String(root.currentValue)
                    }
                }
            }
            SettingsButton {
                visible: root.option.type === "file"
                text: qsTr("Browse")
                Accessible.name: qsTr("Browse for %1").arg(root.label)
                onClicked: {
                    const file = filePicker.chooseOpenFile(root.label,
                                                           qsTr("Image Files (*.png *.jpg);;All Files (*)"))
                    if (file.length) root.effects.setParameter(root.option.id, file)
                }
            }
        }
    }

    // A value of a list: Figma 614:1430 (the mode of the gradient), a 48 px
    // dropdown, 2 px #0E0E0E around #191919, the text (#6B6B6B) 18 px in,
    // the chevron 13 px from the end; the list of the settings
    SettingsDropdown {
        id: choiceBox
        visible: root.option.type === "choice" && !root.colorValue && !root.compact
        Layout.fillWidth: true
        implicitHeight: root.px(48)
        // the text 18 in, the chevron at the end (swapped right to left)
        leftPadding: LayoutMirroring.enabled ? root.px(18 + 24 + 13) : root.px(18)
        rightPadding: LayoutMirroring.enabled ? root.px(18) : root.px(18 + 24 + 13)
        model: root.choiceList
        textRole: "name"
        currentIndex: root.choiceList.findIndex(choice => String(choice.id) === String(root.currentValue))
        // the name before the value, but where the design shows the value alone
        displayText: (root.label.length ? root.label + ": " : "")
                     + (currentIndex < 0 ? String(root.currentValue) : currentText)
        Accessible.name: root.label.length ? root.label : root.option.name
        onActivated: (index) => root.effects.setParameter(root.option.id, root.choiceList[index].id)

        background: Rectangle {
            radius: root.px(6)
            color: root.c("#0E0E0E")
            border.width: choiceBox.visualFocus ? 1 : 0
            border.color: VLCStyle.redesign.accent
            Rectangle {
                anchors.fill: parent
                anchors.margins: root.px(2)
                radius: root.px(4)
                color: choiceBox.hovered && !choiceBox.popup.visible ? root.c("#222222") : root.c("#191919")
            }
        }
        contentItem: Text {
            id: choiceText
            text: choiceBox.displayText
            color: root.c("#6B6B6B")
            font: choiceBox.font
            verticalAlignment: Text.AlignTop
            // the baseline where the design centres a line of Inter
            topPadding: height / 2 + VLCStyle.redesign.centeredBaseline(font.pixelSize) - choiceMetrics.ascent
            FontMetrics { id: choiceMetrics; font: choiceText.font }
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
        }
        indicator: Image {
            x: choiceBox.LayoutMirroring.enabled ? root.px(13) : choiceBox.width - root.px(13) - width
            y: (choiceBox.height - height) / 2
            width: root.px(24)
            height: width
            sourceSize: Qt.size(width, height)
            source: "qrc:///redesign/icons/chevron-down/default.png"
        }
    }

    // The name of a row (14 px, #6B6B6B): cut before its control, whole in
    // its tooltip; at the start of the row, right to left too
    component OptionLabel: Text {
        id: optionLabel
        text: root.label
        color: root.c("#6B6B6B")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(14)
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignLeft
        HoverHandler { id: labelHover }
        T.ToolTip.visible: labelHover.hovered && optionLabel.truncated
        T.ToolTip.text: optionLabel.text
        T.ToolTip.delay: 650
    }
}

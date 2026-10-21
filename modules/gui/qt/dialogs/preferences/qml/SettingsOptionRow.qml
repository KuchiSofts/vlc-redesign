/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import VLC.Style
import VLC.Util

// Row of the settings of the design: the label (Inter 14 #8F8F8F) with its
// (i) 10 px after it, the control on the right; 5 px above and below the
// control (42 px for a toggle, 46 for a dropdown or a field, 55 for a slider
// with its scale)
Item {
    id: root
    property string settingKey
    property string label
    property string help
    property string settingType
    property var settingValue
    property var choices: []
    property var minimum
    property var maximum
    property string editorKind: ""
    property bool dirty: false
    // The (i) of the design (rows of the design without it pass false)
    property bool showInfo: help.length > 0
    // Space between the label and its (i)
    property real infoGap: 10
    // Unit after the value of a field ("s", "ms")
    property string unit: ""
    // Decimals of a float in its field (the design shows "0.0")
    property var decimals
    // Text of an empty field (the "System proxy")
    property string placeholder: ""
    // The value of an "inttoggle" switched on (its default)
    property var onValue: 1
    // The value of a second option the editor sets (the startup volume)
    property var otherValue
    // Shown in an empty field: what VLC uses then (the Videos folder...)
    property string defaultText: ""
    // Shown for the default value while the field has not the focus (the
    // monospace font of the system: "System monospace")
    property string defaultDisplay: ""
    // The value as the design shows it while the field has not the focus
    // (a folder from the home of the user: "~/Videos")
    property string displayValue: ""
    // Dimmed and inactive (a setting that only applies when another is on)
    property bool available: true
    // false: the field is empty while it has not the focus (the port of the
    // web remote while it is off, as the design draws it)
    property bool valueShown: true
    // The name read out for the row
    property string accessibleLabel: label
    // A line under the label ("Applies after restart"; a warning: the web
    // remote without a password), as the note of the interface language
    property string note: ""
    property bool noteWarning: false
    readonly property real noteHeight: noteText.visible ? noteText.implicitHeight + px(6) : 0
    readonly property bool dark: VLCStyle.palette.isDark

    // Sliders of the design for options of VLC without a range
    readonly property bool scaledOption: settingKey === "sub-delay" || settingKey === "osd-duration"
                                         || settingKey === "video-title-timeout"
    readonly property bool useSlider: (editorKind === "slider" || editorKind === "opacity" || editorKind === "percent"
                                       || editorKind === "opacity10" || editorKind === "sensitivity")
                                      && choices.length === 0
                                      && (settingType === "integer" || settingType === "float")
                                      && (scaledOption
                                          || (minimum !== undefined && minimum !== null
                                              && maximum !== undefined && maximum !== null
                                              && Number(minimum) < Number(maximum)))
    readonly property bool useColor: editorKind === "color"
    readonly property bool useSegmented: editorKind === "segmented"
    // "inttoggle": an integer option shown as a toggle (0 off, the default on)
    readonly property bool useToggle: (settingType === "boolean" && editorKind !== "boolchoice"
                                       && editorKind !== "startupvolume")
                                      || editorKind === "inttoggle"
    // Remember last volume / Always starts at (the volume is otherValue)
    readonly property bool useStartupVolume: editorKind === "startupvolume"
    readonly property bool useChoices: !useToggle && !useSegmented && (choices.length > 0 || editorKind === "boolchoice")
    readonly property bool useFile: editorKind === "savefile" || editorKind === "directory" || editorKind === "openfile"
    readonly property bool useText: !useToggle && !useChoices && !useSlider && !useStartupVolume && !useSegmented
                                    && !useColor

    // Scales of the design under some sliders: ticks, then labels
    readonly property var sliderScale: {
        if (!useSlider)
            return null
        const lo = Number(minimum), hi = Number(maximum)
        const range = (values, labels, from, to) => ({ from: from, to: to, ticks: values, labels: labels })
        const seq = (from, to, step) => { const out = []; for (let v = from; v <= to + 1e-9; v += step) out.push(v); return out }
        switch (settingKey) {
        case "redesign/notification-duration":
        case "mouse-hide-timeout":
            return range(seq(0, 10000, 500), [{ value: 0, text: qsTr("0 s") }, { value: 5000, text: qsTr("5 s") },
                                               { value: 10000, text: qsTr("10s") }], 0, 10000)
        case "sub-position":
            // percent of the picture, above (or below) the usual place
            return range(seq(-20, 60, 10), [{ value: -20, text: qsTr("Lower") },
                                            { value: 0, text: qsTr("Usual") },
                                            { value: 60, text: qsTr("Higher") }], -20, 60)
        case "sub-delay":
            // tenths of seconds
            return range(seq(-50, 50, 10), [{ value: -50, text: qsTr("-5 s") }, { value: 0, text: qsTr("0 s") },
                                            { value: 50, text: qsTr("+5 s") }], -50, 50)
        case "osd-duration":
            return range(seq(500, 5000, 500), [{ value: 500, text: qsTr("0.5 s") }, { value: 5000, text: qsTr("5 s") }],
                         500, 5000)
        case "video-title-timeout":
            return range(seq(1000, 10000, 1000), [{ value: 1000, text: qsTr("1 s") }, { value: 10000, text: qsTr("10 s") }],
                         1000, 10000)
        case "rate":
            return range(seq(0.25, 2, 0.25), seq(0.25, 2, 0.25).map(v => ({ value: v, text: String(v) })), 0.25, 2)
        case "qt-max-volume":
            return range(seq(100, 300, 25), [100, 200, 300].map(v => ({ value: v, text: v + "%" })), 100, 300)
        }
        // Low, Medium, High: the pixels the mouse moves before the controls show
        if (editorKind === "sensitivity")
            return range([], [{ value: 0, text: qsTr("Low") }, { value: 1, text: qsTr("Medium") },
                              { value: 2, text: qsTr("High") }], 0, 2)
        if (editorKind === "opacity10")
            return range(seq(1, 10, 1).map(i => i / 10),
                         [{ value: lo, text: Math.round(lo * 100) + "%" }, { value: hi, text: "100%" }], lo, hi)
        // in percent, ticks every 10 % (Text / Background opacity of the
        // design: 50 % is the middle, not 128 of 255)
        if (editorKind === "opacity")
            return range(seq(10, 90, 10), [{ value: 0, text: "0%" }, { value: 50, text: "50%" },
                                           { value: 100, text: "100%" }], 0, 100)
        return null
    }

    signal commit(var value)
    signal commitOther(var value)
    signal browseRequested()
    signal colorRequested()

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value, value) }

    // A height of the design for the row (48 for the skips, the buffers),
    // the top of its control (centred if undefined) and the baseline of the
    // labels of its slider from the top of the slider
    property real rowHeight: 0
    // The height of the field (and its Browse) when the design draws it
    // smaller than 36 (the time-shift folder, the fields of the video)
    property real fieldHeight: 0
    property var controlTop
    property var labelsBaseline
    readonly property bool scaleLabel: useSlider && !!sliderScale

    implicitHeight: noteHeight + (rowHeight > 0 ? px(rowHeight)
                                  : px(10) + (useStartupVolume ? px(78) : useToggle || useColor ? px(32)
                                              : useSlider ? (sliderScale ? px(57) : px(24))
                                              : px(36)))
    // 30 % as the dimmed rows of the design
    opacity: available ? 1 : 0.3
    enabled: available

    Row {
        id: labelRow
        anchors.left: parent.left
        // centred on the control, as the auto layout of the design; next to
        // a slider scale centred on the row, Figma's way: the 17 px box at
        // (row - 17) / 2 (not rounded), the baseline 14 px under it (measured
        // on 57 and 67 rows; centred anchors would snap it to a whole pixel)
        anchors.verticalCenter: root.scaleLabel ? undefined : valueArea.verticalCenter
        anchors.alignWhenCentered: !root.scaleLabel
        y: (root.height - root.noteHeight - root.px(17)) / 2 + root.px(14)
           - VLCStyle.redesign.centeredBaseline(settingLabel.font.pixelSize) - height / 2
        spacing: root.px(root.infoGap)

        Text {
            id: settingLabel
            // From the room left of the control only: a width taken from the
            // Row (which sums its children) could settle on 0 and hide the label
            // Figma's text box: the advance of Inter 3.19 rounded up (measured
            // 16 times larger, less the unit per glyph the bundled font adds)
            width: Math.max(0, Math.min(Math.ceil(labelExact.advanceWidth / 16
                                                  - text.length * font.pixelSize / 2816 - 0.001),
                                        valueArea.x - root.px(20)
                                        - (helpIcon.visible ? helpIcon.width + labelRow.spacing : 0)))
            anchors.baseline: parent.verticalCenter
            anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
            text: root.label
            color: root.c("#8F8F8F")
            elide: Text.ElideRight
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(14)

            HoverHandler { id: labelHover }
            TextMetrics {
                id: labelExact
                font.family: settingLabel.font.family
                font.weight: settingLabel.font.weight
                font.pixelSize: settingLabel.font.pixelSize * 16
                text: settingLabel.text
            }
        }

        Image {
            id: helpIcon
            visible: root.showInfo
            anchors.verticalCenter: parent.verticalCenter
            width: root.px(18)
            height: width
            sourceSize: Qt.size(width, height)
            source: SVGColorImage.colorize("qrc:///redesign/duo/info-small.svg")
                                 .color1(root.c("#404040")).uri()

            HoverHandler { id: helpHover }
        }
    }

    Text {
        id: noteText
        visible: root.note.length > 0
        y: root.height - root.noteHeight
        width: Math.max(0, valueArea.x - root.px(20))
        text: root.note
        wrapMode: Text.WordWrap
        maximumLineCount: 2
        elide: Text.ElideRight
        color: root.noteWarning ? root.c("#E4A580") : root.c("#6B6B6B")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(12)
        Accessible.role: root.noteWarning ? Accessible.AlertMessage : Accessible.StaticText
        Accessible.name: text
    }

    ToolTip {
        parent: helpIcon.visible ? helpIcon : settingLabel
        visible: (labelHover.hovered || helpHover.hovered) && root.help.length > 0
        text: root.help
        delay: 650
    }

    Item {
        id: valueArea
        anchors.right: parent.right
        y: root.controlTop !== undefined ? root.px(root.controlTop)
                                         : Math.round((root.height - root.noteHeight - height) / 2)
        width: root.useToggle ? root.px(68) : Math.min(root.px(320), root.width * 0.46)
        height: root.useStartupVolume ? root.px(78) : root.useToggle || root.useColor ? root.px(32)
                : root.useSlider && root.sliderScale ? root.px(57)
                : root.useText && root.fieldHeight > 0 ? root.px(root.fieldHeight) : root.px(36)

        // Only the editor of the row is created (a page has dozens of rows)
        Loader {
            anchors.fill: parent
            active: root.useStartupVolume
            sourceComponent: SettingsStartupVolume {
                anchors.fill: parent
                remember: !!root.settingValue
                volume: Number(root.otherValue === undefined ? 1 : root.otherValue)
                onChosen: (remember) => root.commit(remember)
                onVolumeEdited: (volume) => root.commitOther(volume)
            }
        }
        Loader {
            anchors.fill: parent
            active: root.useToggle
            sourceComponent: SettingsToggle {
                objectName: "settingToggle_" + root.settingKey
                anchors.fill: parent
                // OFF while it does not apply (its row dimmed), as the design
                checked: root.available && (root.editorKind === "inttoggle" ? Number(root.settingValue) !== 0
                                                                            : !!root.settingValue)
                onSwitched: (on) => root.commit(root.editorKind === "inttoggle" ? (on ? (Number(root.onValue) || 1) : 0) : on)
                Accessible.name: root.accessibleLabel
            }
        }
        Loader {
            anchors.fill: parent
            active: root.useChoices
            sourceComponent: SettingsDropdown {
                id: choiceControl
                objectName: "settingChoice_" + root.settingKey
                anchors.fill: parent
                model: root.editorKind === "boolchoice" ? root.choices : root.choices
                function valueIndex() {
                    for (let i = 0; i < root.choices.length; ++i) {
                        const value = root.choices[i].value
                        // Floats (volume step) may differ in their last digits
                        if (value === root.settingValue
                                || (typeof value === "number" && typeof root.settingValue === "number"
                                    && Math.abs(value - root.settingValue) < 1e-4))
                            return i
                    }
                    return -1
                }
                currentIndex: valueIndex()
                onActivated: (index) => root.commit(root.choices[index].value)
                Accessible.name: root.accessibleLabel
            }
        }
        Loader {
            anchors.fill: parent
            active: root.useSlider
            sourceComponent: Item {
                SettingsSlider {
                    objectName: "settingRange_" + root.settingKey
                    anchors.left: parent.left
                    anchors.right: root.sliderScale ? parent.right : rangeValue.left
                    anchors.rightMargin: root.sliderScale ? 0 : root.px(10)
                    anchors.verticalCenter: root.sliderScale ? undefined : parent.verticalCenter
                    anchors.top: root.sliderScale ? parent.top : undefined
                    from: root.sliderScale ? root.sliderScale.from : Number(root.minimum || 0)
                    to: root.sliderScale ? root.sliderScale.to : Number(root.maximum || 1)
                    ticks: root.sliderScale ? root.sliderScale.ticks : []
                    // 50 under the top of the slider in the design (measured on the
                    // speed, maximum volume and text opacity rows)
                    labelsBaseline: root.labelsBaseline !== undefined ? root.px(root.labelsBaseline) : root.px(50)
                    // the speed slider of the design keeps its end labels 2 px in
                    labelsInset: root.settingKey === "rate" ? root.px(2) : 0
                    labels: root.sliderScale ? root.sliderScale.labels : []
                    readonly property var sensitivityPixels: [12, 3, 1]
                    // the opacities of VLC (0..255) in percent
                    readonly property real opacityRange: Number(root.maximum) - Number(root.minimum)
                    value: root.editorKind === "sensitivity"
                           ? (Number(root.settingValue) >= 8 ? 0 : Number(root.settingValue) >= 2 ? 1 : 2)
                           : root.editorKind === "opacity" && opacityRange > 0
                           ? Math.round((Number(root.settingValue) - Number(root.minimum)) * 100 / opacityRange)
                           : Number(root.settingValue)
                    stepSize: root.settingKey === "rate" ? 0.25
                              : root.editorKind === "sensitivity" ? 1
                              : root.settingKey === "mouse-hide-timeout"
                                || root.settingKey === "redesign/notification-duration"
                                || root.settingKey === "osd-duration" ? 500
                              : root.settingKey === "video-title-timeout" ? 100
                              : root.settingType === "integer" ? 1 : 0.01
                    onMoved: root.commit(root.editorKind === "sensitivity" ? sensitivityPixels[Math.round(value)]
                                         : root.editorKind === "opacity" && opacityRange > 0
                                           ? Number(root.minimum) + Math.round(Math.round(value) * opacityRange / 100)
                                         : root.settingType === "integer" ? Math.round(value) : value)
                    Accessible.name: root.accessibleLabel
                }
                Text {
                    id: rangeValue
                    visible: root.useSlider && !root.sliderScale
                    anchors.right: parent.right
                    anchors.baseline: parent.verticalCenter
                    anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
                    width: root.px(44)
                    text: root.editorKind === "percent" ? Math.round(100 * Number(root.settingValue)) + "%"
                          : root.settingType === "integer" ? String(root.settingValue)
                          : Number(root.settingValue).toFixed(2)
                    color: root.c("#8F8F8F")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(14)
                    horizontalAlignment: Text.AlignRight
                }
            }
        }
        Loader {
            anchors.fill: parent
            active: root.useText
            sourceComponent: Item {
                SettingsTextField {
                    id: textControl
                    objectName: "settingInput_" + root.settingKey
                    anchors.left: parent.left
                    anchors.right: root.useFile ? browseButton.left : parent.right
                    anchors.rightMargin: root.useFile ? root.px(10) : 0
                    anchors.verticalCenter: parent.verticalCenter
                    height: root.fieldHeight > 0 ? root.px(root.fieldHeight) : implicitHeight
                    // "3 s": the space is the gap before the unit (none after
                    // the name of the default)
                    suffix: root.defaultDisplay.length > 0 && text === root.defaultDisplay ? "" : root.unit
                    placeholderText: root.placeholder
                    // what VLC uses while the field is empty ("System proxy"):
                    // the design draws it as a value
                    placeholderTextColor: root.c("#8F8F8F")
                    // Figma 6543:15649: the fields of a setting that does
                    // not apply (the buffering of a preset) are empty
                    text: !root.available || (!root.valueShown && !activeFocus) ? ""
                          : root.displayValue.length > 0 && !activeFocus ? root.displayValue
                          : root.defaultDisplay.length > 0 && !activeFocus
                            && String(root.settingValue) === String(root.onValue) ? root.defaultDisplay
                          : root.editorKind === "volumepercent" ? String(Math.round(Number(root.settingValue) * 100 / 256))
                          // a track counted from 1 (VLC's from 0, -1 its choice)
                          : root.editorKind === "track" ? (Number(root.settingValue) >= 0 ? String(Number(root.settingValue) + 1) : "")
                          : root.settingType === "float" && root.decimals !== undefined
                            ? Number(root.settingValue).toFixed(root.decimals)
                          : root.settingValue === undefined || root.settingValue === null
                          || (root.defaultText.length > 0 && String(root.settingValue).length === 0)
                          ? root.defaultText : root.settingType === "float"
                          ? String(Number(Number(root.settingValue).toFixed(6))) : String(root.settingValue)
                    inputMethodHints: root.settingType === "integer" || root.settingType === "float"
                                      ? Qt.ImhFormattedNumbersOnly : Qt.ImhNone
                    echoMode: root.editorKind === "password" ? TextInput.Password : TextInput.Normal
                    onEditingFinished: {
                        // the empty field of a setting that does not apply
                        if (!root.available || (!root.valueShown && !activeFocus && !text.length))
                            return
                        if (root.defaultDisplay.length > 0 && text === root.defaultDisplay)
                            return
                        if (root.displayValue.length > 0 && text === root.displayValue)
                            return
                        // the default shown in an empty field is not a new value
                        if (text === root.defaultText && !String(root.settingValue || "").length)
                            return
                        if (root.editorKind === "track") {
                            const track = text.trim().length ? parseInt(text, 10) : 0
                            const value = track >= 1 ? track - 1 : -1
                            if (!isNaN(track) && value !== Number(root.settingValue))
                                root.commit(value)
                            return
                        }
                        if (root.editorKind === "volumepercent") {
                            const percent = parseFloat(text)
                            if (!isNaN(percent) && percent > 0)
                                root.commit(percent * 256 / 100)
                            return
                        }
                        if (text !== String(root.settingValue))
                            root.commit(text)
                    }
                    Accessible.name: root.accessibleLabel
                }
                SettingsButton {
                    id: browseButton
                    visible: root.useFile
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Browse")
                    onClicked: root.browseRequested()
                }
            }
        }
        // Color field of the design: 320 x 32, #090909, radius 6, the value
        // 12 px in, the 22 x 22 color (radius 2, 1.5 px of white at 21 %)
        // 5 px from the right; a click on it picks another one
        Loader {
            anchors.fill: parent
            active: root.useColor
            sourceComponent: Rectangle {
                anchors.fill: parent
                radius: root.px(6)
                color: root.c("#090909")
                border.width: colorInput.activeFocus ? 1 : 0
                border.color: VLCStyle.redesign.accentDim
                readonly property string hex: "#" + ("000000" + Number(root.settingValue || 0).toString(16)).slice(-6).toUpperCase()

                TextInput {
                    id: colorInput
                    objectName: "settingColor_" + root.settingKey
                    x: root.px(12)
                    width: parent.width - x - root.px(5 + 22 + 10)
                    anchors.baseline: parent.verticalCenter
                    anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
                    text: parent.hex
                    color: root.c("#8F8F8F")
                    selectionColor: VLCStyle.redesign.accentBackground
                    selectedTextColor: VLCStyle.redesign.accent
                    selectByMouse: true
                    clip: true
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(14)
                    validator: RegularExpressionValidator { regularExpression: /#?[0-9A-Fa-f]{0,6}/ }
                    onEditingFinished: {
                        const rgb = parseInt(text.replace(/^#/, ""), 16)
                        if (text.replace(/^#/, "").length === 6 && !isNaN(rgb))
                            root.commit(rgb)
                        text = Qt.binding(() => parent.hex)
                    }
                    Accessible.name: root.accessibleLabel
                }

                Rectangle {
                    anchors.right: parent.right
                    anchors.rightMargin: root.px(5)
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.px(22)
                    height: width
                    radius: root.px(2)
                    color: parent.hex
                    // Figma's inside stroke of 1.5 over the fill (a Rectangle's
                    // own border leaves the fill out: the field showed through
                    // it; its width is whole pixels): a ring of 1, then the
                    // half pixel as a ring of 1 at half the opacity
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: "transparent"
                        border.width: root.px(1)
                        border.color: Qt.rgba(1, 1, 1, 0.21)
                    }
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: root.px(1)
                        radius: Math.max(0, parent.radius - root.px(1))
                        color: "transparent"
                        border.width: root.px(1)
                        border.color: Qt.rgba(1, 1, 1, 0.105)
                    }
                    TapHandler { onTapped: root.colorRequested() }
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Choose a color")
                }
            }
        }
        Loader {
            anchors.fill: parent
            active: root.useSegmented
            sourceComponent: SettingsSegmented {
                anchors.fill: parent
                options: root.useSegmented ? root.choices.map(choice => ({ text: choice.label, value: choice.value })) : []
                value: root.settingValue
                onChosen: (value) => root.commit(value)
                Accessible.name: root.accessibleLabel
            }
        }
    }
}

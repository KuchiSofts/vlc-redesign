/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later. */
import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Controls.impl as Impl
import VLC.MainInterface
import VLC.Player
import VLC.Style
import VLC.Widgets as Widgets

// Figma "Audio Visualizer" (2372:8639), the dropdown of the settings: the
// field with the visualization in use (360 x 48: #090909 around #191919),
// 20 under it the list (padding 2, rows of 44 two apart, the icon of each
// visualization, #1F1F1F under the one chosen with the check of the accent
// 13 from its end), the drop shadow of the menus. VLC offers more than the
// design draws: the list scrolls past the height of the window.
Controls.Popup {
    id: root
    objectName: "audioVisualizationDialog"
    // the visualization in use, in the language of the interface (again
    // in another one chosen meanwhile): its row, the name of a module
    property int _selectedIndex: 0
    property string _selectedName: ""
    readonly property string selectedText: _selectedIndex <= 0 ? qsTr("Disabled") : _selectedName
    property bool previousShortcuts: true

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(dark, light) { return VLCStyle.redesign.color(dark, light) }

    width: Math.min(px(360), parent.width - 32)
    padding: 0
    focus: true
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside
    // a press outside only closes it: the video under it does not pause
    modal: true
    dim: false
    onOpened: { previousShortcuts = MainCtx.useGlobalShortcuts; MainCtx.useGlobalShortcuts = false }
    onClosed: MainCtx.useGlobalShortcuts = previousShortcuts
    background: null
    contentItem: Column {
        // right to left (Hebrew, Arabic...): a popup is not under the
        // mirrored interface, its content mirrors itself
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        spacing: root.px(20)

        // the field: the visualization in use
        Rectangle {
            width: parent.width
            height: root.px(48)
            radius: root.px(6)
            color: root.c("#090909", "#dedede")
            Rectangle {
                anchors.fill: parent
                anchors.margins: root.px(2)
                radius: root.px(4)
                color: root.c("#191919", "#ababaa")
            }
            // (the text from the right, the arrow on the left in a right
            // to left layout)
            Text {
                anchors.left: parent.left
                anchors.leftMargin: root.px(16)
                anchors.right: parent.right
                anchors.rightMargin: root.px(16 + 24 + 10)
                height: parent.height
                horizontalAlignment: Text.AlignLeft
                text: root.selectedText
                color: root.c("#8F8F8F", "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(14)
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            Image {
                anchors.right: parent.right
                anchors.rightMargin: root.px(2 + 14)
                anchors.verticalCenter: parent.verticalCenter
                width: root.px(24)
                height: width
                sourceSize: Qt.size(width, height)
                smooth: !VLCStyle.redesign.svgExact(width, height, Screen.devicePixelRatio)
                source: "qrc:///redesign/icons/chevron-down/default.svg"
            }
        }

        // the list
        Rectangle {
            readonly property real rowsHeight: options.count > 0
                                               ? options.count * root.px(46) - root.px(2) : root.px(44)
            // the window's height less the field, the gap and a margin
            readonly property real roomHeight: (root.parent ? root.parent.height : rowsHeight)
                                               - root.px(48 + 20 + 48) - 2 * root.px(2)
            width: parent.width
            height: Math.min(rowsHeight, Math.max(root.px(44), roomHeight)) + 2 * root.px(2)
            radius: root.px(6)
            color: root.c("#191919", "#ababaa")

            // Figma: drop shadow 0 8 blur 15 #000000 46 % (as the menus)
            Widgets.RoundedRectangleShadow {
                yOffset: root.px(8)
                blurRadius: 7.5 * VLCStyle.scale
                color: Qt.rgba(0, 0, 0, 0.46)
            }

            // the frame of the main window
            Widgets.RedesignWindowBorder { }

            ListView {
                id: options
                objectName: "audioVisualizationModes"
                x: root.px(2)
                y: root.px(2)
                width: parent.width - 2 * x
                height: parent.height - 2 * y
                spacing: root.px(2)
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: Player.audioVisualization
                Controls.ScrollBar.vertical: Controls.ScrollBar {
                    // Basic's handle is light grey: darker in the light theme
                    palette.mid: VLCStyle.palette.isDark ? undefined : "#6b6b6b"
                    palette.dark: VLCStyle.palette.isDark ? undefined : "#2e2e2e"
                }
                delegate: Controls.ItemDelegate {
                    id: row
                    required property int index
                    required property var model
                    checked: model.checked
                    width: options.width
                    height: root.px(44)
                    // the icons of the design (Figma 2372:8639), in the
                    // order of the choices of the "visual" variable
                    // (aout_New: Disable, Spectrometer, Scope, Spectrum,
                    // Waveform, VU meter, then the modules); the VU meter,
                    // not drawn there, with the gauge of the icon set
                    readonly property string iconDir: index >= 1 && index <= 5
                        ? "qrc:///redesign/icons/" + ["spectrometer", "scope", "spectrum", "waveform", "vumeter"][index - 1] : ""
                    // the icon, 10 before the name (the other way round in a
                    // right to left layout)
                    readonly property real _startPadding: iconDir ? root.px(14 + 24 + 10) : root.px(14)
                    readonly property real _endPadding: root.px(14 + 24 + 13)
                    leftPadding: mirrored ? _endPadding : _startPadding
                    rightPadding: mirrored ? _startPadding : _endPadding
                    text: index === 0 ? qsTr("Disabled") : model.display
                    function _choose() {
                        root._selectedIndex = index
                        root._selectedName = model.display ?? ""
                    }
                    Component.onCompleted: if (checked) _choose()
                    onCheckedChanged: if (checked) _choose()
                    onClicked: Player.audioVisualization.toggleIndex(index)
                    Accessible.name: text
                    background: Rectangle {
                        radius: root.px(4)
                        color: row.checked || row.hovered || row.visualFocus
                               ? root.c("#1F1F1F", "#9c9c9b") : "transparent"
                        Image {
                            id: modeIcon
                            visible: row.iconDir !== ""
                            anchors.left: parent.left
                            anchors.leftMargin: root.px(14)
                            anchors.verticalCenter: parent.verticalCenter
                            width: root.px(24)
                            height: width
                            sourceSize: Qt.size(width, height)
                            smooth: !VLCStyle.redesign.svgExact(width, height, Screen.devicePixelRatio)
                            // drawn in grey and the accent dim: with another
                            // accent, the grey part and the rest tinted above it
                            readonly property bool twoTone: visible && !VLCStyle.redesign.defaultAccent
                            source: visible ? row.iconDir + (twoTone ? "/default-grey.svg" : "/default.svg") : ""
                            Impl.ColorImage {
                                // drawn at the size shown (an SVG: sharp at any scale and zoom)
                                sourceSize: Qt.size(width, height)
                                smooth: !VLCStyle.redesign.svgExact(width, height, Screen.devicePixelRatio)
                                anchors.fill: parent
                                visible: modeIcon.twoTone
                                source: visible ? row.iconDir + "/default-accent.svg" : ""
                                color: VLCStyle.redesign.accentDim
                            }
                        }
                        Impl.ColorImage {
                            // drawn at the size shown (an SVG: sharp at any scale and zoom)
                            sourceSize: Qt.size(width, height)
                            smooth: !VLCStyle.redesign.svgExact(width, height, Screen.devicePixelRatio)
                            visible: row.checked
                            anchors.right: parent.right
                            anchors.rightMargin: root.px(13)
                            anchors.verticalCenter: parent.verticalCenter
                            width: root.px(24)
                            height: width
                            source: "qrc:///redesign/track-check-active.svg"
                            color: VLCStyle.redesign.accentTint
                        }
                    }
                    contentItem: Text {
                        text: row.text
                        color: root.c("#8F8F8F", "#2e2e2e")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(14)
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }
                Text {
                    anchors.fill: parent
                    anchors.leftMargin: root.px(14)
                    anchors.rightMargin: root.px(14)
                    visible: options.count === 0
                    text: qsTr("Play audio to select a visualization.")
                    color: root.c(VLCStyle.redesign.textSecondary, "#4d4d4d")
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(14)
                }
            }
        }
    }
}

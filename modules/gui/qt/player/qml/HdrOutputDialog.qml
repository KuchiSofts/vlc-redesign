/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later. */
import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Controls.impl as Impl
import VLC.Dialogs
import VLC.MainInterface
import VLC.Style
import VLC.Widgets as Widgets

// Figma "HDR Options" (2372:8733), the dropdown of the settings: the field
// with the mode in use (360 x 48: #090909 around #191919), 20 under it the
// list (padding 2, rows of 44 two apart, #1F1F1F under the mode chosen with
// the check of the accent 13 from its end), the drop shadow of the menus
Controls.Popup {
    id: root
    objectName: "hdrOutputDialog"
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
    HdrOutput { id: output; ctx: MainCtx }
    background: null
    contentItem: Column {
        // right to left (Hebrew, Arabic...): a popup is not under the
        // mirrored interface, its content mirrors itself
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        spacing: root.px(20)

        // the field: the mode in use
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
                text: output.selectedIndex >= 0 ? output.choices[output.selectedIndex].label : qsTr("HDR unavailable")
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
            width: parent.width
            height: choices.height + 2 * root.px(2)
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

            Column {
                id: choices
                x: root.px(2)
                y: root.px(2)
                width: parent.width - 2 * x
                spacing: root.px(2)

                Repeater {
                    model: output.choices
                    // the reason of a choice that cannot be chosen shows on
                    // hover: a disabled control gets no hover itself, the
                    // row around it does
                    delegate: Item {
                        id: choice
                        required property int index
                        required property var modelData
                        readonly property bool chosen: output.selectedIndex === index
                        width: choices.width
                        height: root.px(44)
                        HoverHandler { id: choiceHover }
                        Controls.ToolTip.visible: choiceHover.hovered && modelData.reason.length > 0
                        Controls.ToolTip.text: modelData.reason
                        Controls.ToolTip.delay: 300
                        Controls.ItemDelegate {
                            id: row
                            anchors.fill: parent
                            enabled: choice.modelData.enabled
                            // (the check on the left in a right to left layout)
                            leftPadding: mirrored ? root.px(14 + 24 + 13) : root.px(14)
                            rightPadding: mirrored ? root.px(14) : root.px(14 + 24 + 13)
                            onClicked: output.select(choice.index)
                            Accessible.name: choice.modelData.label
                            background: Rectangle {
                                radius: root.px(4)
                                color: choice.chosen || row.hovered || row.visualFocus
                                       ? root.c("#1F1F1F", "#9c9c9b") : "transparent"
                                Impl.ColorImage {
                                    // drawn at the size shown (an SVG: sharp at any scale and zoom)
                                    sourceSize: Qt.size(width, height)
                                    smooth: !VLCStyle.redesign.svgExact(width, height, Screen.devicePixelRatio)
                                    visible: choice.chosen
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
                                text: choice.modelData.label
                                opacity: row.enabled ? 1 : 0.45
                                color: root.c("#8F8F8F", "#2e2e2e")
                                font.family: VLCStyle.redesign.fontFamily
                                font.pixelSize: root.px(14)
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }

        // what the modes need (not in the design: under its list)
        Text {
            width: parent.width
            visible: text.length > 0
            text: output.message
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            color: root.c("#6B6B6B", "#6b6b6b")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(12)
        }
    }
}

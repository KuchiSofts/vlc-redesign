/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.Style
import VLC.Widgets as Widgets

// "Report a problem" (⋯ menu, About): the text that goes into the link to
// the bug form of GitHub, to copy or to open there. Nothing is sent from
// here: the user reads it, and submits the form in the browser.
Widgets.RedesignDialog {
    id: root

    objectName: "reportDialog"

    property string report: ""
    property bool _copied: false

    title: qsTr("Report a problem")
    preferredWidth: VLCStyle.dp(640, VLCStyle.scale)
    acceptText: qsTr("Open GitHub")
    extraText: _copied ? qsTr("Copied") : qsTr("Copy")

    // read again at each opening (a screen, the theme may have changed)
    onAboutToShow: {
        _copied = false
        report = RedesignFeedback.reportText(MainCtx.colorScheme.scheme, VLCStyle.palette.isDark,
                                             VLCStyle.redesign.accent.toString())
    }
    onExtraClicked: {
        RedesignFeedback.copyText(report)
        _copied = true
        copiedTimer.restart()
    }
    onAccepted: Qt.openUrlExternally(RedesignFeedback.reportUrl(report))

    Timer {
        id: copiedTimer
        interval: 2000
        onTriggered: root._copied = false
    }

    Widgets.RedesignDialogHeading {
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        text: qsTr("What the report contains")
        description: qsTr("GitHub opens with this filled in. Nothing is sent until you submit it there.")
    }

    // The text of the report: in English, from the left in every language
    // (as the field of the form shows it)
    Rectangle {
        LayoutMirroring.enabled: false
        LayoutMirroring.childrenInherit: true

        Layout.fillWidth: true
        implicitHeight: VLCStyle.dp(240, VLCStyle.scale)
        radius: VLCStyle.dp(6, VLCStyle.scale)
        color: VLCStyle.redesign.color("#090909", "#dedede", root.darkTheme)
        border.width: VLCStyle.dp(2, VLCStyle.scale)
        border.color: VLCStyle.redesign.color("#191919", "#c4c4c3", root.darkTheme)

        Flickable {
            id: reportView

            anchors.fill: parent
            anchors.margins: VLCStyle.dp(12, VLCStyle.scale)
            contentWidth: width
            contentHeight: reportText.implicitHeight
            clip: true
            interactive: contentHeight > height
            boundsBehavior: Flickable.StopAtBounds

            T.ScrollBar.vertical: Widgets.ScrollBarExt { }

            TextEdit {
                id: reportText

                width: reportView.width
                readOnly: true
                selectByMouse: true
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                horizontalAlignment: TextEdit.AlignLeft
                text: root.report
                color: VLCStyle.redesign.color("#C6C6C6", "#2e2e2e", root.darkTheme)
                selectionColor: VLCStyle.redesign.color(VLCStyle.redesign.accentBackground, "#dedede", root.darkTheme)
                selectedTextColor: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#191919", root.darkTheme)
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
                Accessible.role: Accessible.EditableText
                Accessible.name: qsTr("Report")
            }
        }
    }

    Text {
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignLeft
        text: qsTr("No file names, user names or logs. To add a log: Settings → General → Diagnostics → Save a log.")
        color: VLCStyle.redesign.color("#6B6B6B", "#4d4d4d", root.darkTheme)
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
        lineHeight: 1.3
    }
}

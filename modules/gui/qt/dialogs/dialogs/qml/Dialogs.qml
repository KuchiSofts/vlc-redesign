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
import QtQuick.Layouts
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.Dialogs
import VLC.Widgets as Widgets
import VLC.Util
import VLC.Style

Item {
    id: root

    onVisibleChanged: if (visible && VLCStyle.customizeRequested) editorOpenTimer.restart()

    //---------------------------------------------------------------------------------------------
    // Properties
    //---------------------------------------------------------------------------------------------

    property Item bgContent: null

    //---------------------------------------------------------------------------------------------
    // Signal
    //---------------------------------------------------------------------------------------------

    signal restoreFocus();

    //---------------------------------------------------------------------------------------------
    // Events
    //---------------------------------------------------------------------------------------------

    Component.onCompleted: {
        if (DialogErrorModel.repeatedMessageCount && MainCtx.showErrorDialogs) {
            hideErrorPopupTimer.restart()
            errorPopup.state = "visible"
        }
        // Customize Player asked from the library (onShowToolbarEditorDialog)
        if (visible && VLCStyle.customizeRequested)
            editorOpenTimer.restart()
    }

    Component.onDestruction: {
        if (questionDialog.dialogId !== null) {
            vlcDialog.dismiss(questionDialog.dialogId)
            questionDialog.dialogId = null
        } if (loginDialog.dialogId !== null) {
            vlcDialog.dismiss(loginDialog.dialogId)
            loginDialog.dialogId = null
        }
    }

    //---------------------------------------------------------------------------------------------
    // Functions
    //---------------------------------------------------------------------------------------------

    function ask(text, acceptCb, rejectCb, buttons) {
        customDialog.ask(text, acceptCb, rejectCb, buttons)
    }

    //---------------------------------------------------------------------------------------------
    // Connections
    //---------------------------------------------------------------------------------------------

    VLCDialog
    {
        id: vlcDialog

        Component.onCompleted: {
            VLCDialogModel.provider = vlcDialog
        }

        Component.onDestruction: {
            // Only clear the provider if we are still the current one.
            if (VLCDialogModel.provider === vlcDialog)
                VLCDialogModel.provider = null
        }

        onLogin: (dialogId, title, text, defaultUsername, askStore) => {
            loginDialog.dialogId = dialogId
            loginDialog.title = title
            loginDialog.defaultUsername = defaultUsername
            loginDialog.text = text
            loginDialog.askStore = askStore
            loginDialog.open()
        }

        onQuestion: (dialogId, title, text, type, cancel, action1, action2) => {
            questionDialog.dialogId = dialogId
            questionDialog.title = title
            questionDialog.text = text
            questionDialog.cancelTxt = cancel
            questionDialog.action1Txt = action1
            questionDialog.action2Txt = action2
            questionDialog.open()
        }

        onProgress: (dialogId, title, text, indeterminate, position, cancel) => {
            progressDialog.dialogId = dialogId
            progressDialog.title = title
            progressDialog.text = text
            progressDialog.cancelTxt = cancel
            progressDialog.position = position
            progressDialog.interderminate = indeterminate
            progressDialog.open()
        }

        onProgressUpdated: (dialogId, position, text) => {
            if (progressDialog.dialogId !== dialogId) {
                console.warn("progress event on an inexisting dialog")
                return
            }
            progressDialog.text = text
            progressDialog.position = position
        }

        // (the id let go first: closing would dismiss it again)
        onCancelled: (dialogId) => {
            if (questionDialog.dialogId === dialogId) {
                questionDialog.dialogId = null
                questionDialog.close()
            } else if (loginDialog.dialogId === dialogId)  {
                loginDialog.dialogId = null
                loginDialog.close()
            } else if (progressDialog.dialogId === dialogId) {
                progressDialog.dialogId = null
                progressDialog.close()
            }
            dismiss(dialogId)
        }
    }

    Connections
    {
        target: DialogErrorModel

        function onCountChanged() {
            // "Show confirmation and error dialogs" off: only kept in the
            // messages, no popup nor badge
            if (!MainCtx.showErrorDialogs) {
                DialogErrorModel.resetRepeatedMessageCount()
                return
            }
            hideErrorPopupTimer.restart()
            errorPopup.state = "visible"
        }
    }

    //---------------------------------------------------------------------------------------------
    // Childs
    //---------------------------------------------------------------------------------------------

    Widgets.DrawerExt {
        id: errorPopup

        property string messageText
        property int repeatedMessageCount: 0

        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            bottomMargin: VLCStyle.margin_small
        }

        edge: Widgets.DrawerExt.Edges.Bottom
        width: contentItem?.layoutWidth ?? 0
        height: contentItem?.height ?? 0
        z: 10

        // As the panels of the redesign: "Dark/BG/A4" at 95 %, radius 14,
        // the shadow of the menus and dialogs; the message in Inter 14, the
        // details as the action of the dialogs (Accent Bright and the
        // accent), the close button of their title bar
        component: Rectangle {
            color: VLCStyle.redesign.color(Qt.rgba(0x19 / 255, 0x19 / 255, 0x19 / 255, 0.95), "#ABABAA")

            height: VLCStyle.dp(52, VLCStyle.scale)
            radius: VLCStyle.dp(14, VLCStyle.scale)

            property real layoutWidth: layout.implicitWidth + 2 * VLCStyle.dp(10, VLCStyle.scale)

            Accessible.role: Accessible.AlertMessage
            Accessible.name: qsTr("error popup")

            Widgets.RoundedRectangleShadow {
                yOffset: VLCStyle.dp(8, VLCStyle.scale)
                blurRadius: 7.5 * VLCStyle.scale
                color: Qt.rgba(0, 0, 0, 0.46)
            }

            // the frame of the main window
            Widgets.RedesignWindowBorder { }

            RowLayout {
                id: layout

                // a popup of its own: right to left, the message on the right
                LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
                LayoutMirroring.childrenInherit: true

                anchors.verticalCenter: parent.verticalCenter
                x: VLCStyle.dp(10, VLCStyle.scale)
                spacing: VLCStyle.dp(8, VLCStyle.scale)

                T.Label {
                    id: messageText

                    Layout.maximumWidth: root.width * 0.5
                    Layout.leftMargin: VLCStyle.dp(10, VLCStyle.scale)
                    Layout.rightMargin: VLCStyle.dp(8, VLCStyle.scale)

                    text: (DialogErrorModel.repeatedMessageCount > 1 ? '[' + DialogErrorModel.repeatedMessageCount + '] ' : '')
                          + DialogErrorModel.notificationText

                    elide: Text.ElideRight
                    maximumLineCount: 1
                    horizontalAlignment: Text.AlignLeft
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    color: VLCStyle.redesign.color("#C6C6C6", "#2e2e2e")
                }

                T.Button {
                    id: detailsBtn

                    text: qsTr("Show Details")
                    implicitWidth: Math.max(VLCStyle.dp(100, VLCStyle.scale),
                                            detailsText.implicitWidth + 2 * VLCStyle.dp(16, VLCStyle.scale))
                    implicitHeight: VLCStyle.dp(32, VLCStyle.scale)
                    Accessible.name: text

                    contentItem: Text {
                        id: detailsText
                        text: detailsBtn.text
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: VLCStyle.redesign.accent
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                        font.weight: Font.DemiBold
                    }
                    background: Rectangle {
                        radius: VLCStyle.dp(6, VLCStyle.scale)
                        color: detailsBtn.down || detailsBtn.hovered
                               ? Qt.lighter(VLCStyle.redesign.accentBackground, 1.15)
                               : VLCStyle.redesign.accentBackground
                        border.width: detailsBtn.visualFocus ? 1 : 0
                        border.color: VLCStyle.redesign.focusColor
                    }

                    onClicked: {
                        hideErrorPopupTimer.stop()
                        errorPopup.state = "hidden"
                        DialogErrorModel.resetRepeatedMessageCount()
                        DialogsProvider.messagesDialog(1)
                    }
                }

                T.Button {
                    id: closeBtn

                    implicitWidth: VLCStyle.dp(32, VLCStyle.scale)
                    implicitHeight: VLCStyle.dp(32, VLCStyle.scale)
                    padding: VLCStyle.dp(4, VLCStyle.scale)
                    Accessible.name: qsTr("Dismiss")

                    background: Rectangle {
                        radius: VLCStyle.dp(4, VLCStyle.scale)
                        color: (closeBtn.hovered || closeBtn.down)
                               ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
                               : "transparent"
                        border.width: closeBtn.visualFocus ? 1 : 0
                        border.color: VLCStyle.redesign.focusColor
                    }
                    contentItem: Image {
                        source: "qrc:///redesign/icons/close/default.svg"
                        sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                        fillMode: Image.Pad
                        smooth: false
                    }

                    onClicked: {
                        hideErrorPopupTimer.stop()
                        errorPopup.state = "hidden"
                        DialogErrorModel.resetRepeatedMessageCount()
                    }
                }
            }
        }

        state: "hidden"

        Timer {
            id: hideErrorPopupTimer
            interval: 5000
            repeat: false
            onTriggered: {
                errorPopup.state = "hidden"
                DialogErrorModel.resetRepeatedMessageCount()
            }
        }
    }

    // The dialogs the core asks for (vlc_dialog: a login, a question, a
    // progress), as the other dialogs of the redesign (RedesignDialog)

    // a text of the core, not markup (a path, an address)
    function plainText(text) {
        return (text || "").replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
                           .replace(/\n/g, "<br>")
    }

    // a label over its field (Inter 14, "Dark/FG/A2")
    component FieldLabel: Text {
        Layout.fillWidth: true
        // (in a popup: not under the mirrored interface)
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        horizontalAlignment: Text.AlignLeft
        elide: Text.ElideRight
        color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
    }

    // The password: a field of one line as Widgets.RedesignTextArea (a text
    // area has no hidden echo)
    component PasswordField: T.TextField {
        id: passwordField

        readonly property real border: VLCStyle.dp(2, VLCStyle.scale)
        readonly property bool _mirrored: LayoutMirroring.enabled

        Layout.fillWidth: true
        Layout.topMargin: VLCStyle.dp(2, VLCStyle.scale)
        Layout.bottomMargin: VLCStyle.dp(2, VLCStyle.scale)
        implicitHeight: VLCStyle.dp(8 + 21 + 8, VLCStyle.scale)
        topPadding: border + VLCStyle.dp(8, VLCStyle.scale)
        bottomPadding: border + VLCStyle.dp(8, VLCStyle.scale)
        leftPadding: border + VLCStyle.dp(_mirrored ? 18 : 12, VLCStyle.scale)
        rightPadding: border + VLCStyle.dp(_mirrored ? 12 : 18, VLCStyle.scale)
        horizontalAlignment: TextInput.AlignLeft
        verticalAlignment: TextInput.AlignVCenter
        echoMode: TextInput.Password

        color: VLCStyle.redesign.color("#C6C6C6", "#2e2e2e")
        selectionColor: VLCStyle.redesign.color(VLCStyle.redesign.accentBackground, "#dedede")
        selectedTextColor: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#2e2e2e")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        selectByMouse: true

        background: Rectangle {
            radius: VLCStyle.dp(6, VLCStyle.scale)
            color: VLCStyle.redesign.color("#090909", "#dedede")
            border.width: passwordField.border
            border.color: VLCStyle.redesign.color("#191919", "#ababaa")
        }
    }

    Widgets.RedesignDialog {
        id: loginDialog

        objectName: "vlcLoginDialog"

        //use var here as DialogId is a QGadget and passed by value
        property var dialogId: null
        property string defaultUsername: ""
        property string text: ""
        property bool askStore: false

        acceptText: qsTr("Log in")
        preferredWidth: VLCStyle.dp(560, VLCStyle.scale)

        onAboutToShow: {
            username.text = loginDialog.defaultUsername
            password.text = ""
            savePassword.checked = false
        }
        onOpened: (username.text === "" ? username : password).forceActiveFocus()
        onAboutToHide: root.restoreFocus()
        onAccepted: {
            if (loginDialog.dialogId !== null) {
                const id = loginDialog.dialogId
                loginDialog.dialogId = null
                vlcDialog.post_login(id, username.text, password.text, savePassword.checked)
            }
        }
        // Cancel, the close button, Escape
        onClosed: {
            if (loginDialog.dialogId !== null) {
                const id = loginDialog.dialogId
                loginDialog.dialogId = null
                vlcDialog.dismiss(id)
            }
            password.text = ""
        }

        Widgets.RedesignDialogHeading {
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
            visible: loginDialog.text !== ""
            headingWraps: true
            text: root.plainText(loginDialog.text)
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: VLCStyle.dp(8, VLCStyle.scale)

            FieldLabel { text: qsTr("User") }

            Widgets.RedesignTextArea {
                id: username

                objectName: "vlcLoginUser"
                LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
                Accessible.name: qsTr("User")
                onAccepted: password.forceActiveFocus()
            }

            FieldLabel {
                Layout.topMargin: VLCStyle.dp(12, VLCStyle.scale)
                text: qsTr("Password")
            }

            PasswordField {
                id: password

                objectName: "vlcLoginPassword"
                LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
                Accessible.name: qsTr("Password")
                onAccepted: loginDialog.accept()
            }
        }

        // "Save password", as the toggles of the settings
        RowLayout {
            Layout.fillWidth: true
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
            visible: loginDialog.askStore
            spacing: VLCStyle.dp(20, VLCStyle.scale)

            FieldLabel {
                Layout.fillWidth: true
                text: qsTr("Save password")
            }

            Widgets.CheckButton {
                id: savePassword

                objectName: "vlcLoginSavePassword"
                Accessible.name: qsTr("Save password")
            }
        }
    }

    Widgets.RedesignDialog {
        id: progressDialog

        objectName: "vlcProgressDialog"

        property var dialogId: null

        property string text : ""
        property string cancelTxt: ""
        property bool interderminate: false
        property real position: 0.0

        preferredWidth: VLCStyle.dp(560, VLCStyle.scale)
        // Cancel only, when the core gives one: else it cannot be closed
        acceptVisible: false
        cancelVisible: cancelTxt !== ""
        cancelText: cancelTxt
        closeButtonVisible: cancelTxt !== ""
        closePolicy: cancelTxt !== "" ? T.Popup.CloseOnEscape : T.Popup.NoAutoClose

        onAboutToHide: root.restoreFocus()
        onClosed: {
            if (progressDialog.dialogId !== null) {
                const id = progressDialog.dialogId
                progressDialog.dialogId = null
                vlcDialog.dismiss(id)
            }
        }

        Widgets.RedesignDialogHeading {
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
            visible: progressDialog.text !== ""
            headingWraps: true
            text: root.plainText(progressDialog.text)
        }

        // the bar: a well of "Dark/BG/B2" filled with the accent (as the
        // seek bar); not known how far: a part of it going across
        Rectangle {
            id: progressWell

            Layout.fillWidth: true
            implicitHeight: VLCStyle.dp(8, VLCStyle.scale)
            radius: VLCStyle.dp(4, VLCStyle.scale)
            color: VLCStyle.redesign.color("#090909", "#dedede")
            clip: true

            Accessible.role: Accessible.ProgressBar
            Accessible.name: progressDialog.text

            Rectangle {
                id: progressFill

                height: parent.height
                radius: parent.radius
                color: VLCStyle.redesign.accent
                width: progressDialog.interderminate ? parent.width * 0.3
                                                     : parent.width * Math.max(0, Math.min(1, progressDialog.position))
                x: LayoutMirroring.enabled && !progressDialog.interderminate ? parent.width - width : 0

                SequentialAnimation on x {
                    running: progressDialog.visible && progressDialog.interderminate
                    loops: Animation.Infinite
                    NumberAnimation {
                        from: -progressFill.width
                        to: progressWell.width
                        duration: 1200
                        easing.type: Easing.InOutQuad
                    }
                }
            }
        }
    }

    Widgets.RedesignDialog {
        id: questionDialog

        objectName: "vlcQuestionDialog"

        //use var here as DialogId is a QGadget and passed by value
        property var dialogId: null
        property string text: ""
        property string cancelTxt: ""
        property string action1Txt: ""
        property string action2Txt: ""

        preferredWidth: VLCStyle.dp(560, VLCStyle.scale)
        // Cancel on the left, the first action on the right (in the accent),
        // the second one before it
        cancelVisible: cancelTxt !== ""
        cancelText: cancelTxt
        acceptVisible: action1Txt !== ""
        acceptText: action1Txt
        extraText: action2Txt

        onAboutToHide: root.restoreFocus()
        onAccepted: {
            if (questionDialog.dialogId !== null) {
                const id = questionDialog.dialogId
                questionDialog.dialogId = null
                vlcDialog.post_action1(id)
            }
        }
        onExtraClicked: {
            if (questionDialog.dialogId !== null) {
                const id = questionDialog.dialogId
                questionDialog.dialogId = null
                vlcDialog.post_action2(id)
            }
            questionDialog.close()
        }
        // Cancel, the close button, Escape
        onClosed: {
            if (questionDialog.dialogId !== null) {
                const id = questionDialog.dialogId
                questionDialog.dialogId = null
                vlcDialog.dismiss(id)
            }
        }

        Widgets.RedesignDialogHeading {
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
            headingWraps: true
            text: root.plainText(questionDialog.text)
        }
    }

    CustomDialog {
        id: customDialog
        rootWindow: root.bgContent
        onAboutToHide: restoreFocus()
    }

    Loader {
        id: toolbarEditorDialogLoader
        active: false
        source: "qrc:///qt/qml/VLC/Dialogs/ToolbarEditorDialog.qml"
        onLoaded: item.open()

        Connections {
            target: toolbarEditorDialogLoader.item
            function onUnload() { toolbarEditorDialogLoader.active = false }
        }

        Connections {
            target: DialogsProvider

            function onShowToolbarEditorDialog() {
                // The library and the player each have these dialogs: only
                // the ones of the view shown answer
                if (!root.visible)
                    return
                // The editor edits the player's bar in place: from the
                // library, show the player; its dialogs open the editor
                if (MainCtx.effectiveMainInterfaceMode !== MainCtx.MAININTERFACE_MODE_PLAYER) {
                    VLCStyle.customizeRequested = true
                    VLCStyle.customizeFromLibrary = true
                    MainCtx.playerView = true
                    return
                }
                toolbarEditorDialogLoader.active = true
                if (toolbarEditorDialogLoader.item)
                    toolbarEditorDialogLoader.item.open()
            }
        }

        // once the switch to the player is over: a popup opened during it
        // is hidden
        Timer {
            id: editorOpenTimer
            interval: VLCStyle.duration_long + 400
            onTriggered: {
                VLCStyle.customizeRequested = false
                DialogsProvider.showToolbarEditorDialog()
            }
        }
    }
}

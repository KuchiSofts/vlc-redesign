/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import VLC.Player
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

// The services subtitles are found on (SubtitleFinder.services), in their
// order: each with what it has and a switch; the fields of the credentials
// it declares (a key, a user name and password, an email: none special
// cased), Test or Sign in, Sign out or Clear, the page of its free key or
// account; Remove (Add a service brings it back). Smart: a service of one
// language says which turns it on, a service whose credentials a search found
// refused or that asked to wait says so. Kept when the settings are (save),
// but a check that works the first time turns its service on at once
SettingsSectionTitle {
    id: root
    text: qsTr("Subtitle services")

    // the languages to search on in the page above (not saved yet)
    property var languagesOn: []
    // what is shown, by service: { on, removed, key, username, password,
    // email, result, who, checking }
    property var states: ({})
    // and what was kept when the page opened
    property var _initial: ({})
    property bool _addOpen: false

    // The services are read (their keys from the keystore) and their rows
    // made when the page shows them: every opening of Settings, on any
    // page, paid for them
    property bool _begun: false
    function begin() {
        _begun = false
        states = ({})
        _initial = ({})
        if (visible)
            _load()
    }
    onVisibleChanged: if (visible && !_begun) _load()

    function _load() {
        _begun = true
        const all = {}
        const initial = {}
        for (const service of SubtitleFinder.services) {
            const credentials = SubtitleFinder.serviceCredentials(service.id)
            all[service.id] = { on: service.enabled, removed: service.removed, key: credentials.key,
                                username: credentials.username, password: credentials.password,
                                email: credentials.email, result: "", who: "", checking: false }
            initial[service.id] = Object.assign({}, all[service.id])
        }
        states = all
        _initial = initial
        _addOpen = false
        fieldsReset("")
    }

    // the fields show the credentials again (a user's typing ends a field's
    // binding): all of them (""), or a service's
    signal fieldsReset(string id)

    function save() {
        for (const id in states) {
            const entry = states[id]
            const initial = _initial[id] || {}
            // (only what changed: a service never switched keeps its default)
            if (entry.on !== initial.on)
                SubtitleFinder.setServiceEnabled(id, entry.on)
            if (entry.removed !== initial.removed)
                SubtitleFinder.setServiceRemoved(id, entry.removed)
            SubtitleFinder.setServiceCredentials(id, { key: entry.key, username: entry.username,
                                                       password: entry.password, email: entry.email })
        }
    }

    function update(id, changes) {
        const all = Object.assign({}, states)
        all[id] = Object.assign({}, all[id] || {}, changes)
        states = all
    }

    function typed(id) {
        const entry = states[id] || {}
        return { key: entry.key || "", username: entry.username || "", password: entry.password || "",
                 email: entry.email || "" }
    }
    function _same(a, b) {
        return a.key === b.key && a.username === b.username && a.password === b.password && a.email === b.email
    }
    // the fields it needs are filled
    function complete(service) {
        const values = typed(service.id)
        return service.required.every(field => String(values[field]).trim().length > 0)
    }

    Connections {
        target: SubtitleFinder
        function onServiceChecked(id, result, who) {
            const service = SubtitleFinder.services.find(s => s.id === id)
            const changes = { result: result, who: who, checking: false }
            // working the first time: on by itself
            if ((result === "works" || result === "signedIn") && service && !service.switched
                && root.states[id] && root.states[id].on === root._initial[id].on)
                changes.on = true
            root.update(id, changes)
        }
    }

    function c(value) { return VLCStyle.redesign.color(value) }

    // What a check found, in the words of the services page
    function resultText(result, who) {
        switch (result) {
        case "works": return qsTr("Key works")
        case "signedIn": return qsTr("Signed in as %1").arg(who)
        case "keyRefused": return qsTr("Key refused")
        case "loginRefused": return qsTr("Wrong user name or password")
        case "busy": return qsTr("Busy: try again in a minute")
        case "noAnswer": return qsTr("No answer, try again")
        default: return ""
        }
    }

    // A field of the credentials: hidden (a key, a password) with Show/Hide
    component SecretField: Item {
        id: secretField
        property alias text: field.text
        property alias placeholderText: field.placeholderText
        property bool secret: false
        property bool shown: false
        property alias field: field
        signal edited(string text)
        implicitWidth: root.px(200)
        implicitHeight: root.px(36)
        SettingsTextField {
            id: field
            anchors.fill: parent
            echoMode: secretField.secret && !secretField.shown ? TextInput.Password : TextInput.Normal
            // (room for Show/Hide)
            rightPadding: secretField.secret ? (LayoutMirroring.enabled ? root.px(14) : showButton.width + root.px(8))
                                             : (LayoutMirroring.enabled ? root.px(14) : root.px(18))
            leftPadding: secretField.secret && LayoutMirroring.enabled ? showButton.width + root.px(8)
                         : (LayoutMirroring.enabled ? root.px(18) : root.px(14))
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
            Accessible.name: placeholderText
            onTextEdited: secretField.edited(text)
        }
        T.Button {
            id: showButton
            visible: secretField.secret
            anchors.right: parent.right
            anchors.rightMargin: root.px(4)
            anchors.verticalCenter: parent.verticalCenter
            height: root.px(28)
            width: Math.max(root.px(48), showLabel.implicitWidth + root.px(12))
            focusPolicy: Qt.StrongFocus
            text: secretField.shown ? qsTr("Hide") : qsTr("Show")
            Accessible.name: text
            onClicked: secretField.shown = !secretField.shown
            background: Rectangle {
                radius: root.px(4)
                color: showButton.hovered ? root.c("#222222") : "transparent"
                border.width: showButton.visualFocus ? 1 : 0
                border.color: VLCStyle.redesign.accent
            }
            contentItem: Text {
                id: showLabel
                textFormat: Text.PlainText
                text: showButton.text
                color: root.c("#8F8F8F")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(12)
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    // A link of the design's accent ("Get a free key")
    component LinkButton: T.Button {
        id: link
        focusPolicy: Qt.StrongFocus
        implicitHeight: root.px(32)
        implicitWidth: linkLabel.implicitWidth + root.px(8)
        Accessible.role: Accessible.Link
        Accessible.name: text
        background: Rectangle {
            radius: root.px(4)
            color: "transparent"
            border.width: link.visualFocus ? 1 : 0
            border.color: VLCStyle.redesign.accent
        }
        contentItem: Text {
            id: linkLabel
            textFormat: Text.PlainText
            text: link.text
            color: VLCStyle.redesign.accent
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(13)
            font.underline: link.hovered
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }
        HoverHandler { cursorShape: Qt.PointingHandCursor }
    }

    Repeater {
        model: root._begun ? SubtitleFinder.services : []
        delegate: Column {
            id: serviceRow
            required property var modelData
            readonly property var service: modelData
            readonly property var local: root.states[service.id] || ({})
            readonly property bool on: local.on === true
            readonly property bool hasFields: service.fields.length > 0
            readonly property var values: root.typed(service.id)
            readonly property bool anyTyped: values.key !== "" || values.username !== "" || values.password !== ""
                                             || values.email !== ""
            // (what a search found is of the credentials kept)
            readonly property bool sameAsKept: root._initial[service.id] !== undefined
                                               && root._same(values, root.typed_initial(service.id))
            // (an account without a key: Sign in, Sign out)
            readonly property bool login: service.fields.indexOf("key") < 0
            readonly property bool languageOn: service.languages.length === 0
                                               || service.languages.some(l => root.languagesOn.indexOf(l) >= 0)

            objectName: "settingsSubtitleServiceRow_" + service.id
            visible: local.removed !== true
            width: root.width
            spacing: root.px(6)
            bottomPadding: root.px(10)

            function focusFields() {
                for (let i = 0; i < fieldsRepeater.count; ++i) {
                    const item = fieldsRepeater.itemAt(i)
                    if (item && item.field.text.trim() === "" && service.required.indexOf(item.name) >= 0) {
                        item.field.forceActiveFocus()
                        return
                    }
                }
                if (fieldsRepeater.count > 0 && fieldsRepeater.itemAt(0))
                    fieldsRepeater.itemAt(0).field.forceActiveFocus()
            }

            Item {
                width: parent.width
                height: root.px(52)
                Column {
                    anchors.left: parent.left
                    anchors.right: rowControls.left
                    anchors.rightMargin: root.px(12)
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: root.px(2)
                    Text {
                        width: parent.width
                        textFormat: Text.PlainText
                        text: serviceRow.service.name
                        color: root.c("#8F8F8F")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(14)
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                    }
                    Text {
                        width: parent.width
                        text: serviceRow.service.detail
                        color: root.c("#5F5F5F")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(12)
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                    }
                }
                Row {
                    id: rowControls
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: root.px(8)
                    T.Button {
                        id: removeButton
                        objectName: "settingsSubtitleServiceRemove_" + serviceRow.service.id
                        anchors.verticalCenter: parent.verticalCenter
                        implicitWidth: root.px(32)
                        implicitHeight: root.px(32)
                        focusPolicy: Qt.StrongFocus
                        Accessible.name: qsTr("Remove") + " " + serviceRow.service.name
                        T.ToolTip.visible: hovered || visualFocus
                        T.ToolTip.text: qsTr("Remove")
                        T.ToolTip.delay: VLCStyle.delayToolTipAppear
                        onClicked: root.update(serviceRow.service.id, { removed: true })
                        background: Rectangle {
                            radius: root.px(6)
                            color: removeButton.down ? root.c("#090909") : removeButton.hovered ? root.c("#222222") : "transparent"
                            border.width: removeButton.visualFocus ? 1 : 0
                            border.color: VLCStyle.redesign.accent
                        }
                        contentItem: Image {
                            source: SVGColorImage.colorize("qrc:///redesign/duo/close.svg").color1(root.c("#8F8F8F")).uri()
                            sourceSize: Qt.size(root.px(20), root.px(20))
                            fillMode: Image.Pad
                        }
                    }
                    SettingsToggle {
                        objectName: "settingsSubtitleService_" + serviceRow.service.id
                        anchors.verticalCenter: parent.verticalCenter
                        on: serviceRow.on
                        Accessible.name: serviceRow.service.name
                        onSwitched: (on) => {
                            root.update(serviceRow.service.id, { on: on })
                            // on without what it needs: its fields
                            if (on && serviceRow.hasFields && !root.complete(serviceRow.service))
                                Qt.callLater(serviceRow.focusFields)
                        }
                    }
                }
            }

            // what turns it on, what a search found of it
            Text {
                readonly property string status: serviceRow.sameAsKept ? serviceRow.service.status : ""
                width: parent.width
                visible: text !== ""
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignLeft
                text: status === "keyRefused" ? qsTr("Key refused: check it")
                      : status === "loginRefused" ? qsTr("Sign-in refused: check it")
                      : status === "busy" ? qsTr("Busy: try again in a minute")
                      : serviceRow.service.languages.length > 0
                        ? qsTr("Asked only when %1 is one of your subtitle languages").arg(serviceRow.service.languageNames)
                        : ""
                color: status !== "" ? root.c("#E15032")
                       : serviceRow.languageOn ? root.c("#5F5F5F") : root.c("#8F8F8F")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(12)
            }

            // its credentials, when it is on or has some
            Flow {
                id: fieldsFlow
                width: parent.width
                visible: serviceRow.hasFields && (serviceRow.on || serviceRow.anyTyped)
                spacing: root.px(8)
                Repeater {
                    id: fieldsRepeater
                    model: serviceRow.service.fields
                    delegate: SecretField {
                        id: fieldDelegate
                        required property string modelData
                        readonly property string name: modelData
                        readonly property bool optional: serviceRow.service.required.indexOf(name) < 0
                        objectName: "settingsSubtitleServiceField_" + serviceRow.service.id + "_" + name
                        width: Math.max(root.px(160), Math.min(root.px(260),
                                        (fieldsFlow.width - root.px(8) * (serviceRow.service.fields.length - 1))
                                        / serviceRow.service.fields.length))
                        secret: name === "key" || name === "password"
                        placeholderText: name === "key" ? qsTr("Your free key")
                                         : name === "username" ? (optional ? qsTr("User name (optional)") : qsTr("User name"))
                                         : name === "password" ? (optional ? qsTr("Password (optional)") : qsTr("Password"))
                                         : qsTr("Email")
                        Component.onCompleted: fieldDelegate.text = serviceRow.values[fieldDelegate.name]
                        Connections {
                            target: root
                            function onFieldsReset(id) {
                                if (id === "" || id === serviceRow.service.id)
                                    fieldDelegate.text = serviceRow.values[fieldDelegate.name]
                            }
                        }
                        onEdited: (value) => {
                            const changes = { result: "" }
                            changes[name] = value
                            root.update(serviceRow.service.id, changes)
                        }
                    }
                }
            }
            Flow {
                width: parent.width
                visible: serviceRow.hasFields
                spacing: root.px(8)
                SettingsButton {
                    objectName: "settingsSubtitleServiceCheck_" + serviceRow.service.id
                    visible: serviceRow.on || serviceRow.anyTyped
                    text: serviceRow.login ? qsTr("Sign in") : qsTr("Test")
                    enabled: !serviceRow.local.checking && root.complete(serviceRow.service) && serviceRow.anyTyped
                    onClicked: {
                        root.update(serviceRow.service.id, { checking: true, result: "" })
                        SubtitleFinder.checkService(serviceRow.service.id, root.typed(serviceRow.service.id))
                    }
                }
                SettingsButton {
                    objectName: "settingsSubtitleServiceClear_" + serviceRow.service.id
                    visible: serviceRow.anyTyped
                    text: serviceRow.login ? qsTr("Sign out") : qsTr("Clear")
                    onClicked: {
                        root.update(serviceRow.service.id, { key: "", username: "", password: "", email: "",
                                                             result: "" })
                        root.fieldsReset(serviceRow.service.id)
                    }
                }
                LinkButton {
                    objectName: "settingsSubtitleServicePage_" + serviceRow.service.id
                    visible: serviceRow.service.page !== ""
                    text: serviceRow.service.account ? qsTr("Create a free account") : qsTr("Get a free key")
                    onClicked: Qt.openUrlExternally(serviceRow.service.page)
                }
                Text {
                    objectName: "settingsSubtitleServiceResult_" + serviceRow.service.id
                    height: root.px(32)
                    verticalAlignment: Text.AlignVCenter
                    visible: text !== ""
                    text: serviceRow.local.checking ? "…" : root.resultText(serviceRow.local.result, serviceRow.local.who)
                    color: serviceRow.local.result === "works" || serviceRow.local.result === "signedIn"
                           ? VLCStyle.redesign.accent : root.c("#E15032")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(13)
                    Accessible.role: Accessible.StaticText
                    Accessible.name: text
                }
            }
        }
    }

    // The services removed, to bring back
    readonly property var _removed: !_begun ? []
        : SubtitleFinder.services.filter(s => states[s.id] && states[s.id].removed === true)
    Flow {
        width: parent.width
        visible: root._removed.length > 0
        spacing: root.px(8)
        topPadding: root.px(4)
        bottomPadding: root.px(8)
        SettingsButton {
            objectName: "settingsSubtitleServiceAdd"
            text: qsTr("Add a service")
            onClicked: root._addOpen = !root._addOpen
        }
        Repeater {
            model: root._addOpen ? root._removed : []
            delegate: SettingsButton {
                required property var modelData
                objectName: "settingsSubtitleServiceRestore_" + modelData.id
                text: modelData.name
                onClicked: {
                    root.update(modelData.id, { removed: false })
                    if (root._removed.length === 0)
                        root._addOpen = false
                }
            }
        }
    }

    function typed_initial(id) {
        const entry = _initial[id] || {}
        return { key: entry.key || "", username: entry.username || "", password: entry.password || "",
                 email: entry.email || "" }
    }

    Text {
        width: parent.width
        topPadding: root.px(4)
        bottomPadding: root.px(8)
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignLeft
        text: qsTr("To find subtitles, the services turned on get the video's title, its IMDb id, its file name and hash (made from a part of the file), and nothing else.")
        color: root.c("#5F5F5F")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(12)
    }
}

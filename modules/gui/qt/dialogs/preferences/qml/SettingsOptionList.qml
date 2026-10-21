/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import VLC.Style
import VLC.MainInterface
import VLC.Widgets as Widgets


Column {
    id: root
    Widgets.FilePicker { id: filePicker; ctx: MainCtx }
    SettingsPresentation { id: presentationData }
    property var settingsModel
    property string category: ""
    property string search: ""
    property string filterMode: "all"
    property string sectionFilter: ""
    property string excludedSection: ""
    property bool hideSectionHeadings: false
    // Developer mode: the expert options (the design leaves them out) show
    // under Advanced Options
    property bool showExpert: false
    // Reset all settings was clicked (the dialog confirms it)
    signal resetAllRequested()
    readonly property int count: optionsRepeater.count
    // Whether the option of a row of the model shows in this list
    // Font size of the subtitles: by name (Relative) or in pixels, as Size values says
    function sizeModeHidden(key) {
        if (key !== "sub-text-scale" && key !== "freetype-fontsize")
            return false
        const pixels = Number((root.settingsModel.revision,
                               root.settingsModel.valueFor("redesign/subtitle-size-mode"))) === 1
        return (key === "freetype-fontsize") !== pixels
    }
    // A search shows the rows whose label, section or help (the text of
    // their (i)) holds the text: what the page shows, not the keys of VLC
    // ("Shuffle" is "random"; every key of the interface starts with "qt").
    // Worked out once per text, not for each row and the rows before it
    readonly property var searchMatches: {
        const count = optionsRepeater.count
        if (!root.search.length)
            return null
        const text = root.search.toLowerCase()
        const matches = []
        for (let i = 0; i < count; ++i) {
            const key = root.settingsModel.keyAt(i)
            const entry = presentationData.entry(key, root.settingsModel.labelAt(i))
            // a row shown again elsewhere is found once
            matches.push(!key.startsWith("x-mirror:")
                         && (String(entry.label || "").toLowerCase().includes(text)
                             || String(entry.section || "").toLowerCase().includes(text)
                             || root.settingsModel.helpAt(i).toLowerCase().includes(text)))
        }
        return matches
    }
    function entryVisible(i) {
        const key = root.settingsModel.keyAt(i)
        const entry = presentationData.entry(key, root.settingsModel.labelAt(i))
        return !entry.hidden && (!entry.expert || root.showExpert) && !sizeModeHidden(key)
               && (filterMode === "all" || (filterMode === "advanced") === entry.advanced)
               && (!sectionFilter || entry.section === sectionFilter)
               && (!excludedSection || entry.section !== excludedSection)
               && (!root.searchMatches || !!root.searchMatches[i])
    }
    // A new category can have as many rows as the last one: the rows shown
    // are worked out again when the model is rebuilt
    property int _rowsRevision: 0
    Connections {
        target: root.settingsModel
        function onModelReset() { ++root._rowsRevision }
    }
    // Whether each row shows, and the row shown before it (-1: none),
    // worked out once for the list: each row looking back through the rows
    // before it took the time of the square of the rows on every key typed.
    // (A row still asks for itself whether it shows: a row created while
    // the model is rebuilt would read the rows of the last one here.)
    readonly property var shownRows: {
        root._rowsRevision
        const shown = []
        for (let i = 0; i < optionsRepeater.count; ++i)
            shown.push(entryVisible(i))
        return shown
    }
    readonly property var previousShownRows: {
        const previous = []
        let last = -1
        for (let i = 0; i < shownRows.length; ++i) {
            previous.push(last)
            if (shownRows[i])
                last = i
        }
        return previous
    }
    readonly property int availableCount: shownRows.filter(shown => shown).length
    width: parent ? parent.width : implicitWidth
    spacing: 0

    Repeater {
        id: optionsRepeater
        model: root.settingsModel
        delegate: Column {
            width: root.width
            spacing: 0
            readonly property var presentation: presentationData.entry(model.key, model.label)
            // A category can place an inline effect panel between native option groups.
            visible: root.entryVisible(index)
            // The row shown before this one in the list (-1: the first one)
            readonly property int previousVisible: root.previousShownRows[index] ?? -1
            readonly property string previousSection: previousVisible >= 0
                ? presentationData.entry(root.settingsModel.keyAt(previousVisible), "").section : ""

            // Section title of the design: Inter Medium 16 #C6C6C6, 10 px
            // above and below, 20 px after the previous section
            Text {
                id: sectionTitle
                visible: !root.hideSectionHeadings
                         && (root.filterMode !== "advanced" || !!presentation.heading)
                         && (previousVisible < 0 || presentation.section !== previousSection)
                width: parent.width
                // whole pixels: the rows below stay on the pixel grid
                height: visible ? VLCStyle.dp(previousVisible < 0 ? 39 : 59, VLCStyle.scale) : 0
                // the baseline where the design puts it (the line 10 px from
                // the top), whatever the line metrics of Qt
                topPadding: Math.round(VLCStyle.redesign.textBaseline(VLCStyle.dp(previousVisible < 0 ? 10 : 30, VLCStyle.scale),
                                                                      font.pixelSize)) - titleMetrics.ascent
                FontMetrics { id: titleMetrics; font: sectionTitle.font }
                text: presentation.section
                color: VLCStyle.redesign.color("#C6C6C6", "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                font.weight: Font.Medium
            }

            // Equalizer of the design: the bar (or the open panel), 5 px
            // above and below as a row
            // (the editors of a row are only created for a row shown: a
            // page creates them for each option of its category)
            Loader {
                width: parent.width
                active: parent.visible && presentation.editor === "equalizer"
                sourceComponent: Item {
                    height: equalizerPanel.implicitHeight + VLCStyle.dp(10, VLCStyle.scale)
                    SettingsEqualizerPanel {
                        id: equalizerPanel
                        y: VLCStyle.dp(5, VLCStyle.scale)
                        width: parent.width
                        // the bands of the user, else those of the preset
                        readonly property var preset: presentation.editor === "equalizer"
                            ? (root.settingsModel.revision, root.settingsModel.equalizerPreset(
                                   String(root.settingsModel.valueFor("equalizer-preset")))) : []
                        bands: String(model.value || "").trim().length
                               ? String(model.value).trim().split(/\s+/).map(Number)
                               : preset.slice(1)
                        preamp: (root.settingsModel.revision,
                                 Number(root.settingsModel.valueFor("equalizer-preamp") ?? 12))
                        onBandsEdited: (bands) => root.settingsModel.setValue(model.key,
                                                      bands.map(v => v.toFixed(1)).join(" "))
                        onPreampEdited: (preamp) => root.settingsModel.setValue("equalizer-preamp", preamp)
                    }
                }
            }

            Loader {
                width: parent.width
                active: parent.visible && presentation.editor !== "equalizer"
                // the roles of a row read undefined for a moment while the
                // list is rebuilt: fall back to empty values
                sourceComponent: SettingsOptionRow {
                    settingKey: model.key ?? ""
                    label: presentation.label ?? ""
                    accessibleLabel: presentation.accessible || presentation.label || ""
                    help: model.help ?? ""
                    settingType: model.type ?? ""
                    settingValue: model.value
                    choices: {
                        const labels = presentation.choiceLabels
                        // Wheels: Seek is backwards (3) when Reverse scroll direction is on
                        const setting = (model.key ?? "").replace(/^x-mirror:/, "")
                        if (setting === "hotkeys-y-wheel-mode" || setting === "hotkeys-x-wheel-mode")
                            return [{ value: -1, label: qsTr("Nothing") }, { value: 0, label: qsTr("Volume") },
                                    { value: model.value === 3 ? 3 : 2, label: qsTr("Seek") }]
                        if (presentation.editor === "boolchoice" && labels)
                            return [{ value: false, label: labels[false] }, { value: true, label: labels[true] }]
                        if (!labels || !model.choices)
                            return model.choices || []
                        const named = model.choices.map(choice => ({ value: choice.value,
                                                                     label: labels[choice.value] ?? choice.label }))
                        // the default of VLC as a choice of its own, shown for
                        // that value; not when it is one of the choices (its
                        // name would never show)
                        return presentation.defaultChoice
                               && !model.choices.some(choice => choice.value === model.defaultValue)
                            ? [{ value: model.defaultValue, label: presentation.defaultChoice }].concat(named)
                            : named
                    }
                    minimum: model.minimum
                    maximum: model.maximum
                    editorKind: presentation.editor
                    showInfo: presentation.info !== undefined ? presentation.info : (model.help ?? "").length > 0
                    infoGap: presentation.infoGap !== undefined ? presentation.infoGap : 10
                    unit: presentation.unit || ""
                    onValue: model.defaultValue
                    otherValue: model.key === "volume-save"
                                ? (root.settingsModel.revision, root.settingsModel.valueFor("mmdevice-volume"))
                                : undefined
                    onCommitOther: (value) => root.settingsModel.setValue("mmdevice-volume", value)
                    defaultText: model.type === "directory" ? root.settingsModel.defaultText(model.key) : ""
                    displayValue: model.type === "directory"
                                  ? root.settingsModel.displayPath(String(model.value || "").length
                                                                   ? String(model.value) : defaultText)
                                  : ""
                    dirty: !!model.dirty
                    onCommit: (value) => root.settingsModel.setValue(settingKey, value)
                    decimals: presentation.decimals
                    rowHeight: presentation.rowHeight || 0
                    fieldHeight: presentation.fieldHeight || 0
                    defaultDisplay: presentation.defaultDisplay || ""
                    controlTop: presentation.controlTop
                    labelsBaseline: presentation.labelsBaseline
                    available: (model.key !== "one-instance-when-started-from-file" && model.key !== "playlist-enqueue"
                                || !!(root.settingsModel.revision, root.settingsModel.valueFor("one-instance")))
                               // the fullscreen controls of the skins: with a skin only
                               && (model.key !== "qt-fs-controller"
                                   || !!(root.settingsModel.revision, root.settingsModel.valueFor("x-custom-skin")))
                               && (!(model.key ?? "").endsWith("-caching") || model.category !== "network"
                                   || Number((root.settingsModel.revision, root.settingsModel.valueFor("x-buffering"))) === 4)
                    placeholder: presentation.placeholder || ""
                    valueShown: model.key !== "http-port"
                                || !!(root.settingsModel.revision, root.settingsModel.valueFor("x-web-remote"))
                    // The web interface without a password only shows a page
                    // asking for one (http.lua)
                    readonly property bool webRemoteLocked: model.key === "x-web-remote" && !!model.value
                        && (root.settingsModel.revision, root.settingsModel.hasOption("http-password"))
                        && !String(root.settingsModel.valueFor("http-password") || "").length
                    note: {
                        const notes = []
                        if (webRemoteLocked)
                            notes.push(qsTr("Set a web remote password in the Advanced Options: the remote stays locked without one."))
                        // the settings read when VLC starts, once changed
                        if (presentation.restart && model.pendingRestart)
                            notes.push(qsTr("Applies after restart"))
                        return notes.join(" ")
                    }
                    noteWarning: webRemoteLocked
                    onColorRequested: {
                        const rgb = filePicker.chooseColor(Number(settingValue), label)
                        if (rgb >= 0)
                            root.settingsModel.setValue(settingKey, rgb)
                    }
                    onBrowseRequested: {
                        if (editorKind === "directory") {
                            const titles = {
                                "snapshot-path": qsTr("Choose a snapshot folder"),
                                "input-timeshift-path": qsTr("Choose a time-shift buffer folder"),
                                "input-record-path": qsTr("Choose a recording folder")
                            }
                            const folder = filePicker.chooseFolder(titles[settingKey] || qsTr("Choose a folder"))
                            if (folder.toString() !== "")
                                root.settingsModel.setDirectoryUrl(settingKey, folder)
                        } else if (editorKind === "openfile") {
                            const file = filePicker.chooseOpenFile(qsTr("Choose a skin"),
                                                                   qsTr("Skin files (*.vlt *.wsz *.xml)"))
                            if (file.length) root.settingsModel.setValue(settingKey, file)
                        } else if (editorKind === "savefile") {
                            const file = filePicker.chooseSaveFile(qsTr("Choose a log file"), String(settingValue || ""))
                            if (file.length) root.settingsModel.setValue(settingKey, file)
                        }
                    }
                }
            }

            // (built in the list that shows the row only: each holds a model
            // of the media library)
            Loader {
                width: parent.width
                active: parent.visible && root.category === "privacy" && root.search.length === 0
                        && model.key === "redesign/recentplay-filter"
                sourceComponent: SettingsPrivacyActions { width: root.width }
            }

            // Open configurations folder, Reset all settings: after the
            // advanced rows of Diagnostics in the design
            Loader {
                width: parent.width
                active: parent.visible && root.category === "general" && root.filterMode === "advanced"
                        && model.key === "stats"
                sourceComponent: Column {
                    width: root.width
                    SettingsActionRow {
                        width: parent.width
                        label: qsTr("Open configurations folder")
                        help: qsTr("Shows the folder of the settings of VLC.")
                        buttonText: qsTr("Open")
                        buttonWidth: VLCStyle.dp(100, VLCStyle.scale)
                        onClicked: root.settingsModel.openConfigFolder()
                    }
                    SettingsActionRow {
                        width: parent.width
                        label: qsTr("Reset all settings")
                        help: qsTr("Puts every setting of VLC back to its default.")
                        buttonText: qsTr("Reset")
                        buttonWidth: VLCStyle.dp(100, VLCStyle.scale)
                        destructive: true
                        onClicked: root.resetAllRequested()
                    }
                }
            }

            // "Rescan plugins" ends Smooth Playback in the design
            Loader {
                width: parent.width
                active: parent.visible && root.category === "performance" && root.search.length === 0
                        && root.filterMode !== "advanced" && model.key === "redesign/process-priority"
                sourceComponent: SettingsActionRow {
                    width: root.width
                    label: qsTr("Rescan plugins")
                    buttonText: qsTr("Rescan")
                    onClicked: root.settingsModel.rescanPlugins()
                }
            }
        }
    }
}

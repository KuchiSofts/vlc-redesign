/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import VLC.MainInterface

// The named presets of the Effects window (Figma 2372:8834 "Setting
// Preset"): the values of the audio and the video effects kept under a name
// in the settings of the interface, as JSON. A preset holds the part of the
// effects that played when it was saved (the audio, the video or both);
// applied, it changes that part only, inside the transaction of the window
// (Save keeps it, Cancel drops it), as the controls would.
QtObject {
    id: root

    // AudioEffects and VideoEffects of the window
    required property var audio
    required property var video

    readonly property string storageKey: "redesign/effectPresets"
    // as audio_effects.cpp and video_effects.cpp list them
    readonly property var audioFilters: ["equalizer", "compressor", "spatializer", "stereo_widen"]
    readonly property var videoFilters: ["adjust", "sharpen", "gradfun", "grain", "extract", "colorthres",
                                         "invert", "posterize", "gradient", "sepia", "transform", "rotate",
                                         "magnify", "puzzle", "logo", "marq", "erase", "anaglyph", "mirror",
                                         "psychedelic", "wave", "ripple", "motiondetect", "gaussianblur",
                                         "motionblur", "antiflicker", "hqdn3d"]
    readonly property var cropSides: ["top", "bottom", "left", "right"]
    readonly property string defaultName: qsTr("Default Preset")
    // the longest name kept (a longer one is cut)
    readonly property int nameLength: 120

    // [{ name, audio?, video? }], in the order they were made
    property var presets: []
    readonly property var names: presets.map(preset => preset.name)
    property string lastError: ""

    Component.onCompleted: load()

    function isObject(value) { return value !== null && typeof value === "object" && !Array.isArray(value) }
    function has(object, key) { return isObject(object) && Object.prototype.hasOwnProperty.call(object, key) }
    // one line, whatever was pasted
    function normalName(text) { return String(text).replace(/\s+/g, " ").trim().slice(0, nameLength) }

    function indexOf(name) {
        for (let i = 0; i < presets.length; ++i)
            if (presets[i].name === name) return i
        return -1
    }

    // why a name cannot be given ("" when it can), the preset named except
    // apart (its own name when renamed)
    function nameError(text, except) { return nameErrorIn(presets, text, except) }

    function nameErrorIn(list, text, except) {
        const name = normalName(text)
        if (!name) return qsTr("Enter a name for the preset.")
        const folded = name.toLocaleLowerCase()
        if (folded === defaultName.toLocaleLowerCase() || folded === "default preset")
            return qsTr("\"%1\" is the preset of the default effects.").arg(name)
        for (const preset of list)
            if (preset.name !== except && preset.name.toLocaleLowerCase() === folded)
                return qsTr("A preset named \"%1\" already exists.").arg(name)
        return ""
    }

    // a name not taken yet, as the design names them ("Custom Preset 01")
    function suggestedName() {
        for (let i = presets.length + 1; ; ++i) {
            const name = qsTr("Custom Preset %1").arg(i < 10 ? "0" + i : String(i))
            if (!nameError(name, "")) return name
        }
    }

    // Missing or garbled data reads as no preset; a preset without a name,
    // with a name already read or with no effects is left out. What its
    // effects hold is checked as it is applied.
    function load() {
        let raw = MainCtx.settingValue(storageKey, "")
        // (a list where the settings file lost the quotes of the text)
        if (Array.isArray(raw)) raw = raw.join(",")
        let items = []
        if (typeof raw === "string" && raw.length > 0) {
            try {
                const data = JSON.parse(raw)
                if (isObject(data) && Array.isArray(data.presets)) items = data.presets
            } catch (error) {
                items = []
            }
        }
        const list = []
        for (const item of items) {
            if (!isObject(item) || typeof item.name !== "string") continue
            const preset = { name: normalName(item.name) }
            if (isObject(item.audio)) preset.audio = item.audio
            if (isObject(item.video)) preset.video = item.video
            if ((preset.audio || preset.video) && !nameErrorIn(list, preset.name, ""))
                list.push(preset)
        }
        presets = list
    }

    function store(list) {
        presets = list
        MainCtx.setSettingValue(storageKey, JSON.stringify({ version: 1, presets: list }))
    }

    // ---- what plays now

    // The effects of the outputs playing: the audio ones with an audio
    // output, the video ones with a video output (none without)
    function capture() {
        const result = {}
        if (audio.activeAudio) {
            const filters = {}
            for (const id of audioFilters) {
                if (!audio.filterAvailable(id)) continue
                const entry = { enabled: audio.filterEnabled(id) }
                if (id === "equalizer") {
                    entry.preset = String(audio.parameterValue("equalizer-preset") ?? "")
                    entry.bands = audio.bands.map(Number)
                    entry.preamp = Number(audio.preamp)
                    entry.twoPass = !!audio.twoPass
                } else {
                    const values = {}
                    for (const parameter of audio.parameters(id))
                        if (parameter.type === "float") values[parameter.id] = Number(parameter.value)
                    entry.values = values
                }
                filters[id] = entry
            }
            result.audio = { filters: filters }
        }
        if (video.activeVideo) {
            const filters = {}
            for (const id of videoFilters) {
                if (!video.filterAvailable(id)) continue
                const values = {}
                for (const parameter of video.parameters(id)) {
                    const value = parameter.value
                    if (typeof value === "number" || typeof value === "boolean" || typeof value === "string")
                        values[parameter.id] = value
                }
                filters[id] = { enabled: video.filterEnabled(id), values: values }
            }
            const crop = {}
            for (const side of cropSides) crop[side] = video.cropValue(side)
            result.video = { filters: filters, crop: crop }
        }
        return result
    }

    function canCapture() { return audio.activeAudio || video.activeVideo }

    // no filter on, no crop
    function atDefaults(current) {
        for (const part of [current.audio, current.video]) {
            if (!part) continue
            for (const id in part.filters)
                if (part.filters[id].enabled) return false
        }
        if (current.video)
            for (const side of cropSides)
                if (current.video.crop[side] !== 0) return false
        return true
    }

    function sameNumber(a, b, tolerance) {
        const x = Number(a), y = Number(b)
        return Number.isFinite(x) && Number.isFinite(y) && Math.abs(x - y) <= tolerance * Math.max(1, Math.abs(y))
    }

    // The same filters on, with the same values (those of the filters off
    // do not show)
    function sameFilters(stored, current, sameEntry) {
        for (const id in current) {
            const entry = has(stored, id) && isObject(stored[id]) ? stored[id] : null
            const on = current[id].enabled === true
            if (on !== (entry !== null && entry.enabled === true)) return false
            if (on && !sameEntry(id, entry, current[id])) return false
        }
        return true
    }

    function sameValues(stored, current) {
        if (!isObject(stored)) return false
        for (const key in current) {
            if (!has(stored, key)) continue
            const value = current[key]
            if (typeof value === "number" ? !sameNumber(stored[key], value, 1e-4) : stored[key] !== value)
                return false
        }
        return true
    }

    function sameAudio(stored, current) {
        return sameFilters(stored.filters, current.filters, (id, entry, now) => {
            if (id !== "equalizer") return sameValues(entry.values, now.values)
            const bands = Array.isArray(entry.bands) ? entry.bands : []
            for (let i = 0; i < now.bands.length; ++i)
                if (!sameNumber(bands[i] ?? 0, now.bands[i], 0.005)) return false
            return sameNumber(entry.preamp ?? 0, now.preamp, 0.005) && (entry.twoPass === true) === now.twoPass
        })
    }

    function sameVideo(stored, current) {
        if (!sameFilters(stored.filters, current.filters, (id, entry, now) => sameValues(entry.values, now.values)))
            return false
        const crop = isObject(stored.crop) ? stored.crop : {}
        for (const side of cropSides)
            if (Math.round(Number(crop[side] ?? 0)) !== current.crop[side]) return false
        return true
    }

    // The preset the effects playing are: "" (the default one) at their
    // defaults or when no preset holds them
    function matchingName() {
        const current = capture()
        if (atDefaults(current)) return ""
        for (const preset of presets) {
            let compared = false
            if (current.audio && preset.audio) {
                if (!sameAudio(preset.audio, current.audio)) continue
                compared = true
            }
            if (current.video && preset.video) {
                if (!sameVideo(preset.video, current.video)) continue
                compared = true
            }
            if (compared) return preset.name
        }
        return ""
    }

    // ---- the presets

    function create(text) {
        lastError = nameError(text, "")
        if (lastError) return false
        const current = capture()
        if (!current.audio && !current.video) {
            lastError = qsTr("Play a media to save its effects as a preset.")
            return false
        }
        const list = presets.slice()
        list.push(Object.assign({ name: normalName(text) }, current))
        store(list)
        return true
    }

    function rename(name, text) {
        const index = indexOf(name)
        lastError = index < 0 ? qsTr("The preset no longer exists.") : nameError(text, name)
        if (lastError) return false
        const list = presets.slice()
        list[index] = Object.assign({}, list[index], { name: normalName(text) })
        store(list)
        return true
    }

    // the effects playing replace those of the preset (a part not playing,
    // audio or video, stays as it was)
    function update(name) {
        const index = indexOf(name)
        const current = capture()
        lastError = index < 0 ? qsTr("The preset no longer exists.")
                  : !current.audio && !current.video ? qsTr("Play a media to save its effects in the preset.") : ""
        if (lastError) return false
        const list = presets.slice()
        list[index] = Object.assign({}, list[index], current)
        store(list)
        return true
    }

    function remove(name) {
        const index = indexOf(name)
        lastError = index < 0 ? qsTr("The preset no longer exists.") : ""
        if (lastError) return false
        const list = presets.slice()
        list.splice(index, 1)
        store(list)
        return true
    }

    // ---- applying one

    // A value of a slider or a field: as it is when the effect takes it (a
    // value beyond the slider set elsewhere), else brought into the range
    function setNumber(bridge, parameter, value) {
        if (bridge.setParameter(parameter.id, value)) return true
        const low = parameter.minimum !== undefined ? Number(parameter.minimum) : value
        const high = parameter.maximum !== undefined ? Number(parameter.maximum) : value
        const clamped = Math.min(Math.max(value, low), high)
        return clamped !== value && bridge.setParameter(parameter.id, clamped)
    }

    function applyAudioValues(id, values) {
        let ok = true
        for (const parameter of audio.parameters(id)) {
            if (parameter.type !== "float" || !has(values, parameter.id)) continue
            const value = values[parameter.id]
            if (typeof value !== "number" || !Number.isFinite(value)) continue
            if (!sameNumber(value, parameter.value, 1e-6))
                ok = setNumber(audio, parameter, value) && ok
        }
        return ok
    }

    // The preset of the equalizer first (its name, then its bands and preamp
    // if they were changed from it), as the panel sets them
    function applyEqualizer(entry) {
        let ok = true
        const known = audio.presets.find(item => item.id === entry.preset)
                      ?? audio.presets.find(item => item.id === "flat") ?? audio.presets[0]
        if (known && audio.parameterValue("equalizer-preset") !== known.id)
            ok = audio.setPreset(known.id)
        const bands = Array.isArray(entry.bands) ? entry.bands : []
        const current = audio.bands
        for (let i = 0; i < Math.min(bands.length, current.length); ++i) {
            const value = Number(bands[i])
            if (typeof bands[i] !== "number" || !Number.isFinite(value)) continue
            const band = Math.min(Math.max(value, -20), 20)
            if (!sameNumber(band, current[i], 1e-4))
                ok = audio.setBand(i, band) && ok
        }
        if (typeof entry.preamp === "number" && Number.isFinite(entry.preamp)) {
            const preamp = audio.parameters("equalizer").find(item => item.id === "equalizer-preamp")
            if (preamp && !sameNumber(entry.preamp, audio.preamp, 1e-6))
                ok = setNumber(audio, preamp, entry.preamp) && ok
        }
        if (typeof entry.twoPass === "boolean" && entry.twoPass !== audio.twoPass)
            ok = audio.setTwoPass(entry.twoPass) && ok
        return ok
    }

    // a value of a video effect as its control would set it, else left
    function applyVideoValue(parameter, value) {
        switch (parameter.type) {
        case "float":
            if (typeof value !== "number" || !Number.isFinite(value) || sameNumber(value, parameter.value, 1e-6))
                return true
            return setNumber(video, parameter, value)
        case "integer":
        case "color": {
            if (typeof value !== "number" || !Number.isFinite(value)) return true
            let number = Math.round(value)
            if (parameter.type === "color") number = Math.min(Math.max(number, 0), 0xFFFFFF)
            return number === parameter.value || setNumber(video, parameter, number)
        }
        case "choice": {
            const choice = video.choices(parameter.id).find(item => item.id === value)
            return !choice || choice.id === parameter.value || video.setParameter(parameter.id, choice.id)
        }
        case "bool":
            return typeof value !== "boolean" || value === parameter.value || video.setParameter(parameter.id, value)
        case "text":
        case "file":
            return typeof value !== "string" || value.length > 4096 || value === parameter.value
                   || video.setParameter(parameter.id, value)
        }
        return true
    }

    // The filters to be off first (what follows does not run through them),
    // then the values, then the filters to be on. A filter the preset does
    // not name (or the build lacks) is off; none: the default effects.
    function applyAudio(part) {
        if (!audio.activeAudio || !audio.begin()) return false
        const stored = part && isObject(part.filters) ? part.filters : {}
        const wanted = id => part !== null && has(stored, id) && isObject(stored[id]) && stored[id].enabled === true
        const available = audioFilters.filter(id => audio.filterAvailable(id))
        let ok = true
        for (const id of available)
            if (!wanted(id)) ok = audio.setFilterEnabled(id, false) && ok
        for (const id of available) {
            if (part === null) {
                ok = audio.resetDefaults(id) && ok
                continue
            }
            if (!has(stored, id) || !isObject(stored[id])) continue
            ok = (id === "equalizer" ? applyEqualizer(stored[id])
                                     : applyAudioValues(id, stored[id].values)) && ok
        }
        for (const id of available)
            if (wanted(id)) ok = audio.setFilterEnabled(id, true) && ok
        return ok
    }

    function applyVideo(part) {
        if (!video.activeVideo || !video.begin()) return false
        const stored = part && isObject(part.filters) ? part.filters : {}
        const wanted = id => part !== null && has(stored, id) && isObject(stored[id]) && stored[id].enabled === true
        const available = videoFilters.filter(id => video.filterAvailable(id))
        let ok = true
        for (const id of available)
            if (!wanted(id)) ok = video.setFilterEnabled(id, false) && ok
        for (const id of available) {
            if (part === null) {
                ok = video.resetDefaults(id) && ok
                continue
            }
            const values = has(stored, id) && isObject(stored[id]) ? stored[id].values : null
            if (!isObject(values)) continue
            for (const parameter of video.parameters(id))
                if (has(values, parameter.id)) ok = applyVideoValue(parameter, values[parameter.id]) && ok
        }
        for (const id of available)
            if (wanted(id)) ok = video.setFilterEnabled(id, true) && ok
        const crop = part && isObject(part.crop) ? part.crop : {}
        for (const side of cropSides) {
            let pixels = Math.round(Number(crop[side] ?? 0))
            pixels = Number.isFinite(pixels) ? Math.min(Math.max(pixels, 0), 2147483647) : 0
            if (pixels !== video.cropValue(side)) ok = video.setCrop(side, pixels) && ok
        }
        return ok
    }

    // The preset named ("" the default effects) on the outputs playing: the
    // default one on both, another on the part it holds. False when nothing
    // could be applied; a value refused leaves lastError.
    function apply(name) {
        const preset = name ? presets[indexOf(name)] : null
        lastError = name && !preset ? qsTr("The preset no longer exists.") : ""
        if (lastError) return false
        const audioPart = preset ? (isObject(preset.audio) ? preset.audio : undefined) : null
        const videoPart = preset ? (isObject(preset.video) ? preset.video : undefined) : null
        const playsAudio = audioPart !== undefined && audio.activeAudio
        const playsVideo = videoPart !== undefined && video.activeVideo
        // (a preset holds one part at least: the part playing is not its own)
        if (!playsAudio && !playsVideo) {
            lastError = !audio.activeAudio && !video.activeVideo ? qsTr("Play a media to apply a preset.")
                      : audio.activeAudio ? qsTr("This preset holds video effects only: play a video to apply it.")
                                          : qsTr("This preset holds audio effects only: play a media with sound to apply it.")
            return false
        }
        let ok = true
        if (playsAudio) ok = applyAudio(audioPart) && ok
        if (playsVideo) ok = applyVideo(videoPart) && ok
        if (!ok)
            lastError = qsTr("Some effects of the preset could not be applied.")
        return true
    }
}

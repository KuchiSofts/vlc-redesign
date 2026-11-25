// Copyright (C) 2026 VLC authors and VideoLAN
// SPDX-License-Identifier: GPL-2.0-or-later

const assert = require("node:assert/strict")
const fs = require("node:fs")
const vm = require("node:vm")

const path = "modules/gui/qt/iptv/qml/TvGuideView.qml"
const qml = fs.readFileSync(path, "utf8")
const replies = []
const deferred = []
const row = {
    key: "channel",
    hasGuide: true,
    programmes: [],
    _blocks: {},
    _loadGeneration: 0,
    shown: { clear() {} },
    refreshShown() {},
    Qt: { callLater: load => deferred.push(load) },
    root: {
        origin: 0,
        blockSecs: 3600,
        firstBlock: 0,
        lastBlock: 0,
        askProgrammes(key, from, to, done) {
            replies.push(done)
        }
    }
}

// Exercise the row's callbacks without starting the guide or its workers.
row.row = row
const methods = ["load", "reload"].map(name => {
    const match = qml.match(new RegExp(`^ {12}(function ${name}\\(\\) \\{[\\s\\S]*?^ {12}\\})\\r?$`, "m"))
    assert.ok(match, `TV guide row method ${name} is missing`)
    return match[1]
})
vm.runInNewContext(methods.join("\n"), row, { filename: path })

row.load()
row.reload()
deferred.shift()()

// Scrolling can queue another request without invalidating this reload's first one.
row.root.lastBlock++
row.load()
assert.equal(replies.length, 3)

// The old reply beats the correction for the same channel and start time.
replies[0]([{ start: 100, stop: 200, title: "Old title" }])
const programmesAfterOldReply = row.programmes.length
replies[1]([{ start: 100, stop: 200, title: "Corrected title" }])
replies[2]([{ start: 3700, stop: 3800, title: "Later programme" }])
assert.equal(row.programmes.length, 2)
assert.equal(row.programmes[0].title, "Corrected title")
assert.equal(row.programmes[1].title, "Later programme")
assert.equal(programmesAfterOldReply, 0)

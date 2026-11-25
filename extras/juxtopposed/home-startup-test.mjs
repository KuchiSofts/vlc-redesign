// Copyright (C) 2026 VLC authors and VideoLAN
// SPDX-License-Identifier: GPL-2.0-or-later
import assert from "node:assert/strict"
import { readFileSync } from "node:fs"

// Exercise the geometry calculation from QML with plain objects. The real
// Column's layout, Loader activation and keyboard focus still need Qt Quick.
const source = readFileSync(new URL("../../modules/gui/qt/medialibrary/qml/HomePage.qml", import.meta.url), "utf8")
const helper = source.match(/^    function _sectionMayBeVisible\(header: Item\): bool \{([\s\S]*?)^    \}/m)
assert.ok(helper, "Home's viewport calculation must be present")
const sectionMayBeVisible = new Function("header", "mediaRows", "flickable", helper[1])

const header = { visible: true, height: 40 }
const rows = {
    y: 17,
    contentItem: { y: 3 },
    spacing: 5,
    contentChildren: [
        { visible: true, height: 40 },
        { visible: true, height: 200 },
        { visible: false, height: 1000 },
        { visible: true, height: 0 },
        header
    ]
}

assert.equal(sectionMayBeVisible(header, rows, { contentY: 0, height: 0 }), false)
assert.equal(sectionMayBeVisible(header, rows, { contentY: 0, height: 270 }), false)
assert.equal(sectionMayBeVisible(header, rows, { contentY: 0, height: 271 }), true)
assert.equal(sectionMayBeVisible(header, rows, { contentY: 20, height: 251 }), true)
assert.equal(sectionMayBeVisible(header, { ...rows, contentChildren: [header] },
                                   { contentY: 0, height: 100 }), true)
assert.equal(sectionMayBeVisible({}, rows, { contentY: 0, height: 100 }), true)

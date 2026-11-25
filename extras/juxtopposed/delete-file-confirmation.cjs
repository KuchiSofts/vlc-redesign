// Copyright (C) 2026 VLC authors and VideoLAN
// SPDX-License-Identifier: GPL-2.0-or-later

const assert = require("node:assert/strict")
const { readFileSync } = require("node:fs")
const { test } = require("node:test")
const { runInNewContext } = require("node:vm")

const menu = readFileSync("modules/gui/qt/medialibrary/qml/MLContextMenu.qml", "utf8")
// Run the QML action itself with stubbed dialogs and model so no GUI is needed.
const action = menu.match(/^    function deleteFileFromSource\([\s\S]*?^    }/m)?.[0]
assert.ok(action)

const selected = { id: 2, mrl: "file:///B.mp4" }
const trash = [selected.id, selected.mrl, true]
const permanent = [selected.id, selected.mrl, false]

function confirmDeletion(answers, trashFailed) {
    const rows = [selected]
    const calls = []
    let retry
    let prompt = 0
    const model = {
        deleteFileFromSource(id, mrl, toTrash, onTrashFailed) {
            calls.push([id, mrl, toTrash])
            if (toTrash && trashFailed)
                retry = onTrashFailed
        }
    }
    const deleteFile = runInNewContext(`(${action})`, {
        model,
        idDataRole: "id",
        Qt: { platform: { os: "windows" } },
        qsTr: text => text,
        DialogsProvider: {
            questionDialog() {
                assert.ok(prompt < answers.length, "unexpected confirmation")
                rows.unshift({ id: 3 + prompt, mrl: `file:///A${answers.length - prompt}.mp4` })
                return answers[prompt++]
            }
        }
    })
    deleteFile(rows.slice(), {}, [{ row: 0 }])
    if (retry)
        retry()
    assert.equal(prompt, answers.length)
    return calls
}

test("a library refresh during the trash prompt keeps the original media and path", () => {
    assert.deepEqual(confirmDeletion([true], false), [trash])
})

test("a second refresh during the permanent-delete prompt keeps the same target", () => {
    assert.deepEqual(confirmDeletion([true, true], true), [trash, permanent])
})

test("cancelling either confirmation stops that delete attempt", () => {
    assert.deepEqual(confirmDeletion([false], false), [])
    assert.deepEqual(confirmDeletion([true, false], true), [trash])
})

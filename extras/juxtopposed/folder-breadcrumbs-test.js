// Copyright (C) 2026 VLC authors and VideoLAN
// SPDX-License-Identifier: GPL-2.0-or-later

const assert = require("node:assert/strict")
const fs = require("node:fs")

// Run the JavaScript from the QML singleton without loading Qt Quick.
const qml = fs.readFileSync("modules/gui/qt/util/qml/Helpers.qml", "utf8")
const helper = qml.match(/^    function folderBreadcrumbs\(url : var\) : var \{([\s\S]*?)^    \}/m)
assert.ok(helper, "folderBreadcrumbs must be defined in Helpers.qml")
const folderBreadcrumbs = new Function("url", helper[1])

function check(url, expected) {
    assert.deepEqual(folderBreadcrumbs(url).map(crumb => [crumb.title, crumb.url]), expected, url)
}

check("file://nas/library/shows/season/", [
    ["nas", "file://nas"],
    ["library", "file://nas/library"],
    ["shows", "file://nas/library/shows"],
    ["season", "file://nas/library/shows/season"]
])
check("file:///C:/Shows/Season", [
    ["C:", "file:///C:/"],
    ["Shows", "file:///C:/Shows"],
    ["Season", "file:///C:/Shows/Season"]
])
check("file://nas/library/Rock%20%26%20Roll/%E6%97%A5%23%25%2F/Season", [
    ["nas", "file://nas"],
    ["library", "file://nas/library"],
    ["Rock & Roll", "file://nas/library/Rock%20%26%20Roll"],
    ["日#%/", "file://nas/library/Rock%20%26%20Roll/%E6%97%A5%23%25%2F"],
    ["Season", "file://nas/library/Rock%20%26%20Roll/%E6%97%A5%23%25%2F/Season"]
])
check("file:///nas/library/%FF", [
    ["nas", "file:///nas"],
    ["library", "file:///nas/library"],
    ["%FF", "file:///nas/library/%FF"]
])
assert.equal(folderBreadcrumbs("file://nas/library")[0].isAuthority, true)
assert.equal(folderBreadcrumbs("file:///nas/library")[0].isAuthority, undefined)
check("file:///", [])
check("", [])

console.log("folder breadcrumbs passed")

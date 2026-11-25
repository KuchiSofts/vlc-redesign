// Copyright (C) 2026 VLC authors and VideoLAN
// SPDX-License-Identifier: GPL-2.0-or-later

const assert = require("node:assert/strict")
const fs = require("node:fs")
const vm = require("node:vm")

const qml = fs.readFileSync("modules/gui/qt/widgets/qml/ExpandGridView.qml", "utf8").replaceAll("\r\n", "\n")
const animation = qml.match(/^    FrameAnimation \{\n[^]*?running: ([^\n]+)\n +onTriggered: ([^\n]+)/m)
const functions = [
    "_prefetchOne", "_incubatePrefetch", "_prunePrefetch", "_invalidatePrefetch",
    "_markPrefetchedDataChanged", "_takePrefetchedItem", "_delegateData", "_itemProperties",
    "_containsItem", "_getItem", "_setItem", "_setupChild", "_updatePosition", "_indexToZ"
]
const methods = functions.map(name => qml.match(new RegExp("^    function " + name + "\\(.*?\\n    }", "ms"))[0])

// Run the grid's own JavaScript with stand-ins for Qt's frame and incubation delivery.
function grid() {
    const requests = []
    const reads = []
    const selected = new Set()
    let revision = 0
    const Component = { Ready: 1, Loading: 2, Error: 3 }
    const context = vm.createContext({
        Component,
        console,
        visible: true,
        isAnimating: false,
        activeFocus: true,
        currentIndex: 8,
        expandIndex: -1,
        _currentFocusReason: "keyboard",
        _count: 14,
        _currentRange: [4, 8],
        _idChildrenList: [],
        _delayedChildrenMap: {},
        _unusedItemList: [],
        _prefetchedItems: {},
        _pendingPrefetch: null,
        _prefetchGeneration: 0,
        _prefetchRequested: true,
        _prefetchForward: true,
        _expandItemVerticalSpace: 125,
        nbItemPerRow: 4,
        delegateRoles: null,
        model: { getDataAt(id, ...roles) {
            reads.push([id, ...roles])
            return { revision }
        } },
        selectionModel: { isSelected: id => selected.has(id) },
        flickable: { contentItem: {}, getExpandItemGridId: () => 8 },
        getItemPos: id => [(id % 4) * 30, Math.floor(id / 4) * 50],
        getItemRowCol: id => [id % 4, Math.floor(id / 4)],
        delegate: { incubateObject(parent, properties) {
            const item = {
                ...properties,
                destroyed: false,
                destroy() { this.destroyed = true },
                forceActiveFocus(reason) { this.focusReason = reason }
            }
            const request = {
                item,
                status: Component.Loading,
                object: null,
                finish(status = Component.Ready) {
                    this.status = status
                    if (status === Component.Ready)
                        this.object = item
                    this.onStatusChanged?.(this.status)
                },
                forceCompletion() { this.finish() }
            }
            requests.push(request)
            return request
        } }
    })
    context.root = context
    vm.runInContext(methods.join("\n"), context)

    return {
        context, requests, reads, selected,
        updateData() { revision += 1 },
        frame() {
            if (vm.runInContext(animation[1], context))
                vm.runInContext(animation[2], context)
        }
    }
}

{
    const { context, requests, frame } = grid()
    frame()
    frame()
    assert.deepEqual(requests.map(request => request.item.index), [8])
    assert.equal(requests[0].item.y, 225)
    assert.equal(requests[0].item.visible, false)
    assert.equal(requests[0].item.focus, false)
    assert.equal(context._containsItem(8), false)

    for (let i = 0; i < 8; ++i) {
        assert.equal(requests.length, i + 1)
        requests[i].finish()
        frame()
    }
    assert.deepEqual(requests.map(request => request.item.index), [8, 9, 10, 11, 0, 1, 2, 3])
    assert.equal(context._prefetchRequested, false)
}

{
    const { context, requests, reads, selected, updateData, frame } = grid()
    context.delegateRoles = ["title"]
    frame()
    updateData()
    context._markPrefetchedDataChanged(8, 9)
    requests[0].finish()
    frame()
    assert.equal(requests.length, 1)
    assert.equal(reads.length, 2)
    assert.deepEqual(reads[0], [8, ["title"]])

    selected.add(8)
    context._currentRange = [8, 12]
    context._setupChild(8, 125)
    const item = context._getItem(8)
    assert.equal(item, requests[0].item)
    assert.equal(item.model.revision, 1)
    assert.equal(item.selected, true)
    assert.equal(item.visible, true)
    assert.equal(item.y, 225)
    assert.equal(item.focusReason, "keyboard")
    assert.equal(reads.length, 2)
    assert.equal(context._containsItem(8), true)
}

{
    const { context, requests, updateData, frame } = grid()
    frame()
    updateData()
    context._markPrefetchedDataChanged(8, 9)
    context._currentRange = [8, 12]
    const item = context._takePrefetchedItem(8, 0, 42)
    assert.equal(item, requests[0].item)
    assert.equal(item.model.revision, 1)
    assert.equal(context._pendingPrefetch, null)

    frame()
    assert.equal(requests[1].item.index, 12)
    context._invalidatePrefetch()
    assert.equal(context._takePrefetchedItem(12, 0, 0), null)
    requests[1].finish()
    assert.equal(requests[1].item.destroyed, true)
}

{
    const { context, requests, frame } = grid()
    frame()
    context._currentRange = [12, 14]
    context._prunePrefetch()
    context._prefetchForward = false
    requests[0].finish()
    // Row 8 is still adjacent. An actual jump out of the buffer must drop it.
    assert.equal(requests[0].item.destroyed, false)
    context._currentRange = [0, 4]
    context._prunePrefetch()
    assert.equal(requests[0].item.destroyed, true)

    context._currentRange = [8, 12]
    context._delayedChildrenMap[4] = {}
    frame()
    assert.equal(requests[1].item.index, 5)
    requests[1].finish(context.Component.Error)
    context._unusedItemList = [{}, {}, {}, {}]
    frame()
    assert.equal(requests.length, 2)
    assert.equal(context._prefetchRequested, false)
}

{
    const { context, requests, frame } = grid()
    frame()
    context._currentRange = [8, 12]
    context._unusedItemList = [{}]
    assert.equal(context._takePrefetchedItem(8, 0, 0), null)
    assert.equal(requests[0].status, context.Component.Loading)
    requests[0].finish()
    assert.equal(requests[0].item.destroyed, true)
}

console.log("expand-grid-prefetch: bounded frames, promotion, freshness, and invalidation passed")

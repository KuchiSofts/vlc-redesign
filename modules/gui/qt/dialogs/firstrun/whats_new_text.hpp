/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/

#pragma once

#include <QString>
#include <QVariantList>

// The text of "What's new" (WhatsNewController): the headlines of a version's
// section of CHANGELOG.md. Qt Core only, so that
// extras/juxtopposed/whats-new.cpp tests it without the interface.
namespace WhatsNewText {

// the headlines of a group shown, the others counted
constexpr int maxItemsPerGroup = 6;
constexpr int maxHeadlineLength = 140;

// The headline of a bullet: its bold lead ("**Subtitle languages that
// learn.** Each subtitle..."), else its first sentence, as plain text.
// Without what the changelog keeps for its readers and the card has no
// room for: a reference to an issue or to who asked ("(#72, by @someone's
// request; Fixes #72)"), the detail after a long lead's colon, and, in a
// headline still long, its remarks in brackets. Cut between two words.
QString headline(const QString& bullet);

// The headlines of the "## [version]" section of a changelog, else of
// "## [Unreleased]" (a build between two releases); ownSection: the version
// has its own.
// [{ kind: "added" | "changed" | "fixed", items: [text...], more: n }]
QVariantList parseChangelog(const QString& changelog, const QString& version,
                            bool* ownSection = nullptr);

}

/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/

#include "whats_new_text.hpp"

#include <QRegularExpression>
#include <QStringList>
#include <QVariantMap>

#include <algorithm>

namespace WhatsNewText {

namespace {

// The text of a bit of Markdown: links, code and emphasis without their marks
QString plainText(QString text)
{
    static const QRegularExpression link(QStringLiteral(R"(\[([^\]]*)\]\([^)]*\))"));
    static const QRegularExpression emphasis(QStringLiteral(R"((\*\*|\*|__)(.+?)\1)"));
    text.replace(link, QStringLiteral("\\1"));
    text.remove(QLatin1Char('`'));
    text.replace(emphasis, QStringLiteral("\\2"));
    return text.simplified();
}

} // namespace

QString headline(const QString& bullet)
{
    // "(#72, by @someone's request; Fixes #72)": brackets that name an issue,
    // a pull request or a person
    static const QRegularExpression reference(QStringLiteral(R"(\s*\((?=[^()]*(?:#\d|@\w))[^()]*\))"));
    // any remark in brackets
    static const QRegularExpression remark(QStringLiteral(R"(\s*\([^()]*\))"));
    // a lead this long before a colon says it all: what follows is its detail
    // ("Live TV: ..." is a topic, not a lead)
    constexpr int leadBeforeColon = 40;
    // a headline longer than this loses its remarks in brackets
    constexpr int comfortableLength = 100;

    QString lead;
    if (bullet.startsWith(QLatin1String("**"))) {
        const int end = bullet.indexOf(QLatin1String("**"), 2);
        if (end > 2)
            lead = plainText(bullet.mid(2, end - 2));
    }
    if (!lead.isEmpty()) {
        lead.remove(reference);
    } else {
        lead = plainText(bullet);
        lead.remove(reference);
        const int stop = lead.indexOf(QLatin1String(". "));
        if (stop > 0)
            lead.truncate(stop);
        const int colon = lead.indexOf(QLatin1String(": "), leadBeforeColon);
        if (colon > 0)
            lead.truncate(colon);
    }
    if (lead.size() > comfortableLength)
        lead.remove(remark);
    lead = lead.simplified();
    while (lead.endsWith(QLatin1Char('.')) || lead.endsWith(QLatin1Char(':')))
        lead.chop(1);
    lead = lead.trimmed();
    if (lead.size() > maxHeadlineLength) {
        // between two words (a word alone longer than a third of the line: inside it)
        QString cut = lead.left(maxHeadlineLength - 1);
        const int space = cut.lastIndexOf(QLatin1Char(' '));
        if (lead[maxHeadlineLength - 1] != QLatin1Char(' ') && space > maxHeadlineLength * 2 / 3)
            cut.truncate(space);
        while (!cut.isEmpty() && QStringLiteral(" ,;:(").contains(cut.back()))
            cut.chop(1);
        lead = cut + QChar(0x2026);
    }
    return lead;
}

QVariantList parseChangelog(const QString& changelog, const QString& version, bool* ownSection)
{
    QStringList lines = changelog.split(QLatin1Char('\n'));
    for (QString& line : lines) {
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
    }
    const auto find = [&lines](const QString& name) {
        const QString heading = QStringLiteral("## [") + name + QLatin1Char(']');
        for (int i = 0; i < lines.size(); ++i) {
            if (lines[i].startsWith(heading))
                return i;
        }
        return -1;
    };
    // the section of the version (a release), else the one it prepares
    int start = find(version);
    if (ownSection)
        *ownSection = start >= 0;
    if (start < 0)
        start = find(QStringLiteral("Unreleased"));
    if (start < 0)
        return {};

    static const char* const kinds[] = { "added", "changed", "fixed" };
    QStringList items[3];
    int group = -1;
    QString bullet;
    // a bullet of a bullet: its lines are not the headline's
    bool nested = false;
    const auto flush = [&]() {
        if (group >= 0 && !bullet.isEmpty()) {
            const QString text = headline(bullet);
            if (!text.isEmpty())
                items[group].append(text);
        }
        bullet.clear();
        nested = false;
    };

    for (int i = start + 1; i < lines.size(); ++i) {
        const QString& line = lines[i];
        if (line.startsWith(QLatin1String("## ")))
            break; // the next version
        if (line.startsWith(QLatin1String("### "))) {
            flush();
            const QString name = line.mid(4).trimmed();
            group = -1;
            for (int k = 0; k < 3; ++k) {
                if (name.compare(QLatin1String(kinds[k]), Qt::CaseInsensitive) == 0)
                    group = k;
            }
            continue; // Technical, Documentation...: not for the card
        }
        if (group < 0)
            continue;
        if (line.startsWith(QLatin1String("- ")) || line.startsWith(QLatin1String("* "))) {
            flush();
            bullet = line.mid(2).trimmed();
            continue;
        }
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            flush();
            continue;
        }
        if (bullet.isEmpty())
            continue;
        if (trimmed.startsWith(QLatin1String("- ")) || trimmed.startsWith(QLatin1String("* ")))
            nested = true;
        else if (!nested && line.startsWith(QLatin1Char(' ')))
            bullet += QLatin1Char(' ') + trimmed;
    }
    flush();

    QVariantList result;
    for (int k = 0; k < 3; ++k) {
        if (items[k].isEmpty())
            continue;
        const int more = std::max(0, int(items[k].size()) - maxItemsPerGroup);
        result.append(QVariantMap{
            {QStringLiteral("kind"), QString::fromLatin1(kinds[k])},
            {QStringLiteral("items"), QStringList(items[k].mid(0, maxItemsPerGroup))},
            {QStringLiteral("more"), more},
        });
    }
    return result;
}

}

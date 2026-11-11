/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * The headlines of "What's new" (modules/gui/qt/dialogs/firstrun/
 * whats_new_text.cpp): what the card shows of a changelog's bullets. A
 * bullet's bold lead or its first sentence; never the references the
 * changelog keeps for its readers ("(#72, by @someone's request; Fixes
 * #72)"), nor the detail after a long lead's colon; cut between two words.
 *   g++ -std=c++17 -O1 -Wall -Wextra -fPIC -I. $(pkg-config --cflags Qt6Core) \
 *       extras/juxtopposed/whats-new.cpp modules/gui/qt/dialogs/firstrun/whats_new_text.cpp \
 *       $(pkg-config --libs Qt6Core) -o whats-new && ./whats-new
 */
#include "modules/gui/qt/dialogs/firstrun/whats_new_text.hpp"

#include <QStringList>
#include <QVariantMap>
#include <cstdio>

using namespace WhatsNewText;

static int checks = 0;
static int failures = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)
#define CHECK_EQ(text, expected) do { ++checks; const QString got = (text); if (got != QStringLiteral(expected)) { \
    std::fprintf(stderr, "FAIL line %d: \"%s\"\n      expected \"%s\"\n", __LINE__, qPrintable(got), expected); \
    ++failures; } } while (0)

int main()
{
    // ---- one bullet ---------------------------------------------------
    // the bold lead is the headline, without its full stop
    CHECK_EQ(headline(QStringLiteral("**Subtitle languages that learn.** Each subtitle you choose counts.")),
             "Subtitle languages that learn");
    // else the first sentence
    CHECK_EQ(headline(QStringLiteral("The search field has a clear button while it holds text. Tracks shows its count.")),
             "The search field has a clear button while it holds text");
    // links, code and emphasis without their marks
    CHECK_EQ(headline(QStringLiteral("The `--foo` option of [the wiki](https://example.org/a.b) is *gone*.")),
             "The --foo option of the wiki is gone");

    // the references of the changelog are not for the card: an issue, who
    // asked, "Fixes #n" (0.6.0 showed "(#72, by @BarbodF's request; Fixes
    // #72): Settings > Int..." under Added)
    CHECK_EQ(headline(QStringLiteral(
                 "In full screen, a shortcut no longer has to show the playback controls, as in VLC 3 "
                 "([#72](https://github.com/KuchiSofts/vlc-redesign/issues/72), by "
                 "[@BarbodF](https://github.com/BarbodF)'s request; Fixes #72): **Settings › Interface › "
                 "Playback Controls › When you use a shortcut in full screen**: Show the playback controls.")),
             "In full screen, a shortcut no longer has to show the playback controls, as in VLC 3");
    CHECK_EQ(headline(QStringLiteral("Online subtitles on Android ([#73](https://example.org/73), reported by [@Someone](https://example.org)).")),
             "Online subtitles on Android");
    CHECK_EQ(headline(QStringLiteral("**Hold to play faster** (#58). The space bar held plays at twice the speed.")),
             "Hold to play faster");
    CHECK_EQ(headline(QStringLiteral("**Hold to play faster (#58).** The space bar held plays at twice the speed.")),
             "Hold to play faster");
    // (a remark without a reference stays while the headline is short)
    CHECK_EQ(headline(QStringLiteral("A jump to the very start shows 00:00 (not \"--:--\").")),
             "A jump to the very start shows 00:00 (not \"--:--\")");

    // a long lead before a colon is the headline, what follows is its detail
    CHECK_EQ(headline(QStringLiteral(
                 "Every dialog and window opens inside its screen: the dialogs of the interface are sized from the "
                 "window and scroll when it is too low for them.")),
             "Every dialog and window opens inside its screen");
    // a short topic before a colon is not a headline by itself
    CHECK_EQ(headline(QStringLiteral("Live TV: Update downloads the guide again.")),
             "Live TV: Update downloads the guide again");
    // (the colon after the topic is passed, the next one ends the headline)
    CHECK_EQ(headline(QStringLiteral(
                 "Live TV: a server that stops sending no longer holds it: a download with nothing new for 30 s "
                 "stops and is tried again.")),
             "Live TV: a server that stops sending no longer holds it");

    // a long headline loses its remarks in brackets before it is cut
    CHECK_EQ(headline(QStringLiteral(
                 "Open Recent, the Recent page, Ctrl+1 to Ctrl+0 and Recent of the library beside the video list the "
                 "last played first (they showed the oldest first after a restart, and the menu the other way).")),
             "Open Recent, the Recent page, Ctrl+1 to Ctrl+0 and Recent of the library beside the video list the "
             "last played first");
    // and is cut between two words, never inside one
    {
        const QString text = headline(QStringLiteral(
            "The subtitle language lists no longer say American English as the own name of Herero, Haitian or Hiri "
            "Motu, and list what you type in alphabetical order whichever language the interface speaks today"));
        CHECK(text.size() <= maxHeadlineLength);
        CHECK(text.endsWith(QChar(0x2026)));
        CHECK(!text.contains(QStringLiteral(" …")));
        // (the word before the ellipsis is whole: the text goes on with a space there)
        const QString kept = text.left(text.size() - 1);
        CHECK(QStringLiteral(
                  "The subtitle language lists no longer say American English as the own name of Herero, Haitian or "
                  "Hiri Motu, and list what you type in alphabetical order whichever language the interface speaks "
                  "today").startsWith(kept + QLatin1Char(' ')));
    }
    // nothing left of a bullet that was only a reference
    CHECK_EQ(headline(QStringLiteral("(#12)")), "");

    // ---- a section ----------------------------------------------------
    const QString changelog = QStringLiteral(
        "# Changelog\n\n## [Unreleased]\n\n### Changed\n\n- Work started.\n\n"
        "## [1.2.0] · 2026-01-02\n\nA paragraph of the release.\n\n"
        "### Added\n\n- **A new page.** With details\n  on a second line.\n"
        "- Shortcuts in full screen ([#7](https://example.org/7); Fixes #7): the\n  setting and its three choices.\n\n"
        "### Technical\n\n- Not for the card.\n\n"
        "### Fixed\n\n- One.\n- Two.\n  - a nested line\n- Three.\n- Four.\n- Five.\n- Six.\n- Seven.\n\n"
        "## [1.1.0] · 2026-01-01\n\n### Added\n\n- Older.\n");
    bool own = false;
    const QVariantList groups = parseChangelog(changelog, QStringLiteral("1.2.0"), &own);
    CHECK(own);
    CHECK(groups.size() == 2);
    if (groups.size() == 2) {
        const QVariantMap added = groups[0].toMap(), fixed = groups[1].toMap();
        CHECK(added.value(QStringLiteral("kind")).toString() == QLatin1String("added"));
        CHECK(added.value(QStringLiteral("items")).toStringList()
              == QStringList({QStringLiteral("A new page"),
                              QStringLiteral("Shortcuts in full screen: the setting and its three choices")}));
        CHECK(added.value(QStringLiteral("more")).toInt() == 0);
        CHECK(fixed.value(QStringLiteral("kind")).toString() == QLatin1String("fixed"));
        CHECK(fixed.value(QStringLiteral("items")).toStringList().size() == maxItemsPerGroup);
        CHECK(fixed.value(QStringLiteral("items")).toStringList().value(1) == QLatin1String("Two"));
        CHECK(fixed.value(QStringLiteral("more")).toInt() == 1);
    }
    // a build between two releases: the section it prepares
    own = true;
    const QVariantList next = parseChangelog(changelog, QStringLiteral("1.3.0"), &own);
    CHECK(!own);
    CHECK(next.size() == 1 && next[0].toMap().value(QStringLiteral("items")).toStringList()
                                  == QStringList({QStringLiteral("Work started")}));
    // no changelog, or none of its sections
    CHECK(parseChangelog(QString(), QStringLiteral("1.0.0")).isEmpty());
    CHECK(parseChangelog(QStringLiteral("# Changelog\n\n## [0.9.0]\n\n### Added\n\n- x\n"), QStringLiteral("1.0.0")).isEmpty());

    std::printf("what's new: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}

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
    // a line for another system only: not on this one's card
    CHECK(forOtherSystem(QStringLiteral("Android: lyrics in the music player.")));
    CHECK(forOtherSystem(QStringLiteral("**Samsung TV app:** the guide's colours.")));
    CHECK(!forOtherSystem(QStringLiteral("Live TV: the reasons said in the language.")));
    CHECK(!forOtherSystem(QStringLiteral("Updates: GitHub's limit of requests.")));
    CHECK(!forOtherSystem(QStringLiteral("A sentence: with a colon far after its start, not a system's name at all.")));
#if defined(__linux__)
    CHECK(forOtherSystem(QStringLiteral("Windows: the installer.")));
    CHECK(!forOtherSystem(QStringLiteral("Linux: the HTTP proxy settings.")));
#endif
    {
        const QVariantList mixed = parseChangelog(QStringLiteral(
            "## [2.0.0]\n\n### Fixed\n\n- Android: one.\n- Two for every copy.\n- Samsung TV app: three.\n"),
            QStringLiteral("2.0.0"));
        CHECK(mixed.size() == 1 && mixed[0].toMap().value(QStringLiteral("items")).toStringList()
                                       == QStringList({QStringLiteral("Two for every copy")}));
    }
    // several systems named together: another system's line only when all are
    CHECK(forOtherSystem(QStringLiteral("**Android TV and Samsung TV:** the remote's Back key.")));
    CHECK(forOtherSystem(QStringLiteral("Android phones and tablets: the sort dialog.")));
    CHECK(forOtherSystem(QStringLiteral("Android, Samsung TV: the queue.")));
    CHECK(!forOtherSystem(QStringLiteral("TV: the guide keeps its place.")));
    CHECK(!forOtherSystem(QStringLiteral("Phones and tablets: a sentence that names no app.")));
    CHECK(!forOtherSystem(QStringLiteral("Android files on this computer: read by their names.")));
#if defined(__linux__)
    CHECK(forOtherSystem(QStringLiteral("Windows and macOS: the title bar.")));
    CHECK(!forOtherSystem(QStringLiteral("Windows and Linux: the tray icon.")));
    CHECK(!forOtherSystem(QStringLiteral("Linux, Android: the lyrics.")));
#endif
    // the update card's lines of a release's notes: the kinds of change only,
    // a wrapped bullet whole, other systems' lines left out, four at most
    {
        const QString notes = QStringLiteral(
            "# A Player 2.0.0\n\n"
            "> The interface is **someone's** design\n\n"
            "An update with:\n- not a change, a line of the introduction\n\n"
            "### Added\n\n"
            "- **A scroll bar in the panel** ([#143](https://example.com/143)), as on the\n"
            "  library pages.\n"
            "- **Samsung TV: the queue into a playlist**.\n"
            "- **Android TV and Samsung TV: the Back key**.\n\n"
            "### Fixed\n\n"
            "- **Unmuting brings sound back**, even after restarting\n"
            "  ([#145](https://example.com/145)).\n"
            "  - a detail of that line\n"
            "    over two lines\n"
            "- A name_with_underscores stays whole.\n"
            "* Fourth for this system.\n"
            "- Fifth: never shown.\n\n"
            "## Downloads\n\n- a file\n");
        CHECK_EQ(releaseSummary(notes),
                 "A scroll bar in the panel (#143), as on the library pages.\n"
                 "Unmuting brings sound back, even after restarting (#145).\n"
                 "A name_with_underscores stays whole.\n"
                 "Fourth for this system.");
        CHECK_EQ(releaseSummary(notes, 1), "A scroll bar in the panel (#143), as on the library pages.");
        // notes without those parts: their first bullets, up to the next part
        CHECK_EQ(releaseSummary(QStringLiteral("Some words.\r\n\r\n- One.\r\n- Android: two.\r\n- Three\r\n  and its end.\r\n\r\n## Downloads\r\n\r\n- a file\r\n")),
                 "One.\nThree and its end.");
        // the last bullet of notes that end without an empty line
        CHECK_EQ(releaseSummary(QStringLiteral("### Fixed\n- Only one")), "Only one");
        CHECK_EQ(releaseSummary(QString()), "");
        // a wrapped line that begins with an issue's number is text, not a heading
        CHECK_EQ(releaseSummary(QStringLiteral("### Fixed\n- First line (see\n  #145).\n- Second.\n### Changed\n- Third.\n")),
                 "First line (see #145).\nSecond.\nThird.");
        // a "####" inside a part keeps its bullets; kinds written "## Added" count as "### Added"
        CHECK_EQ(releaseSummary(QStringLiteral("### Added\n#### Player\n- a\n### Fixed\n- c\n")), "a\nc");
        CHECK_EQ(releaseSummary(QStringLiteral("## Added\n\n- Android: x\n\n## Fixed\n\n- y\n\n## Downloads\n\n- z\n")), "y");
        // bullets all indented alike are bullets, not bullets of a bullet; "+" and numbered lists
        CHECK_EQ(releaseSummary(QStringLiteral("### Fixed\n\n  - one\n  - two\n    - of two\n  - three\n")), "one\ntwo\nthree");
        CHECK_EQ(releaseSummary(QStringLiteral("### Changed\n+ a\n+ b\n### Fixed\n1. c\n2) d\n")), "a\nb\nc\nd");
        // italics inside a bold lead, a picture
        CHECK_EQ(releaseSummary(QStringLiteral("### Fixed\n- **The arrows stop at *Show the video here too*.** More ![shot](https://example.com/a.png).\n")),
                 "The arrows stop at Show the video here too. More shot.");
        CHECK_EQ(releaseSummary(QStringLiteral("### Fixed\n- a\n- b\n"), 0), "");
        CHECK_EQ(releaseSummary(QStringLiteral("No bullet at all.\n\n## Downloads\n")), "");
    }
    // no changelog, or none of its sections
    CHECK(parseChangelog(QString(), QStringLiteral("1.0.0")).isEmpty());
    CHECK(parseChangelog(QStringLiteral("# Changelog\n\n## [0.9.0]\n\n### Added\n\n- x\n"), QStringLiteral("1.0.0")).isEmpty());

    std::printf("what's new: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}

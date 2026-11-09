/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * Windows that fit their screen (modules/gui/qt/util/screen_fit.cpp): the
 * size Automatic finds for "Interface size" and its choices, the size of
 * the main window at the first start, a rectangle put inside a screen (one at the left of or above the
 * main one too: negative coordinates), and whether a saved geometry can
 * still be used.
 *   g++ -std=c++17 -O1 -Wall -Wextra -fPIC -I. $(pkg-config --cflags Qt6Core) \
 *       extras/juxtopposed/screen-fit.cpp modules/gui/qt/util/screen_fit.cpp \
 *       $(pkg-config --libs Qt6Core) -o screen-fit && ./screen-fit
 */
#include "modules/gui/qt/util/screen_fit.hpp"
#include <cmath>
#include <cstdio>

using namespace ScreenFit;

static int checks = 0;
static int failures = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

static bool inside(const QRect &inner, const QRect &outer)
{
    return outer.contains(inner);
}

int main()
{
    // ---- the first window --------------------------------------------
    // (the area a window may use: the screen without a 48 pixel taskbar)
    const QSize fullHd(1920, 1032), laptop(1366, 720), hd(1280, 672), qhd(2560, 1392),
                uhd(3840, 2112), ultraWide(3440, 1392), tiny(911, 464);

    // Full HD: the design's window
    CHECK(defaultWindowSize(fullHd) == QSize(1280, 720));
    // larger screens: a larger window, not the same 1280 x 720 lost in them
    CHECK(defaultWindowSize(qhd).width() >= 1600);
    CHECK(defaultWindowSize(uhd).width() >= 2400);
    // an ultra-wide one: from its height
    CHECK(defaultWindowSize(ultraWide).height() <= 1392 * 0.9);
    CHECK(defaultWindowSize(ultraWide).width() < 3440 * 0.66);
    // always 16:9 (a pixel of rounding), inside nine tenths of the area
    for (const QSize &area : { fullHd, laptop, hd, qhd, uhd, ultraWide, tiny, QSize(1024, 720), QSize(800, 1232) })
    {
        for (double scale : { 1.0, 1.1, 1.25, 1.5, 1.75, 2.0 })
        {
            const QSize size = defaultWindowSize(area, scale);
            CHECK(size.width() <= std::floor(area.width() * 0.9));
            CHECK(size.height() <= std::floor(area.height() * 0.9));
            CHECK(size.width() >= 300 && size.height() >= 160);
            // 16:9 unless the screen is narrower than that (a portrait one)
            if (area.width() * 9 >= area.height() * 16)
                CHECK(std::abs(size.width() * 9 - size.height() * 16) <= 16);
        }
    }
    // a small screen: smaller than it, yet most of it
    CHECK(defaultWindowSize(laptop).width() < 1366 && defaultWindowSize(laptop).width() >= 1100);
    CHECK(defaultWindowSize(laptop).height() < 720);
    // a larger interface asks for a larger window when the screen has room
    CHECK(defaultWindowSize(uhd, 1.5).width() >= 1920);
    CHECK(defaultWindowSize(qhd, 1.25).width() >= 1600);
    CHECK(defaultWindowSize(fullHd, 1.5) == QSize(1650, 928)); // nine tenths of the height: no more
    // no screen (a headless start): the design's size
    CHECK(defaultWindowSize(QSize()) == QSize(1280, 720));

    // ---- "Interface size": Automatic --------------------------------
    // a usual desktop, whatever its size: the design's own
    CHECK(automaticInterfaceSize(1920, 1.0, {531, 299}) == 100);
    CHECK(automaticInterfaceSize(2560, 1.0, {597, 336}) == 100);
    CHECK(automaticInterfaceSize(3440, 1.0, {800, 335}) == 100);
    CHECK(automaticInterfaceSize(1440, 1.0, {}) == 100);
    // a 4K laptop the system scales to 250 %: its pixels are already scaled
    CHECK(automaticInterfaceSize(1536, 1.0, {344, 194}) == 100);
    // a screen small in scaled pixels: the rule of the TV and tablet apps
    CHECK(automaticInterfaceSize(1366, 1.0, {344, 194}) == 105);
    CHECK(automaticInterfaceSize(1280, 1.0, {}) == 115);
    CHECK(automaticInterfaceSize(1024, 1.0, {}) == 140);
    CHECK(automaticInterfaceSize(960, 1.0, {}) == 150); // (a TV: 960 across)
    CHECK(automaticInterfaceSize(800, 1.0, {}) == 150);
    // 4K left at 100 % (X11 with no scale): 27 inches is 163 dpi, 15.6 is 282
    CHECK(automaticInterfaceSize(3840, 1.0, {597, 336}) == 135);
    CHECK(automaticInterfaceSize(3840, 1.0, {344, 194}) == 150);
    // the same with the system's text size doubled (Xft.dpi 192): the
    // interface is already twice as large
    CHECK(automaticInterfaceSize(3840, 2.0, {597, 336}) == 100);
    CHECK(automaticInterfaceSize(3840, 2.0, {344, 194}) == 100);
    // a 55 inch 4K TV at 100 %: not dense
    CHECK(automaticInterfaceSize(3840, 1.0, {1210, 680}) == 100);
    // a monitor that says nothing, or nonsense
    CHECK(automaticInterfaceSize(3840, 1.0, {}) == 100);
    CHECK(automaticInterfaceSize(3840, 1.0, {16, 9}) == 100);
    CHECK(automaticInterfaceSize(1920, 1.0, {160000, 90000}) == 100);
    CHECK(automaticInterfaceSize(0, 1.0, {}) == 100);
    CHECK(automaticInterfaceSize(1920, 0.0, {}) == 100);
    // always 100 ... 150, in steps of 5
    for (int w = 160; w <= 7680; w += 40)
        for (double mm : { 0., 120., 300., 600., 1200., 2500. })
            for (double system : { 0.5, 1.0, 1.25, 2.0 })
            {
                const int percent = automaticInterfaceSize(w, system, { mm, mm * 9 / 16 });
                CHECK(percent >= 100 && percent <= 150 && percent % 5 == 0);
            }

    // ---- "Interface size": the choices ---------------------------------
    CHECK(nearestInterfaceSize(100) == 100 && nearestInterfaceSize(30) == 100);
    CHECK(nearestInterfaceSize(104) == 100 && nearestInterfaceSize(106) == 110);
    CHECK(nearestInterfaceSize(120) == 125 && nearestInterfaceSize(115) == 110);
    CHECK(nearestInterfaceSize(140) == 150 && nearestInterfaceSize(160) == 150);
    CHECK(nearestInterfaceSize(190) == 200 && nearestInterfaceSize(300) == 200);
    CHECK(steppedInterfaceSize(100, true) == 110 && steppedInterfaceSize(100, false) == 100);
    CHECK(steppedInterfaceSize(115, true) == 125 && steppedInterfaceSize(115, false) == 110);
    CHECK(steppedInterfaceSize(135, true) == 150 && steppedInterfaceSize(135, false) == 125);
    CHECK(steppedInterfaceSize(200, true) == 200 && steppedInterfaceSize(200, false) == 175);

    // ---- a rectangle put inside a screen ------------------------------
    const QRect main(0, 0, 1920, 1032);
    // inside: untouched
    CHECK(fitted({100, 100, 800, 600}, main) == QRect(100, 100, 800, 600));
    // past the right and bottom edges: moved, the size kept
    CHECK(fitted({1500, 800, 800, 600}, main) == QRect(1120, 432, 800, 600));
    // past the left and top edges
    CHECK(fitted({-300, -50, 800, 600}, main) == QRect(0, 0, 800, 600));
    // larger than the screen: shrunk to it
    CHECK(fitted({-100, -100, 3000, 2000}, main) == main);
    // taller only
    CHECK(fitted({200, 0, 600, 1500}, main) == QRect(200, 0, 600, 1032));
    // with a frame (a title bar of 31, borders of 8): the frame is inside too
    const QMargins frame(8, 31, 8, 8);
    const QRect withFrame = fitted({1500, 900, 800, 600}, main, frame);
    CHECK(withFrame.size() == QSize(800, 600));
    CHECK(inside(withFrame.marginsAdded(frame), main));
    CHECK(fitted({0, 0, 800, 600}, main, frame) == QRect(8, 31, 800, 600));
    const QRect big = fitted({0, 0, 4000, 4000}, main, frame);
    CHECK(big.marginsAdded(frame) == main);
    // a screen at the left of the main one, and one above it (negative
    // coordinates), with a taskbar: the window lands on that screen
    const QRect left(-2560, 0, 2560, 1392), above(0, -1080, 1920, 1040);
    CHECK(fitted({-2700, 200, 900, 700}, left) == QRect(-2560, 200, 900, 700));
    CHECK(fitted({-500, 900, 900, 700}, left) == QRect(-900, 692, 900, 700));
    CHECK(inside(fitted({100, -1300, 900, 700}, above), above));
    CHECK(fitted({100, -300, 900, 700}, above) == QRect(100, -740, 900, 700));
    CHECK(inside(fitted({5000, 5000, 4000, 3000}, left), left));
    // every result is inside, whatever comes in
    for (int x = -4000; x <= 4000; x += 700)
        for (int y = -3000; y <= 3000; y += 650)
            for (const QSize &size : { QSize(320, 360), QSize(1280, 720), QSize(3000, 800), QSize(500, 2500) })
                for (const QRect &area : { main, left, above })
                {
                    const QRect result = fitted(QRect(QPoint(x, y), size), area, frame);
                    CHECK(inside(result.marginsAdded(frame), area));
                    CHECK(result.width() <= size.width() && result.height() <= size.height());
                }
    // nothing to fit in, or nothing to fit: unchanged
    CHECK(fitted({10, 10, 100, 100}, QRect()) == QRect(10, 10, 100, 100));
    CHECK(fitted(QRect(), main) == QRect());

    // ---- a saved geometry ---------------------------------------------
    CHECK(reachable({100, 100, 1280, 720}, main));
    // hanging over the right or the bottom edge, its title bar on the screen
    CHECK(reachable({1500, 600, 1280, 720}, main));
    CHECK(reachable({-600, 300, 1280, 720}, main));
    // only a sliver of the title bar left
    CHECK(!reachable({1900, 300, 1280, 720}, main));
    CHECK(!reachable({-1250, 300, 1280, 720}, main));
    // the title bar above the screen, or under it
    CHECK(!reachable({100, -200, 1280, 720}, main));
    CHECK(!reachable({100, 1020, 1280, 720}, main));
    // (the borders of a window that was maximized: 8 pixels out)
    CHECK(reachable({100, -8, 1280, 720}, main));
    // saved on a monitor that is gone (at the left), or on a larger one
    CHECK(!reachable({-2000, 200, 1280, 720}, main));
    CHECK(!reachable({0, 0, 2560, 1392}, main));
    CHECK(reachable({-2000, 200, 1280, 720}, left));
    CHECK(!reachable(QRect(), main) && !reachable({0, 0, 100, 100}, QRect()));

    if (failures)
        return std::fprintf(stderr, "screen fit: %d of %d checks failed\n", failures, checks), 1;
    std::printf("screen fit: %d checks, 0 failed\n", checks);
    return 0;
}

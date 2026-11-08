/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * Live TV: downloads that stall (modules/misc/iptv/idle_watch.hpp). VLC's
 * network reads wait without bound: a server that sends its headers and a
 * few bytes, then nothing, kept a download (and its thread) forever. Here a
 * local server does just that, and a reader that waits as VLC's reads do
 * (vlc_tls_Read: a poll woken by an interruption, a pipe as vlc_poll_i11e's,
 * then a read again, until data comes or the context is killed) is stopped
 * by the watch killing it after its idle time, measured. A wake alone (a
 * raise) does not stop such a reader: the watch's first version raised, and
 * on Windows the stalled download stayed open. One whose server goes on
 * slowly is not stopped; a transfer ended is never touched afterwards. On
 * Linux, from the root of the tree:
 *   g++ -std=c++17 -O1 -Wall -Wextra -pthread -I. extras/juxtopposed/iptv-idle.cpp -o iptv-idle && ./iptv-idle
 */
#include "modules/misc/iptv/idle_watch.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

static int checks = 0;
static int failures = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

using Clock = std::chrono::steady_clock;
using std::chrono::milliseconds;

namespace {

double secondsSince(Clock::time_point t)
{
    return std::chrono::duration<double>(Clock::now() - t).count();
}

// A server of one connection: its headers and the first bytes, then a byte
// every `every` (0: nothing more, a stall) until `for_` passed, then nothing;
// it keeps the connection open until stopped
class StallingServer
{
public:
    StallingServer(milliseconds every, milliseconds for_) : m_every(every), m_for(for_)
    {
        m_listen = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in a {};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.sin_port = 0;
        bind(m_listen, reinterpret_cast<sockaddr*>(&a), sizeof a);
        listen(m_listen, 1);
        socklen_t length = sizeof a;
        getsockname(m_listen, reinterpret_cast<sockaddr*>(&a), &length);
        m_port = ntohs(a.sin_port);
        m_thread = std::thread([this]() { serve(); });
    }
    ~StallingServer()
    {
        m_stop = true;
        shutdown(m_listen, SHUT_RDWR);
        close(m_listen);
        m_thread.join();
    }
    int port() const { return m_port; }
    // when the last byte was sent
    Clock::time_point lastSent() const { return Clock::time_point(Clock::duration(m_lastSent.load())); }

private:
    void serve()
    {
        const int c = accept(m_listen, nullptr, nullptr);
        if (c < 0)
            return;
        const std::string head = "HTTP/1.1 200 OK\r\nContent-Length: 100000\r\n\r\n#EXTM3U\n#EXTINF:-1,A\n";
        send(c, head.data(), head.size(), MSG_NOSIGNAL);
        m_lastSent = Clock::now().time_since_epoch().count();
        const auto start = Clock::now();
        while (!m_stop)
        {
            std::this_thread::sleep_for(m_every.count() > 0 ? m_every : milliseconds(20));
            if (m_every.count() > 0 && Clock::now() - start < m_for)
            {
                send(c, "x", 1, MSG_NOSIGNAL);
                m_lastSent = Clock::now().time_since_epoch().count();
            }
        }
        close(c);
    }

    milliseconds m_every, m_for;
    int m_listen = -1;
    int m_port = 0;
    std::atomic<bool> m_stop { false };
    std::atomic<Clock::rep> m_lastSent { 0 };
    std::thread m_thread;
};

// A reading as VLC's (vlc_tls_Read): a read of the socket, and when nothing
// is there a poll of it and of a pipe its interruption writes to; woken, it
// reads again; it stops at the end of the data or once killed (vlc_killed())
struct Reader
{
    int wake[2] = { -1, -1 };
    std::atomic<int> raised { 0 };
    std::atomic<bool> killed { false };
    std::atomic<bool> reading { false };

    Reader() { (void)!pipe(wake); }
    ~Reader()
    {
        close(wake[0]);
        close(wake[1]);
    }
    // an interruption: wakes the wait (vlc_interrupt_raise)
    void raise()
    {
        ++raised;
        (void)!write(wake[1], "!", 1);
    }
    // and stops the reading (vlc_interrupt_kill)
    void kill()
    {
        killed = true;
        raise();
    }

    std::string read(int port, iptv::IdleWatch::Transfer& transfer, size_t* bytes)
    {
        const int s = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in a {};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.sin_port = htons(uint16_t(port));
        if (connect(s, reinterpret_cast<sockaddr*>(&a), sizeof a) != 0)
            return "cannot be reached";
        fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK);
        reading = true;
        *bytes = 0;
        std::string why = "ended";
        char buffer[4096];
        for (;;)
        {
            if (killed)
            {
                why = transfer.expired() ? "no answer" : "killed";
                break;
            }
            const ssize_t got = recv(s, buffer, sizeof buffer, 0);
            if (got > 0)
            {
                *bytes += size_t(got);
                transfer.received();
                continue;
            }
            if (got == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
                break;
            pollfd fds[2] = { { s, POLLIN, 0 }, { wake[0], POLLIN, 0 } };
            poll(fds, 2, -1);
            if (fds[1].revents & POLLIN)
            {
                char c;
                (void)!::read(wake[0], &c, 1);
            }
        }
        reading = false;
        close(s);
        return why;
    }
};

}

int main()
{
    iptv::IdleWatch watch(milliseconds(50));

    // headers and a few bytes, then a stall: stopped about 1 s after the
    // last byte, its idle time
    {
        StallingServer server(milliseconds(0), milliseconds(0));
        Reader reader;
        size_t bytes = 0;
        std::string why;
        double sinceLast = 0;
        {
            iptv::IdleWatch::Transfer transfer(watch, milliseconds(1000), [&reader]() { reader.kill(); });
            why = reader.read(server.port(), transfer, &bytes);
            sinceLast = secondsSince(server.lastSent());
        }
        std::printf("a stall after %zu bytes: \"%s\" %.2f s after the last byte\n", bytes, why.c_str(), sinceLast);
        CHECK(why == "no answer");
        CHECK(bytes > 0);
        CHECK(sinceLast >= 0.95 && sinceLast < 1.6);
    }
    // a server that sends a byte every 200 ms for 2 s, then stalls: not
    // stopped while it sends (a 500 ms idle time), stopped after
    {
        StallingServer server(milliseconds(200), milliseconds(2000));
        Reader reader;
        size_t bytes = 0;
        std::string why;
        const auto start = Clock::now();
        double sinceLast = 0;
        {
            iptv::IdleWatch::Transfer transfer(watch, milliseconds(500), [&reader]() { reader.kill(); });
            why = reader.read(server.port(), transfer, &bytes);
            sinceLast = secondsSince(server.lastSent());
        }
        const double took = secondsSince(start);
        std::printf("a slow server for 2 s: \"%s\" after %.2f s, %.2f s after its last byte\n", why.c_str(), took,
                    sinceLast);
        CHECK(why == "no answer");
        // (its bytes until about 1.8-2.0 s, then the 0.5 s of silence)
        CHECK(took >= 2.1);
        CHECK(sinceLast >= 0.45 && sinceLast < 1.0);
    }
    // a wake alone does not stop a reading as VLC's (it reads, then waits
    // again): the watch calls its function again and again, and only a kill
    // ends it (the watch that raised left a stalled download open on Windows)
    {
        StallingServer server(milliseconds(0), milliseconds(0));
        Reader reader;
        size_t bytes = 0;
        std::string why;
        std::atomic<bool> done { false };
        {
            iptv::IdleWatch::Transfer transfer(watch, milliseconds(300), [&reader]() { reader.raise(); });
            std::thread t([&]() {
                why = reader.read(server.port(), transfer, &bytes);
                done = true;
            });
            std::this_thread::sleep_for(milliseconds(1500));
            CHECK(!done && reader.reading);
            CHECK(reader.raised >= 3);
            CHECK(transfer.expired());
            reader.kill();
            t.join();
        }
        CHECK(why == "no answer");
    }
    // ended before its idle time: never touched, then or afterwards
    {
        Reader reader;
        {
            iptv::IdleWatch::Transfer transfer(watch, milliseconds(300), [&reader]() { reader.kill(); });
            std::this_thread::sleep_for(milliseconds(100));
            transfer.received();
            std::this_thread::sleep_for(milliseconds(150));
        }
        std::this_thread::sleep_for(milliseconds(800));
        CHECK(reader.raised == 0 && !reader.killed);
    }
    // many at once, each by its own time; one ended while others are killed
    {
        Reader quick, slow, done;
        {
            iptv::IdleWatch::Transfer a(watch, milliseconds(200), [&quick]() { quick.kill(); });
            iptv::IdleWatch::Transfer b(watch, milliseconds(2000), [&slow]() { slow.kill(); });
            {
                iptv::IdleWatch::Transfer c(watch, milliseconds(200), [&done]() { done.kill(); });
                std::this_thread::sleep_for(milliseconds(100));
            }
            std::this_thread::sleep_for(milliseconds(600));
            CHECK(quick.killed && a.expired());
            CHECK(!slow.killed && !b.expired());
        }
        const int after = quick.raised.load();
        std::this_thread::sleep_for(milliseconds(300));
        CHECK(quick.raised == after);
        CHECK(!done.killed && done.raised == 0);
    }
    std::printf("TV idle downloads: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}

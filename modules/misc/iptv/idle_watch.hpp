/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

// Transfers watched for silence. VLC's network reads wait without bound: a
// server that sends its headers and a few bytes, then nothing, kept a
// download (and the thread it runs on) forever. A transfer that receives
// nothing for its idle time is woken by its own function (raise: an
// interruption of the thread's read), and again every interval until it
// ends. Nothing of Qt: the thread is the plug-in's.
namespace iptv {

class IdleWatch
{
public:
    using Clock = std::chrono::steady_clock;

    explicit IdleWatch(std::chrono::milliseconds interval = std::chrono::milliseconds(250))
        : m_interval(interval)
    {
    }

    ~IdleWatch()
    {
        {
            std::lock_guard<std::mutex> guard(m_lock);
            m_stop = true;
        }
        m_wake.notify_all();
        if (m_thread.joinable())
            m_thread.join();
    }

    IdleWatch(const IdleWatch&) = delete;
    IdleWatch& operator=(const IdleWatch&) = delete;

    // One transfer, watched while it lives: raise is called (on the watch's
    // thread) when nothing came for idle, never after its destructor returned
    class Transfer
    {
    public:
        Transfer(IdleWatch& watch, std::chrono::milliseconds idle, std::function<void()> raise)
            : m_watch(watch), m_idle(idle), m_raise(std::move(raise)), m_last(nowMs())
        {
            m_watch.add(this);
        }
        ~Transfer() { m_watch.remove(this); }
        Transfer(const Transfer&) = delete;
        Transfer& operator=(const Transfer&) = delete;

        // bytes came: the idle time starts again
        void received() { m_last.store(nowMs()); }
        // given up for its silence (raised at least once)
        bool expired() const { return m_expired.load(); }

    private:
        friend class IdleWatch;
        static long long nowMs()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();
        }
        IdleWatch& m_watch;
        const std::chrono::milliseconds m_idle;
        const std::function<void()> m_raise;
        std::atomic<long long> m_last;
        std::atomic<bool> m_expired { false };
    };

private:
    void add(Transfer* t)
    {
        std::lock_guard<std::mutex> guard(m_lock);
        m_transfers.push_back(t);
        // (its thread made at the first transfer: none while there is none)
        if (!m_thread.joinable())
            m_thread = std::thread([this]() { run(); });
        m_wake.notify_all();
    }

    void remove(Transfer* t)
    {
        // (under the lock the raise is called with: once removed, never)
        std::lock_guard<std::mutex> guard(m_lock);
        m_transfers.erase(std::remove(m_transfers.begin(), m_transfers.end(), t), m_transfers.end());
    }

    void run()
    {
        std::unique_lock<std::mutex> guard(m_lock);
        while (!m_stop)
        {
            if (m_transfers.empty())
                m_wake.wait(guard);
            else
                m_wake.wait_for(guard, m_interval);
            if (m_stop)
                break;
            const long long now = Transfer::nowMs();
            for (Transfer* t : m_transfers)
                if (now - t->m_last.load() >= t->m_idle.count())
                {
                    t->m_expired.store(true);
                    if (t->m_raise)
                        t->m_raise();
                }
        }
    }

    const std::chrono::milliseconds m_interval;
    std::mutex m_lock;
    std::condition_variable m_wake;
    std::vector<Transfer*> m_transfers;
    bool m_stop = false;
    std::thread m_thread;
};

}

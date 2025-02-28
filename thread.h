/*
author          Oliver Blaser
date            28.02.2025
copyright       MIT - Copyright (c) 2025 Oliver Blaser
*/

#ifndef IG_CURLTHREAD_THREAD_H
#define IG_CURLTHREAD_THREAD_H

#include <mutex>


namespace thread {

class SharedData
{
public:
    using lock_guard = std::lock_guard<std::mutex>;

public:
    SharedData() {}
    virtual ~SharedData() {}

protected:
    mutable std::mutex m_mtx;
};

class ThreadCtl : public SharedData
{
public:
    ThreadCtl()
        : m_booted(false), m_shutdown(false), m_terminate(false)
    {}

    virtual ~ThreadCtl() {}


    // clang-format off
    bool booted() const { lock_guard lg(m_mtxThreadCtl); return m_booted; }
    void shutdown() { lock_guard lg(m_mtxThreadCtl); m_shutdown = true; }
    void terminate() { lock_guard lg(m_mtxThreadCtl); m_terminate = true; }
    // clang-format on


private:
    bool m_booted;
    bool m_shutdown;
    bool m_terminate;
    mutable std::mutex m_mtxThreadCtl;

public:
    // thread intern

    // clang-format off
    void setBooted(bool state) { lock_guard lg(m_mtxThreadCtl); m_booted = state; }
    bool doShutdown() const { lock_guard lg(m_mtxThreadCtl); return m_shutdown; }
    bool doTerminate() const { lock_guard lg(m_mtxThreadCtl); return m_terminate; }
    // clang-format on
};

} // namespace thread


#endif // IG_CURLTHREAD_THREAD_H

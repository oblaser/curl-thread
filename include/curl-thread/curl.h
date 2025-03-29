/*
author          Oliver Blaser
date            01.03.2025
copyright       MIT - Copyright (c) 2025 Oliver Blaser
*/

#ifndef IG_CURLTHREAD_CURL_H
#define IG_CURLTHREAD_CURL_H

#include <cstdint>
#include <queue>
#include <string>
#include <vector>

#include "../curl-thread/thread.h"
#include "../curl-thread/types.h"


#define CURLTHREAD_VERSION_MAJ (3)
#define CURLTHREAD_VERSION_MIN (0)
#define CURLTHREAD_VERSION_PAT (0)


/**
 * @brief Thread safe wrapper for the `curl_easy_..()` interface.
 *
 * \section curl_timeouts Timeouts
 * `CURLOPT_TIMEOUT` is the total (connection + data transfer) timeout in seconds. If `CURLOPT_CONNECTTIMEOUT` = `CURLOPT_TIMEOUT` there might be no or not
 * enough time left for the data transfer.
 * - [`CURLOPT_TIMEOUT`](https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html)
 * - [`CURLOPT_CONNECTTIMEOUT`](https://curl.se/libcurl/c/CURLOPT_CONNECTTIMEOUT.html)
 */
namespace curl {

class ThreadSharedData : public thread::ThreadCtl
{
public:
    class QueueItem
    {
    public:
        QueueItem() = delete;

        explicit QueueItem(const QueueId& queueId)
            : m_queueId(queueId)
        {}

        virtual ~QueueItem() {}

        virtual void clear() { m_queueId = QueueId::NONE; }

        const QueueId& queueId() const { return m_queueId; }

    private:
        QueueId m_queueId;
    };

    class Request : public curl::Request,
                    public ThreadSharedData::QueueItem
    {
    public:
        Request()
            : curl::Request(Method::GET, ""), ThreadSharedData::QueueItem(QueueId::NONE)
        {}

        Request(const curl::Request& other, const QueueId& queueId)
            : curl::Request(other), ThreadSharedData::QueueItem(queueId)
        {}

        virtual ~Request() {}
    };

    class Response : public curl::Response,
                     public ThreadSharedData::QueueItem
    {
    public:
        Response()
            : curl::Response(), ThreadSharedData::QueueItem(QueueId::NONE)
        {}

        Response(const curl::Response& other, const QueueId& queueId)
            : curl::Response(other), ThreadSharedData::QueueItem(queueId)
        {}

        virtual ~Response() {}

        virtual void clear()
        {
            ThreadSharedData::QueueItem::clear();
            curl::Response::m_clear();
        }
    };



public:
    ThreadSharedData()
        : thread::ThreadCtl()
    {}

    virtual ~ThreadSharedData() {}


    // clang-format off
    curl::QueueId queueRequest(const curl::Request& req, const curl::Priority& priority);
    bool responseReady(const curl::QueueId& queueId) const { lock_guard lg(m_mtx); return (queueId == m_response.queueId()); }
    curl::Response popResponse();

    size_t getQNormalSize() const { lock_guard lg(m_mtx); return m_qNormal.size(); }
    size_t getQHighSize() const { lock_guard lg(m_mtx); return m_qHigh.size(); }
    size_t getQMaxSize() const { lock_guard lg(m_mtx); return m_qMax.size(); }
    // clang-format on


private:
    std::queue<ThreadSharedData::Request> m_qNormal;
    std::queue<ThreadSharedData::Request> m_qHigh;
    std::queue<ThreadSharedData::Request> m_qMax;
    std::vector<curl::QueueId::id_type> m_queueId;

    ThreadSharedData::Response m_response;

    void m_rmQueueId(curl::QueueId::id_type id);
    curl::QueueId m_getNewQueueId();


public:
    // thread intern

    // clang-format off
    ThreadSharedData::Request popRequest();
    void setResponse(const curl::Response& res, const QueueId& queueId) { lock_guard lg(m_mtx); m_response = Response(res, queueId); }
    QueueId getResponseQueueId() const { lock_guard lg(m_mtx); return m_response.queueId(); }
    // clang-format on
};



extern ThreadSharedData sharedData;

void thread();

static inline bool booted() { return sharedData.booted(); }
static inline void shutdown() { sharedData.shutdown(); }

static inline curl::QueueId queueRequest(const curl::Request& req, const curl::Priority& priority) { return sharedData.queueRequest(req, priority); }
static inline bool responseReady(const curl::QueueId& queueId) { return sharedData.responseReady(queueId); }
static inline curl::Response popResponse() { return sharedData.popResponse(); }



//! \name Utility
/// @{

std::string toString(const Method& method);
std::string toString(const Priority& priority);

/**
 * @brief Get a random integer in range [min, max].
 *
 * Can be used for e.g. randomised intervals.
 *
 * The result is undefined if `min` > `max` or if any of the arguments is negative.
 */
int random(int min, int max);

/// @}

} // namespace curl


#endif // IG_CURLTHREAD_CURL_H

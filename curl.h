/*
author          Oliver Blaser
date            28.02.2025
copyright       MIT - Copyright (c) 2025 Oliver Blaser
*/

#ifndef IG_CURLTHREAD_CURL_H
#define IG_CURLTHREAD_CURL_H

#include <string>

#include "thread.h"


#define CURLTHREAD_VERSION_MAJ (3)
#define CURLTHREAD_VERSION_MIN (0)
#define CURLTHREAD_VERSION_PAT (0)


namespace curl {

enum class Method
{
    GET = 0,
    POST,
};

class HeaderField
{
public:
    HeaderField() = delete;

    HeaderField(const std::string& key, const std::string& value)
        : m_data(key + ": " + value)
    {}

    virtual ~HeaderField() {}

    bool empty() const { return m_data.empty(); }

    std::string key() const;
    std::string value() const;

    const char* toCurlStr() const { return m_data.c_str(); }

private:
    std::string m_data;
};

} // namespace curl



/*

new nice stuff
########################################################################################################################
old stuff from v2.x

*/



#include <cstddef>
#include <cstdint>
#include <ctime>
#include <queue>
#include <string>
#include <vector>


namespace curl {

/**
 * For info about the timeouts see [`CURLOPT_TIMEOUT`](https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html) and
 * [`CURLOPT_CONNECTTIMEOUT`](https://curl.se/libcurl/c/CURLOPT_CONNECTTIMEOUT.html).
 * `CURLOPT_TIMEOUT` is the total timeout in seconds. If `CURLOPT_CONNECTTIMEOUT` = `CURLOPT_TIMEOUT` there might be no time left for the data transfer.
 */
class Request
{
public:
    static const char* const defaultUserAgent;

public:
    Request()
        : m_method(Method::GET),
          m_url(),
          m_timeoutConn(0),
          m_timeout(0),
          m_userAgent(defaultUserAgent),
          m_hasBody(false),
          m_body(),
          m_header(),
          m_queueId(-1668641388)
    {}

    // GET request
    Request(const std::string& url, long timeoutConn = 0, long timeout = 0, const std::string& userAgent = defaultUserAgent)
        : m_method(Method::GET),
          m_url(url),
          m_timeoutConn(timeoutConn),
          m_timeout(timeout),
          m_userAgent(userAgent),
          m_hasBody(false),
          m_body(),
          m_header(),
          m_queueId(-1668641388)
    {}

    Request(Method method, const std::string& url, long timeoutConn = 0, long timeout = 0, const std::string& userAgent = defaultUserAgent)
        : m_method(method),
          m_url(url),
          m_timeoutConn(timeoutConn),
          m_timeout(timeout),
          m_userAgent(userAgent),
          m_hasBody(false),
          m_body(),
          m_header(),
          m_queueId(-1668641388)
    {}

    virtual ~Request() {}

    Method method() const { return m_method; }
    const std::string& url() const { return m_url; }
    long timeoutConn() const { return m_timeoutConn; }
    long timeout() const { return m_timeout; }
    const std::string& userAgent() const { return m_userAgent; }
    bool hasBody() const { return m_hasBody; }
    const std::string& body() const { return m_body; }
    bool hasHeader() const { return !m_header.empty(); }
    const std::vector<HeaderField>& header() const { return m_header; }

    void setBody(const std::string& body)
    {
        m_hasBody = true;
        m_body = body;
    }

    void setHeader(const std::vector<HeaderField>& header) { m_header = header; }
    void addHeader(const HeaderField& headerField) { m_header.push_back(headerField); }

    int queueId() const { return m_queueId; }
    void setQueueId(int id) { m_queueId = id; }

    std::string toString(bool sgrEnable = false) const;

private:
    Method m_method;
    std::string m_url;
    long m_timeoutConn;
    long m_timeout;
    std::string m_userAgent;
    bool m_hasBody;
    std::string m_body;
    std::vector<HeaderField> m_header;

    int m_queueId;
};

std::string toString(const Request::Method& method);

class Response
{
public:
    Response()
        : m_curlCode(-1), m_httpCode(-1), m_body()
    {}

    Response(int curlCode, int httpCode, const std::string& body);

    virtual ~Response() {}

    int curlCode() const { return m_curlCode; }
    int httpCode() const { return m_httpCode; }
    const std::string& body() const { return m_body; }

    bool aborted() const;
    bool curlOk() const; // curlCode == CURLE_OK (0)
    bool httpOk() const; // httpCode == 200
    bool good() const { return (curlOk() && httpOk()); }

    std::string toString() const;
    std::string toString_noBody() const;

private:
    int m_curlCode;
    int m_httpCode;
    std::string m_body;
};

class ThreadSharedData : public thread::SharedData
{
public:
    ThreadSharedData()
        : m_shutdown(false), m_queue(), m_queuePrio(), m_queuePrioMax(), m_queueId(), m_response(), m_responseId(-1)
    {}
    virtual ~ThreadSharedData() {}

    // clang-format off
    void shutdown(bool state = true) { lock_guard lg(m_mtx); m_shutdown = state; }
    bool testShutdown() const { lock_guard lg(m_mtx); return m_shutdown; }
    // clang-format on



    // extern
public:
    int queueReq(const curl::Request& req, bool priority = false); // returns the queue id of the request
    int queueReqMaxPrio(const curl::Request& req);                 // returns the queue id of the request
    int getResId() const
    {
        lock_guard lg(m_mtx);
        return m_responseId;
    }

    curl::Response getRes();

    bool isQueuePrioMaxEmpty() const
    {
        lock_guard lg(m_mtx);
        return (m_queuePrioMax.empty());
    }



    // intern
public:
    bool isQueueEmpty() const
    {
        lock_guard lg(m_mtx);
        return (m_queue.empty() && m_queuePrio.empty() && m_queuePrioMax.empty());
    }

    curl::Request popReq();

    void setRes(const curl::Response& res, int id)
    {
        lock_guard lg(m_mtx);
        m_response = res;
        m_responseId = id;
    }

private:
    bool m_shutdown;

    std::queue<curl::Request> m_queue;
    std::queue<curl::Request> m_queuePrio;
    std::queue<curl::Request> m_queuePrioMax;
    std::vector<int> m_queueId;

    curl::Response m_response;
    int m_responseId;

    void m_rmQueueId(int id);
    int m_getNewQueueId();
};

extern ThreadSharedData sd;

void thread();

/**
 * @brief Get a random interval in range [min, max].
 */
time_t random(time_t min, time_t max);

} // namespace curl


#endif // IG_CURLTHREAD_CURL_H

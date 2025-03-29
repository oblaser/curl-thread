/*
author          Oliver Blaser
date            01.03.2025
copyright       MIT - Copyright (c) 2025 Oliver Blaser
*/

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>

#include "../include/curl-thread/curl.h"
#include "../include/curl-thread/thread.h"
#include "../include/curl-thread/types.h"

// VS Properties:
//      - C/C++ > General > Additional Include Dirs: `$(sdk)/curl/x86/include`
//      - Linker > Input > Additional Dependencies: `Normaliz.lib;Ws2_32.lib;Wldap32.lib;Crypt32.lib;advapi32.lib;$(sdk)/curl/x86/lib/libcurl_a.lib;`
#define CURL_NO_OLDIES
#define CURL_STATICLIB
#include <curl/curl.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif


#if defined(_MSC_VER)
#define CPPSTD (_MSVC_LANG)
#else
#define CPPSTD (__cplusplus)
#endif
#define CPPSTD_98 (199711L)
#define CPPSTD_11 (201103L)
#define CPPSTD_14 (201402L)
#define CPPSTD_17 (201703L)
#define CPPSTD_20 (202002L)

#if (CPPSTD > CPPSTD_11)
#define CONSTEXPR constexpr
#else
#define CONSTEXPR const
#endif


#ifdef CURLTHREAD_CONFIG_DEBUG_print_queueId_vector

#define DEBUG_print_queueId_vector_before()                                                                                                    \
    auto print_queueId_vector = [&]() {                                                                                                        \
        std::string str = "    m_queueId: [";                                                                                                  \
        for (size_t i = 0; i < m_queueId.size(); ++i) { str += " " + std::to_string(m_queueId[i]); }                                           \
        if (m_queueId.size() > 0) { str += " "; }                                                                                              \
        str += "]";                                                                                                                            \
        return str;                                                                                                                            \
    };                                                                                                                                         \
    const bool print_queueId_vector_enable = (m_queueId.size() > 1);                                                                           \
    if (print_queueId_vector_enable)                                                                                                           \
    {                                                                                                                                          \
        std::string queueId_vector_str = "\033[90m";                                                                                           \
        queueId_vector_str += "CURLTHREAD " + std::string(__func__) + ":" + std::to_string(__LINE__) + "    id: " + std::to_string(id) + "\n"; \
        queueId_vector_str += print_queueId_vector();                                                                                          \
        queueId_vector_str += "\033[39m";                                                                                                      \
        printf("%s\n", queueId_vector_str.c_str());                                                                                            \
    }                                                                                                                                          \
    // LOG_DBG();

#define DEBUG_print_queueId_vector_after()            \
    if (print_queueId_vector_enable)                  \
    {                                                 \
        std::string queueId_vector_str = "\033[90m";  \
        queueId_vector_str += print_queueId_vector(); \
        queueId_vector_str += "\033[39m";             \
        printf("%s\n", queueId_vector_str.c_str());   \
    }                                                 \
    // LOG_DBG();

#else // CURLTHREAD_CONFIG_DEBUG_print_queueId_vector

#define DEBUG_print_queueId_vector_before() (void)0
#define DEBUG_print_queueId_vector_after()  (void)0

#endif // CURLTHREAD_CONFIG_DEBUG_print_queueId_vector


namespace curl {
namespace util {

    CONSTEXPR size_t strlen(const char* str)
    {
        size_t len = 0;
        while (*(str + len) != 0) { ++len; }
        return len;
    }

    /**
     * Errors like `EINTR` are not handled.
     *
     * Values out of the specified range are clamped. On Windows the value is clamped to a minimum of 1ms, and rounded to a
     * multiple of 1ms in such a way that speed is more important than accurate or binary rounding.
     *
     * @param t_us Time to sleep as microseconds, must be in the range [0, 999999]
     */
    void sleep(int t_us)
    {
        if (t_us > 999999) { t_us = 999999; }

#ifdef _WIN32

        if (t_us < 1000) { t_us = 1000; }

        // round at 3/4 (normal rounding would be done at 1/2), this could potentially lead to a value out of the specified
        // range, but this doesn't matter on Windows
        t_us += 250;

        const DWORD t_ms = (DWORD)(t_us / 1000);
        Sleep(t_ms);

#else

        if (t_us < 0) { t_us = 0; }

        struct timespec dur;
        dur.tv_sec = 0;
        dur.tv_nsec = t_us * 1000;
        nanosleep(&dur, nullptr);

#endif
    }

} // namespace util
} // namespace curl

namespace {

enum STATE
{
    S_init = 0,
    S_boot,
    S_shutdown,
    S_halted,

    S_idle,
    S_request,
    S_awaitResponsePop,

    S__end_
};

} // namespace



static curl::Response perform(CURL* curl, const curl::ThreadSharedData::Request& request);
static size_t transfer_write(char* p, size_t size, size_t nmemb, void* pClientData);



curl::ThreadSharedData curl::sharedData; // implicitly calls the default constructor

void curl::thread()
{
    int state = S_init;
    int threadSleep_us = 500;
    curl::ThreadSharedData::Request request;

    while (!sharedData.doTerminate())
    {
        switch (state)
        {
        case S_init:
            // no need to init `request` or `ThreadSharedData::m_response`, default contructors set an invalid queue ID
            state = S_boot;
            break;

        case S_boot:
        {
            CURLcode curl_res = curl_global_init(CURL_GLOBAL_DEFAULT);

            if (curl_res == CURLE_OK)
            {
                sharedData.setBooted(true);
                state = S_idle;
            }
            else
            {
                // LOG_ERR("curl_global_init() returned %i", (int)curl_res);
                state = S_halted;
            }
        }
        break;

        case S_shutdown:
            if (sharedData.booted()) { curl_global_cleanup(); }
            sharedData.setBooted(false);
            state = S_halted;
            break;

        case S_halted:
            sharedData.terminate();
            break;



        case S_idle:

            request = sharedData.popRequest();

            if (request.queueId().isValid())
            {
                state = S_request;
                threadSleep_us = 200;
            }
            else { threadSleep_us = 50 * 1000; }

            if (sharedData.doShutdown()) { state = S_shutdown; }

            break;

        case S_request:
        {
            curl::Response response = curl::Response(-1, -1, "curl_easy_init() failed");

            CURL* curl = curl_easy_init();
            if (curl)
            {
                response = perform(curl, request);
                curl_easy_cleanup(curl);
            }

            sharedData.setResponse(response, request.queueId());
            state = S_awaitResponsePop;
        }
        break;

        case S_awaitResponsePop:
            if (!sharedData.getResponseQueueId().isValid()) { state = S_idle; }
            break;

        default:
            threadSleep_us = (1000 * 1000);
            break;
        }

        curl::util::sleep(threadSleep_us);

    } // while !terminate

    // LOG_DBG("terminated");
}



curl::Response perform(CURL* curl, const curl::ThreadSharedData::Request& request)
{
    curl_easy_setopt(curl, CURLOPT_URL, request.url().c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, request.userAgent().c_str());

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, request.connectTimeout());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, request.totalTimeout());

    curl_slist* headerList = nullptr;

    if (!request.header().empty())
    {
        for (size_t i = 0; i < request.header().size(); ++i)
        {
            const auto& tmp = request.header()[i];
            if (!tmp.empty())
            {
                // the src string is copied, and thus could be freed after this call
                headerList = curl_slist_append(headerList, tmp.curlStr());
            }
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }

    switch (request.method())
    {
    case curl::Method::GET:
        (void)0; // nop, ignoring body of GET request
        break;

    case curl::Method::POST:
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (!request.body().empty())
        {
            // does not copy the data, the memory pointed to has to stay allocated until the transfer finishes
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body().c_str());
        }
        break;
    }

    std::string resBody;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, transfer_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resBody);

    const CURLcode curlCode = curl_easy_perform(curl);

    long httpCode;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headerList);

    return curl::Response((int)curlCode, (int)httpCode, resBody);
}

size_t transfer_write(char* p, size_t size, size_t nmemb, void* pClientData)
{
    size_t effSize = size * nmemb;
    for (size_t i = 0; i < effSize; ++i) { static_cast<std::string*>(pClientData)->push_back(*(p + i)); }
    return effSize;
}



//======================================================================================================================
// curl.h implementation



#ifdef CURLTHREAD_CONFIG_DEBUG_print_queueId_vector
#include <iostream>
using std::cout;
using std::endl;
#endif



curl::QueueId curl::ThreadSharedData::queueRequest(const curl::Request& req, const curl::Priority& priority)
{
    lock_guard lg(m_mtx);

    curl::QueueId id = m_getNewQueueId();

    DEBUG_print_queueId_vector_before();

    if (id.isValid())
    {
        const ThreadSharedData::Request tmp(req, id);

        try
        {
            m_queueId.push_back(id);

            switch (priority)
            {
            case Priority::normal:
                m_qNormal.push(tmp);
                break;

            case Priority::high:
                m_qHigh.push(tmp);
                break;

            case Priority::max:
                m_qMax.push(tmp);
                break;
            }
        }
        catch (...)
        {
            m_rmQueueId(id);
            id = QueueId::FAILED;
        }
    }

    DEBUG_print_queueId_vector_after();

    return id;
}

curl::Response curl::ThreadSharedData::popResponse()
{
    lock_guard lg(m_mtx);
    m_rmQueueId(m_response.queueId());
    const curl::Response res = m_response;
    m_response.clear();
    return res;
}

/**
 * Removes the `id` from `m_queueId`. If `id` is not found `m_queueId` is unchanged.
 */
void curl::ThreadSharedData::m_rmQueueId(curl::QueueId::id_type id)
{
    DEBUG_print_queueId_vector_before();

    for (size_t i = 0; i < m_queueId.size(); ++i)
    {
        if (m_queueId[i] == id)
        {
            m_queueId.erase(m_queueId.begin() + i);
            --i;
        }
    }

    DEBUG_print_queueId_vector_after();
}

/**
 * Returnes an unused ID in range [`curl::QueueId::BASE`, `curl::QueueId::MAX`] or `curl::QueueId::FAILED`.
 */
curl::QueueId curl::ThreadSharedData::m_getNewQueueId()
{
    static_assert((curl::QueueId::BASE > 0) &&                             // see curl::QueueId::isValid()
                      (curl::QueueId::MAX <= INT16_MAX) &&                 // the latter two ensure that the doc of this
                      (sizeof(curl::QueueId::id_type) >= sizeof(int16_t)), // func is true on all possible platormfms
                  "see comments");


    curl::QueueId::id_type id = curl::QueueId::BASE;
    bool used;

    do {
        used = false;

        for (size_t i = 0; (i < m_queueId.size()) && !used; ++i)
        {
            if (m_queueId[i] == id)
            {
                if (id < curl::QueueId::MAX)
                {
                    used = true;
                    ++id;
                }
                else
                {
                    id = curl::QueueId::FAILED;
                    used = false;
                }
            }
        }
    }
    while (used);

    // if ((id > 100) || (id < 0)) { LOG_WRN("new queue ID: %i", id); }

    return id;
}

curl::ThreadSharedData::Request curl::ThreadSharedData::popRequest()
{
    lock_guard lg(m_mtx);

    curl::ThreadSharedData::Request r;

    if (!m_qMax.empty())
    {
        r = m_qMax.front();
        m_qMax.pop();
    }
    else if (!m_qHigh.empty())
    {
        r = m_qHigh.front();
        m_qHigh.pop();
    }
    else if (!m_qNormal.empty())
    {
        r = m_qNormal.front();
        m_qNormal.pop();
    }
    else { r.clear(); }

    return r;
}



std::string curl::toString(const Method& method)
{
    std::string str;

    switch (method)
    {
    case curl::Method::GET:
        str = "GET";
        break;

    case curl::Method::POST:
        str = "POST";
        break;
    }

    return str;
}

std::string curl::toString(const Priority& priority)
{
    std::string str;

    switch (priority)
    {
    case curl::Priority::normal:
        str = "normal";
        break;

    case curl::Priority::high:
        str = "high";
        break;

    case curl::Priority::max:
        str = "max";
        break;
    }

    return str;
}

int curl::random(int min, int max)
{
    std::random_device rd;
    std::mt19937 gen(rd());

    static_assert(sizeof(int) == sizeof(int32_t), "types might need to be changed here");
    std::uniform_int_distribution<int32_t> distrib(0, INT32_MAX);
    auto rand_i32 = [&gen, &distrib]() { return distrib(gen); };

    const int span = (max - min) + 1;

    return ((span > 0) ? (min + (((int)rand_i32()) % span)) : min);
}



// curl.h implementation
//======================================================================================================================
// types.h implementation



std::string curl::QueueId::toString() const
{
    std::string str;

    switch (m_id)
    {
    case NONE:
        str = "NONE (" + std::to_string(m_id) + ")";
        break;

    case FAILED:
        str = "FAILED (" + std::to_string(m_id) + ")";
        break;

    default:
        str = std::to_string(m_id);
        break;
    }

    return str;
}





#define ___curl_HeaderField_delimiter (": ")
const char* const curl::HeaderField::delimiter = ___curl_HeaderField_delimiter;

std::string curl::HeaderField::key() const
{
    const size_t delimiterPos = m_curlStr.find(delimiter);
    return m_curlStr.substr(0, delimiterPos);
}

std::string curl::HeaderField::value() const
{
    const size_t delimiterPos = m_curlStr.find(delimiter);
    CONSTEXPR size_t delimiterLength = curl::util::strlen(___curl_HeaderField_delimiter);
    return m_curlStr.substr(delimiterPos + delimiterLength);
}



const char* const curl::Request::defaultUserAgent = "libcurl";

std::string curl::Request::toString() const
{
    std::string str = curl::toString(m_method);

    str += " " + m_url;
    str += " " + std::to_string(m_connectTimeout);
    str += " " + std::to_string(m_totalTimeout);
    str += " \"" + m_userAgent + "\"";
    // str += " (" + std::to_string(m_queueId) + ")";

    if (!m_body.empty()) { str += " " + m_body; }

    return str;
}



bool curl::Response::aborted() const { return (m_curlCode == CURLE_ABORTED_BY_CALLBACK); }
bool curl::Response::curlOk() const { return (m_curlCode == CURLE_OK); }

std::string curl::Response::toString() const
{
    std::string str = toString_noBody();

    if (!m_body.empty()) { str += " - " + m_body; }

    return str;
}

std::string curl::Response::toString_noBody() const
{
    std::string str = std::to_string(m_curlCode);

    if (m_curlCode != CURLE_OK) { str += " " + std::string(curl_easy_strerror((CURLcode)m_curlCode)); }

    str += " - " + std::to_string(m_httpCode);

    return str;
}

void curl::Response::m_clear()
{
    m_curlCode = (-1);
    m_httpCode = (-1);
    m_body = std::string();
}

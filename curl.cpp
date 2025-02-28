/*
author          Oliver Blaser
date            28.02.2025
copyright       MIT - Copyright (c) 2025 Oliver Blaser
*/

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <queue>
#include <random>
#include <string>
#include <vector>

#include "curl.h"

#define CURL_NO_OLDIES
#define CURL_STATICLIB
#include <curl/curl.h>


namespace {

enum STATE
{
    S_init = 0,
    S_booting,
    S_shutdown,
    S_halted,

    S_idle,
    S_request,
    S_awaitResponsePop,

    S__end_
};

size_t curlWriteFunction(char* p, size_t size, size_t nmemb, void* pClientData)
{
    size_t effSize = size * nmemb;
    for (size_t i = 0; i < effSize; ++i) { static_cast<std::string*>(pClientData)->push_back(*(p + i)); }
    return effSize;
}

} // namespace



/*

new nice stuff
########################################################################################################################
old stuff from v2.x

*/


#if defined(_DEBUG) && 0
#define CURLTH_print_queueId_vector_DEBUG (1) // def/undef
#endif



std::string curl::HeaderField::key() const
{
    std::string r;

    if (m_data.find(": ") < m_data.length())
    {
#if (OMW_VERSION_ID <= OMW_VERSION_ID_0_2_1_ALPHA)
        r = omw::string(m_data).split(':', 2)[0];
#else
        r = omw::split(m_data, ':', 2)[0];
#endif
    }
    else r = m_data;

    return r;
}

std::string curl::HeaderField::value() const
{
    std::string r;

    if (m_data.find(": ") < m_data.length())
    {
#if OMW_VERSION_ID <= OMW_VERSION_ID_0_2_1_ALPHA
        r = omw::string(m_data).split(':', 2)[1].substr(1);
#else
        r = omw::split(m_data, ':', 2)[1].substr(1);
#endif
    }

    return r;
}

const char* const curl::Request::defaultUserAgent = "libcurl";

std::string curl::Request::toString(bool sgrEnable) const
{
    std::string str = curl::toString(m_method);

    str += " " + m_url;
    str += " " + std::to_string(m_timeoutConn);
    str += " " + std::to_string(m_timeout);
    str += " \"" + m_userAgent + "\"";
    str += " (" + std::to_string(m_queueId) + ")";

    if (m_hasBody)
    {
        if (sgrEnable) str += "\n\033[90m";
        else str += " ";
        str += m_body;
        if (sgrEnable) str += "\033[39m";
    }

    return str;
}



std::string curl::toString(const curl::Request::Method& method)
{
    std::string str;

    switch (method)
    {
    case curl::Request::Method::GET:
        str = "GET";
        break;

    case curl::Request::Method::POST:
        str = "POST";
        break;
    }

    return str;
}



curl::Response::Response(int curlCode, int httpCode, const std::string& body)
    : m_curlCode(curlCode), m_httpCode(httpCode), m_body(body)
{}

bool curl::Response::aborted() const { return (m_curlCode == CURLE_ABORTED_BY_CALLBACK); }

bool curl::Response::curlOk() const { return (m_curlCode == CURLE_OK); }

bool curl::Response::httpOk() const { return (m_httpCode == 200); }

std::string curl::Response::toString() const
{
    std::string str = std::to_string(m_curlCode);
    if (m_curlCode != CURLE_OK) str += " " + std::string(curl_easy_strerror((CURLcode)m_curlCode));
    str += " - " + std::to_string(m_httpCode) + " - " + m_body;
    return str;
}

std::string curl::Response::toString_noBody() const
{
    std::string str = std::to_string(m_curlCode);
    if (m_curlCode != CURLE_OK) str += " " + std::string(curl_easy_strerror((CURLcode)m_curlCode));
    str += " - " + std::to_string(m_httpCode);
    return str;
}



int curl::ThreadSharedData::queueReq(const curl::Request& req, bool priority)
{
    lock_guard lg(m_mtx);

    curl::Request tmp = req;
    const int id = m_getNewQueueId();
    tmp.setQueueId(id);

    m_queueId.push_back(id);

    if (priority) m_queuePrio.push(tmp);
    else m_queue.push(tmp);

    return id;
}

int curl::ThreadSharedData::queueReqMaxPrio(const curl::Request& req)
{
    lock_guard lg(m_mtx);

    curl::Request tmp = req;
    const int id = m_getNewQueueId();
    tmp.setQueueId(id);

    m_queueId.push_back(id);

    m_queuePrioMax.push(tmp);

    return id;
}

curl::Response curl::ThreadSharedData::getRes()
{
    lock_guard lg(m_mtx);
    m_rmQueueId(m_responseId);
    m_responseId = (-1);
    return m_response;
}

curl::Request curl::ThreadSharedData::popReq()
{
    lock_guard lg(m_mtx);

    curl::Request r;

    if (!m_queuePrioMax.empty())
    {
        r = m_queuePrioMax.front();
        m_queuePrioMax.pop();
    }
    else if (!m_queuePrio.empty())
    {
        r = m_queuePrio.front();
        m_queuePrio.pop();
    }
    else if (!m_queue.empty())
    {
        r = m_queue.front();
        m_queue.pop();
    }

#if defined(PRJ_DEBUG) && (defined(CURLTH_print_queueId_vector_DEBUG) || 0)
    LOG_DBG(); // std::cout << OMW__FILENAME__ << "(" << __func__ << "):" << __LINE__ << "\n    " << r.toString() << std::endl;
#endif

    return r;
}

void curl::ThreadSharedData::m_rmQueueId(int id)
{
#ifdef CURLTH_print_queueId_vector_DEBUG
    LOG_DBG();
    if (app::cliargs.containsVerbose())
    {
        std::cout << OMW__FILENAME__ << "(" << __func__ << "):" << __LINE__ << "\n    id: " << id << "\n    ";
        std::cout << "m_queueId: [";
        for (size_t i = 0; i < m_queueId.size(); ++i) { std::cout << " " << m_queueId[i]; }
        if (m_queueId.size() > 0) std::cout << " ";
        std::cout << "]" << std::endl;
    }
#endif

#if 0
    bool proc;

    do
    {
        proc = false;

        for (size_t i = 0; (i < m_queueId.size()) && !proc; ++i)
        {
            if (m_queueId[i] == id)
            {
                proc = true;
                m_queueId.erase(m_queueId.begin() + i);
            }
        }
    }
    while (proc);
#else
    for (size_t i = 0; i < m_queueId.size(); ++i)
    {
        if (m_queueId[i] == id)
        {
            m_queueId.erase(m_queueId.begin() + i);
            --i;
        }
    }
#endif

#ifdef CURLTH_print_queueId_vector_DEBUG
    LOG_DBG();
    if (app::cliargs.containsVerbose())
    {
        std::cout << "    m_queueId: [";
        for (size_t i = 0; i < m_queueId.size(); ++i) { std::cout << " " << m_queueId[i]; }
        if (m_queueId.size() > 0) std::cout << " ";
        std::cout << "]" << std::endl;
    }
#endif
}

int curl::ThreadSharedData::m_getNewQueueId()
{
    int id = 1;
    bool used;

    // this implementation always returns a valid request ID, if this behaviour is changed CURLTHREAD_VERSION_MAJ has to increase

    do {
        used = false;

        for (size_t i = 0; (i < m_queueId.size()) && !used; ++i)
        {
            if (m_queueId[i] == id)
            {
                used = true;
                ++id;
            }
        }
    }
    while (used);

    if ((id > 100) || (id < 0)) { LOG_WRN("new queue ID: %i", id); }

    return id;
}



curl::ThreadSharedData curl::sd = curl::ThreadSharedData();



void curl::thread()
{
    int state = S_init;

    useconds_t threadSleep_us = 200;

    curl::Request request;
    curl::Response response;

    while (!sd.testTerminate())
    {
#if LOG_MODULE_LEVEL >= LOG_LEVEL_DBG
        static int oldState = -1;
        if (oldState != state)
        {
            LOG_DBG("%s state %i -> %i", ___LOG_STR(LOG_MODULE_NAME), oldState, state);

            oldState = state;
        }
#endif

        switch (state)
        {
        case S_init:
            sd.setRes(curl::Response(), (-1));
            state = S_booting;
            break;

        case S_booting:
        {
            CURLcode curl_res = curl_global_init(CURL_GLOBAL_DEFAULT);

            if (curl_res == CURLE_OK)
            {
                sd.setBooted(true);
                state = S_idle;
            }
            else
            {
                LOG_ERR("curl_global_init() returned %i", (int)curl_res);
                state = S_halted;
            }
        }
        break;

        case S_idle:
            threadSleep_us = 1000;

            if (!sd.isQueueEmpty())
            {
                threadSleep_us = 200;
                request = sd.popReq();
                state = S_sendRequest;

#if defined(PRJ_DEBUG) && defined(CURL_PRINT_REQRES)
                LOG_DBG(); // util::print("\033[96mcURL request:\033[39m " + request.toString() + "\n");
#endif
            }

            if (sd.testShutdown()) { state = S_shutdown; }

            break;

        case S_sendRequest:
        {
            response = curl::Response(-1, -1, "curl_easy_init() failed");

            CURL* curl = curl_easy_init();

            if (curl)
            {
                curl_easy_setopt(curl, CURLOPT_URL, request.url().c_str());
                curl_easy_setopt(curl, CURLOPT_USERAGENT, request.userAgent().c_str());

                curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, request.timeoutConn());
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, request.timeout());

                curl_slist* headerList = nullptr;

                if (request.hasHeader())
                {
                    for (size_t i = 0; i < request.header().size(); ++i)
                    {
                        if (!request.header()[i].empty()) headerList = curl_slist_append(headerList, request.header()[i].toCurlStr());
                    }

                    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
                }

                switch (request.method())
                {
                case Request::Method::GET:
                    (void)0; // nop
                    if (request.hasBody()) { LOG_WRN("ignoring body of GET request"); }
                    break;

                case Request::Method::POST:
                    curl_easy_setopt(curl, CURLOPT_POST, 1L);
                    if (request.hasBody()) { curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body().c_str()); }
                    break;
                }

                std::string resBody;
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteFunction);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resBody);

                CURLcode res;
                res = curl_easy_perform(curl);

                long resCode;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resCode);

                response = curl::Response(res, resCode, resBody);

                curl_slist_free_all(headerList);
                curl_easy_cleanup(curl);
            }

            state = S_processRequest;

#if defined(PRJ_DEBUG) && defined(CURL_PRINT_REQRES)
            LOG_DBG(); // util::print("\033[96m \\--cURL response:\033[39m " + response.toString() + "\n");
#endif
        }
        break;

        case S_processRequest:
            sd.setRes(response, request.queueId());
            state = S_awaitResponseTaken;
            break;

        case S_awaitResponseTaken:
            if (sd.getResId() < 0) { state = S_idle; }
            break;

        case S_shutdown:
            LOG_INF("shutdown");
            if (sd.isBooted()) { curl_global_cleanup(); }
            sd.setBooted(false);
            state = S_halted;
            break;

        case S_halted:
            sd.terminate();
            break;

        default:
            threadSleep_us = (1000 * 1000);
            break;
        }

        usleep(threadSleep_us);

    } // while !terminate

    LOG_DBG("exit");
}

time_t curl::getReqInterval(const time_t& min, const time_t& max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int32_t> distrib(0, INT32_MAX);
    auto rand_i32 = [&gen, &distrib]() { return distrib(gen); };

    const time_t span = (max - min) + 1;

    return min + (((time_t)rand_i32()) % span);
}

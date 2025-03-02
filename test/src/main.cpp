/*
author          Oliver Blaser
date            01.03.2025
copyright       MIT - Copyright (c) 2025 Oliver Blaser
*/

#include <ctime>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "../../../curl-thread/curl.h"
#include "../../../curl-thread/thread.h"
#include "middleware/util.h"

#define LOG_MODULE_LEVEL LOG_LEVEL_DBG
#define LOG_MODULE_NAME  MAIN
#include "middleware/log.h"


// clang-format off
#define LOG_BLU(msg, ...) printf(___LOG_CSI_EL "\033[94m" "[%s] -{ BLU }- " msg "\033[39m" "\n", util::t_to_iso8601_local(std::time(nullptr)).c_str() ___LOG_OPT_VA_ARGS(__VA_ARGS__))
#define LOG_CYA(msg, ...) printf(___LOG_CSI_EL "\033[96m" "[%s] -{ CYA }- " msg "\033[39m" "\n", util::t_to_iso8601_local(std::time(nullptr)).c_str() ___LOG_OPT_VA_ARGS(__VA_ARGS__))
#define LOG_MAG(msg, ...) printf(___LOG_CSI_EL "\033[95m" "[%s] -{ MAG }- " msg "\033[39m" "\n", util::t_to_iso8601_local(std::time(nullptr)).c_str() ___LOG_OPT_VA_ARGS(__VA_ARGS__))
#define LOG_YEL(msg, ...) printf(___LOG_CSI_EL "\033[93m" "[%s] -{ YEL }- " msg "\033[39m" "\n", util::t_to_iso8601_local(std::time(nullptr)).c_str() ___LOG_OPT_VA_ARGS(__VA_ARGS__))
// clang-format on


using std::cout;
using std::endl;

namespace {

enum
{
    S_init = 0,
    S_boot,
    S_idle,
    S_req,
    S_awaitRes,
};

constexpr int initial_threadSleep_us = 100;

} // namespace


#define DECLARE_TH_NS(_name)  \
    namespace _name {         \
        std::thread th;       \
        thread::ThreadCtl sd; \
        void fn();            \
    }

DECLARE_TH_NS(blue)
DECLARE_TH_NS(cyan)
DECLARE_TH_NS(magenta)
DECLARE_TH_NS(yellow)


static std::thread thread_curl;


int main(int argc, char** argv)
{
    thread_curl = std::thread(curl::thread);
    blue::th = std::thread(blue::fn);
    cyan::th = std::thread(cyan::fn);
    magenta::th = std::thread(magenta::fn);
    yellow::th = std::thread(yellow::fn);

    const time_t tStart = std::time(nullptr);
    time_t tNow;

    do {
        tNow = std::time(nullptr);

        util::sleep(1000 * 1000);
    }
    while ((tNow - tStart) < (2 * 60));

    {
        int n, h, m;
        n = curl::sharedData.getQNormalSize();
        h = curl::sharedData.getQHighSize();
        m = curl::sharedData.getQMaxSize();
        LOG_INF("shutdown curl thread, queue sizes: %i, %i, %i", n, h, m);

        curl::sharedData.shutdown();
        thread_curl.join();

        n = curl::sharedData.getQNormalSize();
        h = curl::sharedData.getQHighSize();
        m = curl::sharedData.getQMaxSize();
        LOG_INF("joined curl thread, queue sizes: %i, %i, %i", n, h, m);
    }

    blue::sd.terminate();
    cyan::sd.terminate();
    magenta::sd.terminate();

    blue::th.join();
    cyan::th.join();
    magenta::th.join();
    yellow::th.join();

    LOG_INF("exit");

    return 0;
}



#define LOG_TH LOG_BLU
namespace blue {

std::string getName()
{
    static size_t i = 0;
    static const char* const names[] = { "Max",    "Anna",   "Peter",  "Maria",  "John",      "Sophie",    "Paul",      "Lena",     "David",
                                         "Julia",  "Tom",    "Laura",  "Stefan", "Nina",      "Christian", "Eva",       "Michael",  "Klara",
                                         "Daniel", "Monika", "Felix",  "Sandra", "Sebastian", "Emma",      "Oliver",    "Carla",    "Jakob",
                                         "Mia",    "Leon",   "Hannah", "Niklas", "Clara",     "Jürgen",    "Katharina", "Benjamin", "Marlene" };

    const char* const name = names[i++];
    if (i >= SIZEOF_ARRAY(names)) { i = 0; }

    return name;
}

} // namespace blue

void blue::fn()
{
    int state = S_init;
    int threadSleep_us = initial_threadSleep_us;
    curl::QueueId curlId;
    time_t tReq = 0;

    while (!sd.doTerminate())
    {
        const time_t tNow = std::time(nullptr);

        switch (state)
        {
        case S_init:
            state = S_boot;
            break;

        case S_boot:
            if (curl::sharedData.booted())
            {
                sd.setBooted(true);
                LOG_TH("booted");
                state = S_idle;
            }
            break;

        case S_idle:
            if ((tNow - tReq) >= 0)
            {
                tReq = tNow;
                threadSleep_us = 100;
                state = S_req;
            }
            else { threadSleep_us = 10000; }
            break;

        case S_req:
        {
            const auto req = curl::GetRequest("https://api.agify.io/?name=" + getName(), 10);
            curlId = curl::queueRequest(req, curl::Priority::normal);
            if (curlId.isValid())
            {
                LOG_TH("[%i] %s", (int)curlId, req.toString().c_str());
                state = S_awaitRes;
            }
            else
            {
                LOG_TH(LOG_SGR_BRED "queue req failed, ID: %s, sd.response.ID: %s", curlId.toString().c_str(),
                       curl::sharedData.getResponseQueueId().toString().c_str());
                util::sleep(1000 * 1000);
            }
        }
        break;

        case S_awaitRes:
            if (curl::responseReady(curlId))
            {
                const auto res = curl::popResponse();

                if (res.good()) { LOG_TH("[%i] %s", (int)curlId, res.toString().c_str()); }
                else { LOG_TH(LOG_SGR_BRED "[%i] request failed: %s", (int)curlId, res.toString().c_str()); }

                state = S_idle;
            }
            break;

        default:
            LOG_ERR("invalid state %i at line %i", state, __LINE__);
            threadSleep_us = 5 * 1000 * 1000;
            break;
        }

        util::sleep(threadSleep_us);
    }

    LOG_TH("terminated");
}



#undef LOG_TH
#define LOG_TH LOG_CYA
void cyan::fn()
{
    int state = S_init;
    int threadSleep_us = initial_threadSleep_us;
    curl::QueueId curlId;
    time_t tReq = 0;

    while (!sd.doTerminate())
    {
        const time_t tNow = std::time(nullptr);

        switch (state)
        {
        case S_init:
            state = S_boot;
            break;

        case S_boot:
            if (curl::sharedData.booted())
            {
                sd.setBooted(true);
                LOG_TH("booted");
                state = S_idle;
            }
            break;

        case S_idle:
            if ((tNow - tReq) >= 16)
            {
                tReq = tNow;
                threadSleep_us = 100;
                state = S_req;
            }
            else { threadSleep_us = 10000; }
            break;

        case S_req:
        {
            static std::string group = "stations";

            const auto req = curl::GetRequest("https://celestrak.org/NORAD/elements/gp.php?GROUP=" + group + "&FORMAT=csv", 10, 1);
            curlId = curl::queueRequest(req, curl::Priority::normal);
            if (curlId.isValid())
            {
                if (group == "stations") { group = "amateur"; }
                else { group = "stations"; }

                LOG_TH("[%i] %s", (int)curlId, req.toString().c_str());
                state = S_awaitRes;
            }
            else
            {
                LOG_TH(LOG_SGR_BRED "queue req failed, ID: %s, sd.response.ID: %s", curlId.toString().c_str(),
                       curl::sharedData.getResponseQueueId().toString().c_str());
                util::sleep(1000 * 1000);
            }
        }
        break;

        case S_awaitRes:
            if (curl::responseReady(curlId))
            {
                const auto res = curl::popResponse();

                if (res.good()) { LOG_TH("[%i] %s", (int)curlId, res.toString().c_str()); }
                else { LOG_TH(LOG_SGR_BRED "[%i] request failed: %s", (int)curlId, res.toString().c_str()); }

                state = S_idle;
            }
            break;

        default:
            LOG_ERR("invalid state %i at line %i", state, __LINE__);
            threadSleep_us = 5 * 1000 * 1000;
            break;
        }

        util::sleep(threadSleep_us);
    }

    LOG_TH("terminated");
}



namespace magenta {

curl::Request generateRequest(curl::Priority* priority)
{
    static size_t i = 0;
    static const char* const url[] = { "https://api.ipify.org/?format=csv",
                                       "https://sat.terrestre.ar/passes/25544?lat=0&lon=0&limit=1&days=7&visible_only=false",
                                       "http://universities.hipolabs.com/search?country=Lichtenstein", "https://api.ipify.org/?format=json",
                                       "http://this-url-does-not-exist-asdfqwer.com" };

    if (i == 1) { *priority = curl::Priority::max; }
    else if (i == 3) { *priority = curl::Priority::high; }
    else { *priority = curl::Priority::normal; }
    const curl::Request req = curl::GetRequest(url[i++], 10);

    if (i >= SIZEOF_ARRAY(url)) { i = 0; }

    return req;
}

} // namespace magenta

#undef LOG_TH
#define LOG_TH LOG_MAG
void magenta::fn()
{
    int state = S_init;
    int threadSleep_us = initial_threadSleep_us;
    time_t tAction = 0;

    while (!sd.doTerminate())
    {
        const time_t tNow = std::time(nullptr);

        switch (state)
        {
        case S_init:
            state = S_boot;
            break;

        case S_boot:
            if (curl::sharedData.booted())
            {
                sd.setBooted(true);
                LOG_TH("booted");
                state = S_idle;
            }
            break;

        case S_idle:
        {
            static std::vector<curl::QueueId::id_type> ids;

            if ((tNow - tAction) >= 31)
            {
                curl::Priority prio;
                const curl::Request req = generateRequest(&prio);
                const curl::QueueId curlId = curl::queueRequest(req, prio);

                if (curlId.isValid())
                {
                    LOG_TH("[%i] %s", (int)curlId, req.toString().c_str());
                    ids.push_back(curlId);
                }
                else
                {
                    LOG_TH(LOG_SGR_BRED "queue req failed, ID: %s, sd.response.ID: %s", curlId.toString().c_str(),
                           curl::sharedData.getResponseQueueId().toString().c_str());

                    tAction = tNow;
                }
            }

            for (size_t i = 0; i < ids.size(); ++i)
            {
                const curl::QueueId curlId = ids[i];

                if (curl::responseReady(curlId))
                {
                    const auto res = curl::popResponse();

                    if (res.good()) { LOG_TH("[%i] %s", (int)curlId, res.toString().c_str()); }
                    else { LOG_TH(LOG_SGR_BRED "[%i] request failed: %s", (int)curlId, res.toString().c_str()); }

                    // remove this ID from the vector
                    ids.erase(ids.begin() + i);
                    i = (-1);
                }
            }
        }
        break;

        default:
            LOG_ERR("invalid state %i at line %i", state, __LINE__);
            threadSleep_us = 5 * 1000 * 1000;
            break;
        }

        util::sleep(threadSleep_us);
    }

    LOG_TH("terminated");
}



#undef LOG_TH
#define LOG_TH LOG_YEL
void yellow::fn()
{
    int state = S_init;
    curl::QueueId curlId;

    while (!sd.doTerminate())
    {
        switch (state)
        {
        case S_init:
            state = S_boot;
            break;

        case S_boot:
            if (curl::sharedData.booted())
            {
                sd.setBooted(true);
                LOG_TH("booted");
                state = S_idle;
            }
            break;

        case S_idle:
            if (std::time(0) != 0) { state = S_req; }
            break;

        case S_req:
        {
            const auto req = curl::GetRequest("https://timeapi.io/api/time/current/zone?timeZone=UTC", 10);
            curlId = curl::queueRequest(req, curl::Priority::high);
            if (curlId.isValid())
            {
                LOG_TH("[%i] %s", (int)curlId, req.toString().c_str());
                state = S_awaitRes;
            }
            else
            {
                LOG_TH(LOG_SGR_BRED "queue req failed, ID: %s, sd.response.ID: %s", curlId.toString().c_str(),
                       curl::sharedData.getResponseQueueId().toString().c_str());
                util::sleep(1000 * 1000);
            }
        }
        break;

        case S_awaitRes:
            if (curl::responseReady(curlId))
            {
                const auto res = curl::popResponse();

                if (res.good()) { LOG_TH("[%i] %s", (int)curlId, res.toString().c_str()); }
                else { LOG_TH(LOG_SGR_BRED "[%i] request failed: %s", (int)curlId, res.toString().c_str()); }

                if (res.good()) { sd.terminate(); }
                else { state = S_idle; }
            }
            break;

        default:
            LOG_ERR("invalid state %i at line %i", state, __LINE__);
            util::sleep(5 * 1000 * 1000);
            break;
        }

        util::sleep(100);
    }

    LOG_TH("terminated");
}

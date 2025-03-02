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
#define LOG_BLU(msg, ...) printf(___LOG_CSI_EL "\033[94m" "[%s] <BLU> " msg "\033[39m" "\n", util::t_to_iso8601_local(std::time(nullptr)).c_str() ___LOG_OPT_VA_ARGS(__VA_ARGS__))
#define LOG_CYA(msg, ...) printf(___LOG_CSI_EL "\033[96m" "[%s] <CYA> " msg "\033[39m" "\n", util::t_to_iso8601_local(std::time(nullptr)).c_str() ___LOG_OPT_VA_ARGS(__VA_ARGS__))
#define LOG_MAG(msg, ...) printf(___LOG_CSI_EL "\033[95m" "[%s] <MAG> " msg "\033[39m" "\n", util::t_to_iso8601_local(std::time(nullptr)).c_str() ___LOG_OPT_VA_ARGS(__VA_ARGS__))
#define LOG_YEL(msg, ...) printf(___LOG_CSI_EL "\033[93m" "[%s] <YEL> " msg "\033[39m" "\n", util::t_to_iso8601_local(std::time(nullptr)).c_str() ___LOG_OPT_VA_ARGS(__VA_ARGS__))
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

    util::sleep(2 * 60 * 1000 * 1000);

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

    blue::th.join();
    cyan::th.join();
    magenta::th.join();
    yellow::th.join();

    LOG_INF("exit");

    return 0;
}



#define LOG_TH LOG_BLU
void blue::fn()
{
    int state = S_init;
    int threadSleep_us = 500;
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
            if ((tNow - tReq) >= 1)
            {
                tReq = tNow;
                threadSleep_us = 100;
                state = S_req;
            }
            else { threadSleep_us = 10000; }
            break;

        case S_req:
        {
            const auto req = curl::GetRequest("https://timeapi.io/api/time/current/zone?timeZone=Europe%2FZurich", 10);
            curlId = curl::queueRequest(req, curl::Priority::normal);
            if (curlId.isValid())
            {
                LOG_TH("[%i] %s", (int)curlId, req.toString().c_str());
                state = S_awaitRes;
            }
            else
            {
                LOG_TH(LOG_SGR_BRED "queue req failed, ID: %s", curlId.toString().c_str());
                util::sleep(1000 * 1000);
            }
        }
        break;

        case S_awaitRes:
            if (curl::responseReady(curlId))
            {
                const auto res = curl::popResponse();

                if (res.good()) { LOG_TH("%s", res.toString().c_str()); }
                else { LOG_TH(LOG_SGR_BRED "request failed: %s", res.toString().c_str()); }

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

        default:
            LOG_ERR("invalid state %i at line %i", state, __LINE__);
            util::sleep(5 * 1000 * 1000);
            break;
        }

        util::sleep(100);
    }

    LOG_TH("terminated");
}

#undef LOG_TH
#define LOG_TH LOG_MAG
void magenta::fn()
{
    int state = S_init;

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



        default:
            LOG_ERR("invalid state %i at line %i", state, __LINE__);
            util::sleep(5 * 1000 * 1000);
            break;
        }

        util::sleep(100);
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
            curlId = curl::queueRequest(req, curl::Priority::normal);
            if (curlId.isValid())
            {
                LOG_TH("[%i] %s", (int)curlId, req.toString().c_str());
                state = S_awaitRes;
            }
            else
            {
                LOG_TH(LOG_SGR_BRED "queue req failed, ID: %s", curlId.toString().c_str());
                util::sleep(1000 * 1000);
            }
        }
        break;

        case S_awaitRes:
            if (curl::responseReady(curlId))
            {
                const auto res = curl::popResponse();
                if (res.good())
                {
                    LOG_TH("%s", res.toString().c_str());
                    sd.terminate();
                }
                else
                {
                    LOG_TH(LOG_SGR_BRED "request failed: %s", res.toString().c_str());
                    state = S_idle;
                }
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

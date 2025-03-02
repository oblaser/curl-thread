/*
author          Oliver Blaser
copyright       MIT - Copyright (c) 2024 Oliver Blaser
*/

#ifndef IG_MIDDLEWARE_LOG_H
#define IG_MIDDLEWARE_LOG_H

#include <cstdio>



#define LOG_LEVEL_OFF (0)
#define LOG_LEVEL_ERR (1)
#define LOG_LEVEL_WRN (2)
#define LOG_LEVEL_INF (3)
#define LOG_LEVEL_DBG (4)



// stringify
#define ___LOG_STR_HELPER(x) #x
#define ___LOG_STR(x)        ___LOG_STR_HELPER(x)



#ifndef CONFIG_LOG_LEVEL
#ifdef _MSC_VER
#define __PRAGMA_LOC__ __FILE__ "(" ___LOG_STR(__LINE__) ") "
__pragma(message(__PRAGMA_LOC__ ": warning: \"CONFIG_LOG_LEVEL is not defined, defaulting to 2 (warning)\""))
#undef __PRAGMA_LOC__
#else //_MSC_VER
#warning "CONFIG_LOG_LEVEL is not defined, defaulting to 2 (warning)"
#endif //_MSC_VER
#define CONFIG_LOG_LEVEL LOG_LEVEL_WRN
#endif

#ifndef LOG_MODULE_LEVEL
#error "define LOG_MODULE_LEVEL before including log.h"
#endif
#ifndef LOG_MODULE_NAME
#error "define LOG_MODULE_NAME before including log.h"
#endif



// SGR foreground colors
#define LOG_SGR_BLACK    "\033[30m"
#define LOG_SGR_RED      "\033[31m"
#define LOG_SGR_GREEN    "\033[32m"
#define LOG_SGR_YELLOW   "\033[33m"
#define LOG_SGR_BLUE     "\033[34m"
#define LOG_SGR_MAGENTA  "\033[35m"
#define LOG_SGR_CYAN     "\033[36m"
#define LOG_SGR_WHITE    "\033[37m"
#define LOG_SGR_DEFAULT  "\033[39m"
#define LOG_SGR_BBLACK   "\033[90m"
#define LOG_SGR_BRED     "\033[91m"
#define LOG_SGR_BGREEN   "\033[92m"
#define LOG_SGR_BYELLOW  "\033[93m"
#define LOG_SGR_BBLUE    "\033[94m"
#define LOG_SGR_BMAGENTA "\033[95m"
#define LOG_SGR_BCYAN    "\033[96m"
#define LOG_SGR_BWHITE   "\033[97m"



// optional args
#define ___LOG_OPT_VA_ARGS(...) , ##__VA_ARGS__

#define ___LOG_CSI_EL "\033[2K" // ANSI ESC CSI erase line



// config can limit log level
#if (CONFIG_LOG_LEVEL < LOG_MODULE_LEVEL)
#undef LOG_MODULE_LEVEL
#define LOG_MODULE_LEVEL CONFIG_LOG_LEVEL
#endif



#include "middleware/util.h"

// clang-format off
#define LOG_ERR(msg, ...) printf(___LOG_CSI_EL "[%s] " "\033[91m" ___LOG_STR(LOG_MODULE_NAME) " <ERR> " msg "\033[39m" "\n", util::t_to_iso8601_local(std::time(nullptr)).c_str() ___LOG_OPT_VA_ARGS(__VA_ARGS__))
#define LOG_WRN(msg, ...) printf(___LOG_CSI_EL "[%s] " "\033[93m" ___LOG_STR(LOG_MODULE_NAME) " <WRN> " msg "\033[39m" "\n", util::t_to_iso8601_local(std::time(nullptr)).c_str() ___LOG_OPT_VA_ARGS(__VA_ARGS__))
#define LOG_INF(msg, ...) printf(___LOG_CSI_EL "[%s] " "\033[39m" ___LOG_STR(LOG_MODULE_NAME) " <INF> " msg "\033[39m" "\n", util::t_to_iso8601_local(std::time(nullptr)).c_str() ___LOG_OPT_VA_ARGS(__VA_ARGS__))
//#define LOG_DBG(msg, ...) printf(___LOG_CSI_EL "[%s] " "\033[39m" ___LOG_STR(LOG_MODULE_NAME) " <DBG> " msg "\033[39m" "\n", util::t_to_iso8601_local(std::time(nullptr)).c_str() ___LOG_OPT_VA_ARGS(__VA_ARGS__))
#define LOG_DBG(msg, ...) printf(___LOG_CSI_EL "[%s] " "\033[39m" ___LOG_STR(LOG_MODULE_NAME) " <DBG> \033[90m" __func__ "():%i\033[39m " msg "\033[39m" "\n", util::t_to_iso8601_local(std::time(nullptr)).c_str(), (int)(__LINE__) ___LOG_OPT_VA_ARGS(__VA_ARGS__))
// clang-format on



#if (LOG_MODULE_LEVEL < LOG_LEVEL_DBG)
#undef LOG_DBG
#define LOG_DBG(...) (void)0
#endif
#if (LOG_MODULE_LEVEL < LOG_LEVEL_INF)
#undef LOG_INF
#define LOG_INF(...) (void)0
#endif
#if (LOG_MODULE_LEVEL < LOG_LEVEL_WRN)
#undef LOG_WRN
#define LOG_WRN(...) (void)0
#endif
#if (LOG_MODULE_LEVEL < LOG_LEVEL_ERR)
#undef LOG_ERR
#define LOG_ERR(...) (void)0
#endif



#endif // IG_MIDDLEWARE_LOG_H

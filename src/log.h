/*
 * renderer-service-upnp
 *
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Ludovic Ferrandis <ludovic.ferrandis@intel.com>
 *
 */

#ifndef RSU_LOG_H__
#define RSU_LOG_H__

#include <syslog.h>

#include <glib.h>

enum rsu_log_type_t_ {
	RSU_LOG_TYPE_SYSLOG = 0,
	RSU_LOG_TYPE_GLIB,
	RSU_LOG_TYPE_FILE
};
typedef enum rsu_log_type_t_ rsu_log_type_t;

typedef struct rsu_log_t_ rsu_log_t;
struct rsu_log_t_ {
	int old_mask;
	int mask;
	rsu_log_type_t log_type;
	GLogLevelFlags flags;
	GLogFunc old_handler;
};

void rsu_log_init(const char *program);

void rsu_log_finalize(void);

void rsu_log_trace(int priority, GLogLevelFlags flags, const char *format, ...)
			__attribute__((format(printf, 3, 4)));


/* Generic Logging macro
 */
#ifdef RSU_DEBUG_ENABLED
	#define RSU_LOG_HELPER(priority, flags, fmt, ...)    \
		do { \
			rsu_log_trace(priority, flags, "%s : %s() --- " fmt, \
				      __FILE__, __func__, ## __VA_ARGS__); \
		} while (0)
#else
	#define RSU_LOG_HELPER(priority, flags, fmt, ...) \
		do { \
			rsu_log_trace(priority, flags, fmt, ## __VA_ARGS__); \
		} while (0)
#endif


/* Logging macro for error messages
 */
#if RSU_LOG_LEVEL & RSU_MSU_LOG_LEVEL_ERROR
	#define RSU_LOG_ERROR(...) \
		RSU_LOG_HELPER(LOG_ERR, G_LOG_LEVEL_ERROR, __VA_ARGS__, 0)
#else
	#define RSU_LOG_ERROR(...)
#endif


/* Logging macro for critical messages
 */
#if RSU_LOG_LEVEL & RSU_LOG_LEVEL_CRITICAL
	#define RSU_LOG_CRITICAL(...) \
		RSU_LOG_HELPER(LOG_CRIT, G_LOG_LEVEL_CRITICAL, __VA_ARGS__, 0)
#else
	#define RSU_LOG_CRITICAL(fmt, ...)
#endif


/* Logging macro for warning messages
 */
#if RSU_LOG_LEVEL & RSU_LOG_LEVEL_WARNING
	#define RSU_LOG_WARNING(...) \
		RSU_LOG_HELPER(LOG_WARNING, G_LOG_LEVEL_WARNING, __VA_ARGS__, 0)
#else
	#define RSU_LOG_WARNING(fmt, ...)
#endif


/* Logging macro for messages
 */
#if RSU_LOG_LEVEL & RSU_LOG_LEVEL_MESSAGE
	#define RSU_LOG_MESSAGE(...) \
		RSU_LOG_HELPER(LOG_NOTICE, G_LOG_LEVEL_MESSAGE, __VA_ARGS__, 0)
#else
	#define RSU_LOG_MESSAGE(fmt, ...)
#endif


/* Logging macro for informational messages
 */
#if RSU_LOG_LEVEL & RSU_LOG_LEVEL_INFO
	#define RSU_LOG_INFO(...) \
		RSU_LOG_HELPER(LOG_INFO, G_LOG_LEVEL_INFO, __VA_ARGS__, 0)
#else
	#define RSU_LOG_INFO(fmt, ...)
#endif


/* Logging macro for debug messages
 */
#if RSU_LOG_LEVEL & RSU_LOG_LEVEL_DEBUG
	#define RSU_LOG_DEBUG(...) \
		RSU_LOG_HELPER(LOG_DEBUG, G_LOG_LEVEL_DEBUG, __VA_ARGS__, 0)
#else
	#define RSU_LOG_DEBUG(fmt, ...)
#endif

#endif /* RSU_LOG_H__ */

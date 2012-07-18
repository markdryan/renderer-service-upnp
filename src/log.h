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

void rsu_log_init(const char *program, rsu_log_t *log_context);

void rsu_log_finialize(rsu_log_t *log_context);

void rsu_log_error(const char *format, ...)
			__attribute__((format(printf, 1, 2)));

void rsu_log_critical(const char *format, ...)
			__attribute__((format(printf, 1, 2)));

void rsu_log_warning(const char *format, ...)
			__attribute__((format(printf, 1, 2)));

void rsu_log_message(const char *format, ...)
			__attribute__((format(printf, 1, 2)));

void rsu_log_info(const char *format, ...)
			__attribute__((format(printf, 1, 2)));

void rsu_log_debug(const char *format, ...)
			__attribute__((format(printf, 1, 2)));



/* Logging macro for error messages
 */
#if RSU_LOG_LEVEL & RSU_LOG_LEVEL_ERROR
	#ifdef RSU_DEBUG_ENABLED
		#define RSU_LOG_ERROR(fmt, ...) \
			do { \
				rsu_log_error("%s:%s() " fmt, __FILE__, \
						__func__, ## __VA_ARGS__); \
			} while (0)
	#else
		#define RSU_LOG_ERROR(fmt, ...) \
			do { \
				rsu_log_error(fmt, ## __VA_ARGS__); \
			} while (0)
	#endif
#else
	#define RSU_LOG_ERROR(fmt, ...)
#endif


/* Logging macro for critical messages
 */
#if RSU_LOG_LEVEL & RSU_LOG_LEVEL_CRITICAL
	#ifdef RSU_DEBUG_ENABLED
		#define RSU_LOG_CRITICAL(fmt, ...) \
			do { \
				rsu_log_critical("%s:%s() " fmt, __FILE__, \
						__func__, ## __VA_ARGS__); \
			} while (0)
	#else
		#define RSU_LOG_CRITICAL(fmt, ...) \
			do { \
				rsu_log_critical(fmt, ## __VA_ARGS__); \
			} while (0)
	#endif
#else
	#define RSU_LOG_CRITICAL(fmt, ...)
#endif


/* Logging macro for warning messages
 */
#if RSU_LOG_LEVEL & RSU_LOG_LEVEL_WARNING
	#ifdef RSU_DEBUG_ENABLED
		#define RSU_LOG_WARNING(fmt, ...) \
			do { \
				rsu_log_warning("%s:%s() " fmt, __FILE__, \
						__func__, ## __VA_ARGS__); \
			} while (0)
	#else
		#define RSU_LOG_WARNING(fmt, ...) \
			do { \
				rsu_log_warning(fmt, ## __VA_ARGS__); \
			} while (0)
	#endif
#else
	#define RSU_LOG_WARNING(fmt, ...)
#endif


/* Logging macro for messages
 */
#if RSU_LOG_LEVEL & RSU_LOG_LEVEL_MESSAGE
	#ifdef RSU_DEBUG_ENABLED
		#define RSU_LOG_MESSAGE(fmt, ...) \
			do { \
				rsu_log_message("%s:%s() " fmt, __FILE__, \
						__func__, ## __VA_ARGS__); \
			} while (0)
	#else
		#define RSU_LOG_MESSAGE(fmt, ...) \
			do { \
				rsu_log_message(fmt, ## __VA_ARGS__); \
			} while (0)
	#endif
#else
	#define RSU_LOG_MESSAGE(fmt, ...)
#endif


/* Logging macro for informational messages
 */
#if RSU_LOG_LEVEL & RSU_LOG_LEVEL_INFO
	#ifdef RSU_DEBUG_ENABLED
		#define RSU_LOG_INFO(fmt, ...) \
			do { \
				rsu_log_info("%s:%s() " fmt, __FILE__, \
						__func__, ## __VA_ARGS__); \
			} while (0)
	#else
		#define RSU_LOG_INFO(fmt, ...) \
			do { \
				rsu_log_info(fmt, ## __VA_ARGS__); \
			} while (0)
	#endif
#else
	#define RSU_LOG_INFO(fmt, ...)
#endif


/* Logging macro for debug messages
 */
#if RSU_LOG_LEVEL & RSU_LOG_LEVEL_DEBUG
	#ifdef RSU_DEBUG_ENABLED
		#define RSU_LOG_DEBUG(fmt, ...) \
			do { \
				rsu_log_debug("%s:%s() " fmt, __FILE__, \
						__func__, ## __VA_ARGS__); \
			} while (0)
	#else
		#define RSU_LOG_DEBUG(fmt, ...) \
			do { \
				rsu_log_debug(fmt, ## __VA_ARGS__); \
			} while (0)
	#endif
#else
	#define RSU_LOG_DEBUG(fmt, ...)
#endif

#endif /* RSU_LOG_H__ */

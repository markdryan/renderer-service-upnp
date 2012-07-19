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


#define _GNU_SOURCE
#include <stdarg.h>
#include <string.h>
#include <syslog.h>

#include <glib.h>

#include "log.h"

static rsu_log_t *s_log_context;

static void prv_rsu_log_set_flags(rsu_log_t *log_context)
{
	int mask = 0;
	GLogLevelFlags flags = 0;

	if (RSU_LOG_LEVEL & RSU_LOG_LEVEL_ERROR) {
		mask |= LOG_MASK(LOG_ERR);
		flags |= G_LOG_LEVEL_ERROR;
	}

	if (RSU_LOG_LEVEL & RSU_LOG_LEVEL_CRITICAL) {
		mask |= LOG_MASK(LOG_CRIT);
		flags |= G_LOG_LEVEL_CRITICAL;
	}

	if (RSU_LOG_LEVEL & RSU_LOG_LEVEL_WARNING) {
		mask |= LOG_MASK(LOG_WARNING);
		flags |= G_LOG_LEVEL_WARNING;
	}

	if (RSU_LOG_LEVEL & RSU_LOG_LEVEL_MESSAGE) {
		mask |= LOG_MASK(LOG_NOTICE);
		flags |= G_LOG_LEVEL_MESSAGE;
	}

	if (RSU_LOG_LEVEL & RSU_LOG_LEVEL_INFO) {
		mask |= LOG_MASK(LOG_INFO);
		flags |= G_LOG_LEVEL_INFO;
	}

	if (RSU_LOG_LEVEL & RSU_LOG_LEVEL_DEBUG) {
		mask |= LOG_MASK(LOG_DEBUG);
		flags |= G_LOG_LEVEL_DEBUG;
	}

	if (flags)
		flags |= G_LOG_FLAG_RECURSION | G_LOG_FLAG_FATAL;

	log_context->mask = mask;
	log_context->flags = flags;
}

static void prv_rsu_log_handler(const gchar *log_domain,
				GLogLevelFlags log_level,
				const gchar *message,
				gpointer data)
{
	rsu_log_t *log_context = (rsu_log_t *)(data);

	if (strcmp(log_domain, G_LOG_DOMAIN))
		return;

	if (log_context->flags & log_level)
		g_log_default_handler(log_domain, log_level, message, data);
}

void rsu_log_init(const char *program, rsu_log_t *log_context)
{
	int option = LOG_NDELAY | LOG_PID;
	int old;

#ifdef RSU_DEBUG_ENABLED
	option |= LOG_PERROR | LOG_CONS;
#endif

	memset(log_context, 0, sizeof(rsu_log_t));
	prv_rsu_log_set_flags(log_context);

	openlog(basename(program), option, LOG_DAEMON);

	old = setlogmask(LOG_MASK(LOG_INFO));
	syslog(LOG_INFO, "Media Service UPnP version %s", VERSION);
	(void) setlogmask(log_context->mask);

	log_context->old_mask = old;
	log_context->old_handler = g_log_set_default_handler(
					prv_rsu_log_handler,
					log_context);

	s_log_context = log_context;

	if (log_context->log_type != RSU_LOG_TYPE_SYSLOG)
		RSU_LOG_INFO("Media Service UPnP version %s", VERSION);
}

void rsu_log_finalize(rsu_log_t *log_context)
{
	(void) setlogmask(LOG_MASK(LOG_INFO));
	syslog(LOG_INFO, "Media Service UPnP: Exit");

	if (log_context->log_type != RSU_LOG_TYPE_SYSLOG)
		RSU_LOG_INFO("%s", "Media Service UPnP: Exit");

	(void) g_log_set_default_handler(log_context->old_handler, NULL);

	(void) setlogmask(log_context->old_mask);
	closelog();

	s_log_context = NULL;
}

void rsu_log_trace(int priority, GLogLevelFlags flags, const char *format, ...)
{
	va_list args;

	va_start(args, format);

	switch (s_log_context->log_type) {
	case RSU_LOG_TYPE_SYSLOG:
		vsyslog(priority, format, args);
		break;
	case RSU_LOG_TYPE_GLIB:
		g_logv(G_LOG_DOMAIN, flags, format, args);
		break;
	case RSU_LOG_TYPE_FILE:
		break;
	default:
		break;
	}

	va_end(args);
}

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
 * Mark Ryan <mark.d.ryan@intel.com>
 *
 */


#include "async.h"
#include "error.h"

rsu_async_cb_data_t *rsu_async_cb_data_new(rsu_task_t *task,
					   rsu_upnp_task_complete_t cb,
					   void *user_data,
					   gpointer private,
					   GDestroyNotify free_private,
					   rsu_device_t *device)
{
	rsu_async_cb_data_t *cb_data = g_new0(rsu_async_cb_data_t, 1);

	cb_data->type = task->type;
	cb_data->task = task;
	cb_data->cb = cb;
	cb_data->user_data = user_data;
	cb_data->private = private;
	cb_data->free_private = free_private;
	cb_data->device = device;

	return cb_data;
}

static void prv_rsu_upnp_cb_data_delete(rsu_async_cb_data_t *cb_data)
{
	if (cb_data) {
		if (cb_data->free_private)
			cb_data->free_private(cb_data->private);
		g_free(cb_data);
	}
}

gboolean rsu_async_complete_task(gpointer user_data)
{
	rsu_async_cb_data_t *cb_data = user_data;

	cb_data->device->current_task = NULL;
	cb_data->cb(cb_data->task, cb_data->result, cb_data->error,
		    cb_data->user_data);
	prv_rsu_upnp_cb_data_delete(cb_data);

	return FALSE;
}

void rsu_async_task_cancelled(GCancellable *cancellable, gpointer user_data)
{
	rsu_async_cb_data_t *cb_data = user_data;

	cb_data->device->current_task = NULL;
	gupnp_service_proxy_cancel_action(cb_data->proxy, cb_data->action);
	if (!cb_data->error)
		cb_data->error = g_error_new(RSU_ERROR, RSU_ERROR_CANCELLED,
					     "Operation cancelled.");
	(void) g_idle_add(rsu_async_complete_task, cb_data);
}

void rsu_async_task_lost_object(gpointer user_data)
{
	rsu_async_cb_data_t *cb_data = user_data;

	if (!cb_data->error)
		cb_data->error = g_error_new(RSU_ERROR, RSU_ERROR_LOST_OBJECT,
					     "Renderer died before command "
					     "could be completed.");
	(void) g_idle_add(rsu_async_complete_task, cb_data);
}

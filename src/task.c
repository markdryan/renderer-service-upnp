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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "task.h"
#include "error.h"

rsu_task_t *rsu_task_get_version_new(GDBusMethodInvocation *invocation)
{
	rsu_task_t *task = g_new0(rsu_task_t, 1);

	task->type = RSU_TASK_GET_VERSION;
	task->invocation = invocation;
	task->result_format = "(@s)";
	task->result = g_variant_ref_sink(g_variant_new_string(VERSION));
	task->synchronous = TRUE;

	return task;
}

rsu_task_t *rsu_task_get_servers_new(GDBusMethodInvocation *invocation)
{
	rsu_task_t *task = g_new0(rsu_task_t, 1);

	task->type = RSU_TASK_GET_SERVERS;
	task->invocation = invocation;
	task->result_format = "(@as)";
	task->synchronous = TRUE;

	return task;
}

rsu_task_t *rsu_task_raise_new(GDBusMethodInvocation *invocation)
{
	rsu_task_t *task = g_new0(rsu_task_t, 1);

	task->type = RSU_TASK_RAISE;
	task->invocation = invocation;
	task->synchronous = TRUE;

	return task;
}

rsu_task_t *rsu_task_quit_new(GDBusMethodInvocation *invocation)
{
	rsu_task_t *task = g_new0(rsu_task_t, 1);

	task->type = RSU_TASK_QUIT;
	task->invocation = invocation;
	task->synchronous = TRUE;

	return task;
}

static void prv_rsu_task_delete(rsu_task_t *task)
{
	switch (task->type) {
	case RSU_TASK_GET_ALL_PROPS:
		g_free(task->get_props.interface_name);
		break;
	case RSU_TASK_GET_PROP:
		g_free(task->get_prop.interface_name);
		g_free(task->get_prop.prop_name);
		break;
	case RSU_TASK_OPEN_URI:
		g_free(task->open_uri.uri);
		break;
	case RSU_TASK_HOST_URI:
	case RSU_TASK_REMOVE_URI:
		g_free(task->host_uri.uri);
		g_free(task->host_uri.client);
		break;
	default:
		break;
	}

	g_free(task->path);
	if (task->result)
		g_variant_unref(task->result);

	g_free(task);
}

static rsu_task_t *prv_device_task_new(rsu_task_type_t type,
				       GDBusMethodInvocation *invocation,
				       const gchar *path,
				       const gchar *result_format)
{
	rsu_task_t *task = g_new0(rsu_task_t, 1);

	task->type = type;
	task->invocation = invocation;
	task->result_format = result_format;

	task->path = g_strdup(path);
	g_strstrip(task->path);

	return task;
}

rsu_task_t *rsu_task_get_prop_new(GDBusMethodInvocation *invocation,
				  const gchar *path, GVariant *parameters)
{
	rsu_task_t *task;

	task = prv_device_task_new(RSU_TASK_GET_PROP, invocation, path, "(v)");

	g_variant_get(parameters, "(ss)", &task->get_prop.interface_name,
		      &task->get_prop.prop_name);

	g_strstrip(task->get_prop.interface_name);
	g_strstrip(task->get_prop.prop_name);

	return task;
}

rsu_task_t *rsu_task_get_props_new(GDBusMethodInvocation *invocation,
				   const gchar *path, GVariant *parameters)
{
	rsu_task_t *task;

	task = prv_device_task_new(RSU_TASK_GET_ALL_PROPS, invocation, path,
				   "(@a{sv})");

	g_variant_get(parameters, "(s)", &task->get_props.interface_name);
	g_strstrip(task->get_props.interface_name);

	return task;
}

rsu_task_t *rsu_task_play_new(GDBusMethodInvocation *invocation,
			      const gchar *path)
{
	return prv_device_task_new(RSU_TASK_PLAY, invocation, path, NULL);
}

rsu_task_t *rsu_task_pause_new(GDBusMethodInvocation *invocation,
			      const gchar *path)
{
	return prv_device_task_new(RSU_TASK_PAUSE, invocation, path, NULL);
}

rsu_task_t *rsu_task_play_pause_new(GDBusMethodInvocation *invocation,
				    const gchar *path)
{
	return prv_device_task_new(RSU_TASK_PLAY_PAUSE, invocation, path, NULL);
}

rsu_task_t *rsu_task_stop_new(GDBusMethodInvocation *invocation,
			      const gchar *path)
{
	return prv_device_task_new(RSU_TASK_STOP, invocation, path, NULL);
}

rsu_task_t *rsu_task_next_new(GDBusMethodInvocation *invocation,
			      const gchar *path)
{
	return prv_device_task_new(RSU_TASK_NEXT, invocation, path, NULL);
}

rsu_task_t *rsu_task_previous_new(GDBusMethodInvocation *invocation,
				  const gchar *path)
{
	return prv_device_task_new(RSU_TASK_PREVIOUS, invocation, path, NULL);
}

rsu_task_t *rsu_task_seek_new(GDBusMethodInvocation *invocation,
			      const gchar *path, GVariant *parameters)
{
	rsu_task_t *task = prv_device_task_new(RSU_TASK_SEEK, invocation,
					       path, NULL);

	g_variant_get(parameters, "(x)", &task->seek.position);

	return task;
}

rsu_task_t *rsu_task_set_position_new(GDBusMethodInvocation *invocation,
				      const gchar *path, GVariant *parameters)
{
	gchar *track_id;

	rsu_task_t *task = prv_device_task_new(RSU_TASK_SET_POSITION,
					       invocation, path, NULL);

	g_variant_get(parameters, "(&ox)", &track_id, &task->seek.position);

	return task;
}

rsu_task_t *rsu_task_open_uri_new(GDBusMethodInvocation *invocation,
				  const gchar *path, GVariant *parameters)
{
	rsu_task_t *task;

	task = prv_device_task_new(RSU_TASK_OPEN_URI, invocation, path,
				   NULL);

	g_variant_get(parameters, "(s)", &task->open_uri.uri);
	g_strstrip(task->open_uri.uri);

	return task;
}

rsu_task_t *rsu_task_host_uri_new(GDBusMethodInvocation *invocation,
				  const gchar *path,
				  GVariant *parameters)
{
	rsu_task_t *task;

	task = prv_device_task_new(RSU_TASK_HOST_URI, invocation, path,
				   "(@s)");

	g_variant_get(parameters, "(s)", &task->host_uri.uri);
	g_strstrip(task->host_uri.uri);
	task->host_uri.client = g_strdup(
		g_dbus_method_invocation_get_sender(invocation));

	return task;
}

rsu_task_t *rsu_task_remove_uri_new(GDBusMethodInvocation *invocation,
				    const gchar *path,
				    GVariant *parameters)
{
	rsu_task_t *task;

	task = prv_device_task_new(RSU_TASK_REMOVE_URI, invocation, path, NULL);

	g_variant_get(parameters, "(s)", &task->host_uri.uri);
	g_strstrip(task->host_uri.uri);
	task->host_uri.client = g_strdup(
		g_dbus_method_invocation_get_sender(invocation));

	return task;
}

void rsu_task_complete_and_delete(rsu_task_t *task)
{
	if (!task)
		goto finished;

	if (task->invocation) {
		if (task->result_format && task->result)
			g_dbus_method_invocation_return_value(
				task->invocation,
				g_variant_new(task->result_format,
					      task->result));
		else
			g_dbus_method_invocation_return_value(task->invocation,
							      NULL);
	}

	prv_rsu_task_delete(task);

finished:

	return;
}

void rsu_task_fail_and_delete(rsu_task_t *task, GError *error)
{
	if (!task)
		goto finished;

	if (task->invocation)
		g_dbus_method_invocation_return_gerror(task->invocation, error);

	prv_rsu_task_delete(task);

finished:

	return;
}

void rsu_task_delete(rsu_task_t *task)
{
	GError *error;

	if (!task)
		goto finished;

	if (task->invocation) {
		error = g_error_new(RSU_ERROR, RSU_ERROR_DIED,
				    "Unable to complete command.");
		g_dbus_method_invocation_return_gerror(task->invocation, error);
		g_error_free(error);
	}

	prv_rsu_task_delete(task);

finished:

	return;
}

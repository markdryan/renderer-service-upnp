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

#ifndef RSU_DEVICE_H__
#define RSU_DEVICE_H__

#include <glib.h>
#include <gio/gio.h>

#include "upnp.h"
#include "host-service.h"

typedef struct rsu_device_t_ rsu_device_t;

typedef struct rsu_service_proxies_t_ rsu_service_proxies_t;
struct rsu_service_proxies_t_ {
	GUPnPServiceProxy *cm_proxy;
	GUPnPServiceProxy *av_proxy;
	GUPnPServiceProxy *rc_proxy;
};

typedef struct rsu_context_t_ rsu_context_t;
struct rsu_context_t_ {
	gchar *ip_address;
	GUPnPDeviceProxy *device_proxy;
	rsu_service_proxies_t service_proxies;
	rsu_device_t *device;
};

typedef struct rsu_props_t_ rsu_props_t;
struct rsu_props_t_ {
	GHashTable *root_props;
	GHashTable *player_props;
	gboolean synced;
};

struct rsu_device_t_ {
	GDBusConnection *connection;
	guint ids[RSU_INTERFACE_INFO_MAX];
	gchar *path;
	GPtrArray *contexts;
	gpointer current_task;
	rsu_props_t props;
};

gboolean rsu_device_new(GDBusConnection *connection,
			GUPnPDeviceProxy *proxy,
			const gchar *ip_address,
			guint counter,
			rsu_interface_info_t *interface_info,
			void *user_data,
			rsu_device_t **device);

void rsu_device_delete(void *device);

void rsu_device_append_new_context(rsu_device_t *device,
				   const gchar *ip_address,
				   GUPnPDeviceProxy *proxy);
rsu_device_t *rsu_device_from_path(const gchar *path, GHashTable *device_list);
rsu_context_t *rsu_device_get_context(rsu_device_t *device);

void rsu_device_get_prop(rsu_device_t *device, rsu_task_t *task,
			GCancellable *cancellable,
			rsu_upnp_task_complete_t cb,
			void *user_data);
void rsu_device_get_all_props(rsu_device_t *device, rsu_task_t *task,
			      GCancellable *cancellable,
			      rsu_upnp_task_complete_t cb,
			      void *user_data);
void rsu_device_play(rsu_device_t *device, rsu_task_t *task,
		     GCancellable *cancellable,
		     rsu_upnp_task_complete_t cb,
		     void *user_data);
void rsu_device_pause(rsu_device_t *device, rsu_task_t *task,
		     GCancellable *cancellable,
		     rsu_upnp_task_complete_t cb,
		     void *user_data);
void rsu_device_play_pause(rsu_device_t *device, rsu_task_t *task,
			   GCancellable *cancellable,
			   rsu_upnp_task_complete_t cb,
			   void *user_data);
void rsu_device_stop(rsu_device_t *device, rsu_task_t *task,
		     GCancellable *cancellable,
		     rsu_upnp_task_complete_t cb,
		     void *user_data);
void rsu_device_next(rsu_device_t *device, rsu_task_t *task,
		     GCancellable *cancellable,
		     rsu_upnp_task_complete_t cb,
		     void *user_data);
void rsu_device_previous(rsu_device_t *device, rsu_task_t *task,
			 GCancellable *cancellable,
			 rsu_upnp_task_complete_t cb,
			 void *user_data);
void rsu_device_open_uri(rsu_device_t *device, rsu_task_t *task,
			 GCancellable *cancellable,
			 rsu_upnp_task_complete_t cb,
			 void *user_data);
void rsu_device_seek(rsu_device_t *device, rsu_task_t *task,
		     GCancellable *cancellable,
		     rsu_upnp_task_complete_t cb,
		     void *user_data);
void rsu_device_set_position(rsu_device_t *device, rsu_task_t *task,
			     GCancellable *cancellable,
			     rsu_upnp_task_complete_t cb,
			     void *user_data);
void rsu_device_host_uri(rsu_device_t *device, rsu_task_t *task,
			 rsu_host_service_t *host_service,
			 GCancellable *cancellable,
			 rsu_upnp_task_complete_t cb,
			 void *user_data);
void rsu_device_remove_uri(rsu_device_t *device, rsu_task_t *task,
			   rsu_host_service_t *host_service,
			   GCancellable *cancellable,
			   rsu_upnp_task_complete_t cb,
			   void *user_data);

#endif

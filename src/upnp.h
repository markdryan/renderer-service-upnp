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

#ifndef RSU_UPNP_H__
#define RSU_UPNP_H__

#include "task.h"

typedef struct rsu_upnp_t_ rsu_upnp_t;

enum rsu_interface_type_ {
	RSU_INTERFACE_INFO_PROPERTIES,
	RSU_INTERFACE_INFO_ROOT,
	RSU_INTERFACE_INFO_PLAYER,
	RSU_INTERFACE_INFO_PUSH_HOST,
	RSU_INTERFACE_INFO_DEVICE,
	RSU_INTERFACE_INFO_MAX
};

typedef struct rsu_interface_info_t_ rsu_interface_info_t;
struct rsu_interface_info_t_ {
	GDBusInterfaceInfo *interface;
	const GDBusInterfaceVTable *vtable;
};

typedef void (*rsu_upnp_callback_t)(const gchar *path, void *user_data);
typedef void (*rsu_upnp_task_complete_t)(rsu_task_t *task, GVariant *result,
					 GError *error, void *user_data);

rsu_upnp_t *rsu_upnp_new(GDBusConnection *connection,
			 rsu_interface_info_t *interface_info,
			 rsu_upnp_callback_t found_server,
			 rsu_upnp_callback_t lost_server,
			 void *user_data);
void rsu_upnp_delete(rsu_upnp_t *upnp);
GVariant *rsu_upnp_get_server_ids(rsu_upnp_t *upnp);
void rsu_upnp_get_prop(rsu_upnp_t *upnp, rsu_task_t *task,
		       GCancellable *cancellable,
		       rsu_upnp_task_complete_t cb,
		       void *user_data);
void rsu_upnp_get_all_props(rsu_upnp_t *upnp, rsu_task_t *task,
			    GCancellable *cancellable,
			    rsu_upnp_task_complete_t cb,
			    void *user_data);
void rsu_upnp_play(rsu_upnp_t *upnp, rsu_task_t *task,
		   GCancellable *cancellable,
		   rsu_upnp_task_complete_t cb,
		   void *user_data);
void rsu_upnp_pause(rsu_upnp_t *upnp, rsu_task_t *task,
		    GCancellable *cancellable,
		    rsu_upnp_task_complete_t cb,
		    void *user_data);
void rsu_upnp_play_pause(rsu_upnp_t *upnp, rsu_task_t *task,
			 GCancellable *cancellable,
			 rsu_upnp_task_complete_t cb,
			 void *user_data);
void rsu_upnp_stop(rsu_upnp_t *upnp, rsu_task_t *task,
		   GCancellable *cancellable,
		   rsu_upnp_task_complete_t cb,
		   void *user_data);
void rsu_upnp_next(rsu_upnp_t *upnp, rsu_task_t *task,
		   GCancellable *cancellable,
		   rsu_upnp_task_complete_t cb,
		   void *user_data);
void rsu_upnp_previous(rsu_upnp_t *upnp, rsu_task_t *task,
		       GCancellable *cancellable,
		       rsu_upnp_task_complete_t cb,
		       void *user_data);
void rsu_upnp_open_uri(rsu_upnp_t *upnp, rsu_task_t *task,
		       GCancellable *cancellable,
		       rsu_upnp_task_complete_t cb,
		       void *user_data);
void rsu_upnp_seek(rsu_upnp_t *upnp, rsu_task_t *task,
		   GCancellable *cancellable,
		   rsu_upnp_task_complete_t cb,
		   void *user_data);
void rsu_upnp_set_position(rsu_upnp_t *upnp, rsu_task_t *task,
			   GCancellable *cancellable,
			   rsu_upnp_task_complete_t cb,
			   void *user_data);
void rsu_upnp_host_uri(rsu_upnp_t *upnp, rsu_task_t *task,
		       GCancellable *cancellable,
		       rsu_upnp_task_complete_t cb,
		       void *user_data);
void rsu_upnp_remove_uri(rsu_upnp_t *upnp, rsu_task_t *task,
			 GCancellable *cancellable,
			 rsu_upnp_task_complete_t cb,
			 void *user_data);
void rsu_upnp_lost_client(rsu_upnp_t *upnp, const gchar *client_name);

#endif

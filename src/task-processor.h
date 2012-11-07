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
 * Regis Merlino <regis.merlino@intel.com>
 *
 */

#ifndef RSU_TASK_PROCESSOR_H__
#define RSU_TASK_PROCESSOR_H__

#include <gio/gio.h>
#include <glib.h>

#include "task-atom.h"

enum rsu_task_queue_flag_mask_ {
	RSU_TASK_QUEUE_FLAG_NONE = 0,
	RSU_TASK_QUEUE_FLAG_AUTO_START = 1,
	RSU_TASK_QUEUE_FLAG_AUTO_REMOVE = 1 << 1,
};
typedef enum rsu_task_queue_flag_mask_ rsu_task_queue_flag_mask;

typedef struct rsu_task_processor_t_ rsu_task_processor_t;

typedef void (*rsu_task_process_cb_t)(rsu_task_atom_t *task,
				      GCancellable **cancellable);
typedef void (*rsu_task_cancel_cb_t)(rsu_task_atom_t *task);
typedef void (*rsu_task_delete_cb_t)(rsu_task_atom_t *task);

rsu_task_processor_t *rsu_task_processor_new(GSourceFunc on_quit_cb);
void rsu_task_processor_free(rsu_task_processor_t *processor);
void rsu_task_processor_set_quitting(rsu_task_processor_t *processor);
const rsu_task_queue_key_t *rsu_task_processor_add_queue(
					rsu_task_processor_t *processor,
					const gchar *source,
					const gchar *sink,
					guint32 flags,
					rsu_task_process_cb_t task_process_cb,
					rsu_task_cancel_cb_t task_cancel_cb,
					rsu_task_delete_cb_t task_delete_cb);
const rsu_task_queue_key_t *rsu_task_processor_lookup_queue(
					const rsu_task_processor_t *processor,
					const gchar *source,
					const gchar *sink);
void rsu_task_processor_cancel_queue(const rsu_task_queue_key_t *queue_id);
void rsu_task_processor_remove_queues_for_source(
						rsu_task_processor_t *processor,
						const gchar *source);
void rsu_task_processor_remove_queues_for_sink(rsu_task_processor_t *processor,
					       const gchar *sink);

void rsu_task_queue_start(const rsu_task_queue_key_t *queue_id);
void rsu_task_queue_add_task(const rsu_task_queue_key_t *queue_id,
			     rsu_task_atom_t *task);
void rsu_task_queue_task_completed(const rsu_task_queue_key_t *queue_id);

#endif /* RSU_TASK_PROCESSOR_H__ */

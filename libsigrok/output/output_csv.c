/*
 * This file is part of the sigrok project.
 *
 * Copyright (C) 2011 Uwe Hermann <uwe@hermann-uwe.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <sigrok.h>
#include <sigrok-internal.h>
#include "config.h"

struct context {
	unsigned int num_enabled_probes;
	unsigned int unitsize;
	char *probelist[SR_MAX_NUM_PROBES + 1];
	uint64_t samplerate;
	GString *header;
	char separator;
};

/*
 * TODO:
 *  - Option to specify delimiter character and/or string.
 *  - Option to (not) print metadata as comments.
 *  - Option to specify the comment character(s), e.g. # or ; or C/C++-style.
 *  - Option to (not) print samplenumber / time as extra column.
 *  - Option to "compress" output (only print changed samples, VCD-like).
 *  - Option to print comma-separated bits, or whole bytes/words (for 8/16
 *    probe LAs) as ASCII/hex etc. etc.
 *  - Trigger support.
 */

static int init(struct sr_output *o)
{
	struct context *ctx;
	struct sr_probe *probe;
	GSList *l;
	int num_probes;
	uint64_t samplerate;
	time_t t;
	unsigned int i;

	if (!o) {
		sr_err("csv out: %s: o was NULL", __func__);
		return SR_ERR_ARG;
	}

	if (!o->device) {
		sr_err("csv out: %s: o->device was NULL", __func__);
		return SR_ERR_ARG;
	}

	if (!o->device->plugin) {
		sr_err("csv out: %s: o->device->plugin was NULL", __func__);
		return SR_ERR_ARG;
	}

	if (!(ctx = g_try_malloc0(sizeof(struct context)))) {
		sr_err("csv out: %s: ctx malloc failed", __func__);
		return SR_ERR_MALLOC;
	}

	o->internal = ctx;

	/* Get the number of probes, their names, and the unitsize. */
	/* TODO: Error handling. */
	for (l = o->device->probes; l; l = l->next) {
		probe = l->data;
		if (!probe->enabled)
			continue;
		ctx->probelist[ctx->num_enabled_probes++] = probe->name;
	}
	ctx->probelist[ctx->num_enabled_probes] = 0;
	ctx->unitsize = (ctx->num_enabled_probes + 7) / 8;

	num_probes = g_slist_length(o->device->probes);

	if (sr_device_has_hwcap(o->device, SR_HWCAP_SAMPLERATE)) {
		samplerate = *((uint64_t *) o->device->plugin->get_device_info(
				o->device->plugin_index, SR_DI_CUR_SAMPLERATE));
		/* TODO: Error checks. */
	} else {
		samplerate = 0; /* TODO: Error or set some value? */
	}
	ctx->samplerate = samplerate;

	ctx->separator = ',';

	ctx->header = g_string_sized_new(512);

	t = time(NULL);

	/* Some metadata */
	g_string_append_printf(ctx->header, "; CSV, generated by %s on %s",
			       PACKAGE_STRING, ctime(&t));
	g_string_append_printf(ctx->header, "; Samplerate: %"PRIu64"\n",
			       ctx->samplerate);

	/* Columns / channels */
	g_string_append_printf(ctx->header, "; Channels (%d/%d): ",
			       ctx->num_enabled_probes, num_probes);
	for (i = 0; i < ctx->num_enabled_probes; i++)
		g_string_append_printf(ctx->header, "%s, ", ctx->probelist[i]);
	g_string_append_printf(ctx->header, "\n");

	return 0; /* TODO: SR_OK? */
}

static int event(struct sr_output *o, int event_type, char **data_out,
		 uint64_t *length_out)
{
	struct context *ctx;

	if (!o) {
		sr_err("csv out: %s: o was NULL", __func__);
		return SR_ERR_ARG;
	}

	if (!(ctx = o->internal)) {
		sr_err("csv out: %s: o->internal was NULL", __func__);
		return SR_ERR_ARG;
	}

	if (!data_out) {
		sr_err("csv out: %s: data_out was NULL", __func__);
		return SR_ERR_ARG;
	}

	switch (event_type) {
	case SR_DF_TRIGGER:
		sr_dbg("csv out: %s: SR_DF_TRIGGER event", __func__);
		/* TODO */
		*data_out = NULL;
		*length_out = 0;
		break;
	case SR_DF_END:
		sr_dbg("csv out: %s: SR_DF_END event", __func__);
		/* TODO */
		*data_out = NULL;
		*length_out = 0;
		g_free(o->internal);
		o->internal = NULL;
		break;
	default:
		sr_err("csv out: %s: unsupported event type: %d", __func__,
		       event_type);
		*data_out = NULL;
		*length_out = 0;
		break;
	}

	return SR_OK;
}

static int data(struct sr_output *o, const char *data_in, uint64_t length_in,
		char **data_out, uint64_t *length_out)
{
	struct context *ctx;
	GString *outstr;
	uint64_t sample, i;
	int j;

	if (!o) {
		sr_err("csv out: %s: o was NULL", __func__);
		return SR_ERR_ARG;
	}

	if (!(ctx = o->internal)) {
		sr_err("csv out: %s: o->internal was NULL", __func__);
		return SR_ERR_ARG;
	}

	if (!data_in) {
		sr_err("csv out: %s: data_in was NULL", __func__);
		return SR_ERR_ARG;
	}

	if (ctx->header) {
		/* First data packet. */
		outstr = ctx->header;
		ctx->header = NULL;
	} else {
		outstr = g_string_sized_new(512);
	}

	for (i = 0; i <= length_in - ctx->unitsize; i += ctx->unitsize) {
		memcpy(&sample, data_in + i, ctx->unitsize);
		for (j = ctx->num_enabled_probes - 1; j >= 0; j--) {
			g_string_append_printf(outstr, "%d%c",
				(int)((sample & (1 << j)) >> j),
				ctx->separator);
		}
		g_string_append_printf(outstr, "\n");
	}

	*data_out = outstr->str;
	*length_out = outstr->len;
	g_string_free(outstr, FALSE);

	return SR_OK;
}

struct sr_output_format output_csv = {
	.id = "csv",
	.description = "Comma-separated values (CSV)",
	.df_type = SR_DF_LOGIC,
	.init = init,
	.data = data,
	.event = event,
};

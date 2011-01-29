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

/*
 * Output format for the OpenBench Logic Sniffer "Alternative" Java client.
 * Details: https://github.com/jawi/ols/wiki/OLS-data-file-format
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <sigrok.h>
#include "config.h"

struct context {
	unsigned int num_enabled_probes;
	unsigned int unitsize;
	char *probelist[MAX_NUM_PROBES + 1];
	char *header;
};

/* FIXME */
#define MAX_HEADER_LEN 2048

/* TODO: Support cursors? */
#define OLS_HEADER \
"# Generated by: %s on %s%s\n" \
"# Probe list used for capturing:\n" \
"# Number:\tName:\n" \
"%s" \
";Size: %" PRIu64 "\n" \
";Rate: %" PRIu64 "\n" \
";Channels: %d\n" \
";EnabledChannels: -1\n" \
";Compressed: true\n" \
";AbsoluteLength: %" PRIu64 "\n" \
";CursorEnabled: false\n" \
";Cursor0: -9223372036854775808\n" \
";Cursor1: -9223372036854775808\n" \
";Cursor2: -9223372036854775808\n" \
";Cursor3: -9223372036854775808\n" \
";Cursor4: -9223372036854775808\n" \
";Cursor5: -9223372036854775808\n" \
";Cursor6: -9223372036854775808\n" \
";Cursor7: -9223372036854775808\n" \
";Cursor8: -9223372036854775808\n" \
";Cursor9: -9223372036854775808\n"

const char *ols_header_comment = "\
# Comment: Acquisition with %d/%d probes at %s";

static int init(struct sr_output *o)
{
	struct context *ctx;
	struct probe *probe;
	GSList *l;
	uint64_t samplerate;
	unsigned int i;
	int b, num_probes;
	char *c, *frequency_s;
	char wbuf[1000], comment[128];
	time_t t;

	if (!(ctx = calloc(1, sizeof(struct context))))
		return SR_ERR_MALLOC;

	if (!(ctx->header = calloc(1, MAX_HEADER_LEN + 1))) {
		free(ctx);
		return SR_ERR_MALLOC;
	}

	o->internal = ctx;
	ctx->num_enabled_probes = 0;
	for (l = o->device->probes; l; l = l->next) {
		probe = l->data;
		if (!probe->enabled)
			continue;
		ctx->probelist[ctx->num_enabled_probes++] = probe->name;
	}
	ctx->probelist[ctx->num_enabled_probes] = 0;
	ctx->unitsize = (ctx->num_enabled_probes + 7) / 8;

	num_probes = g_slist_length(o->device->probes);
	comment[0] = '\0';

	/* TODO: Currently not available in the demo module. */

	if (o->device->plugin) {
		samplerate = *((uint64_t *) o->device->plugin->get_device_info(
				o->device->plugin_index, DI_CUR_SAMPLERATE));
		if (!(frequency_s = sigrok_samplerate_string(samplerate))) {
			free(ctx->header);
			free(ctx);
			return SR_ERR;
		}
		snprintf(comment, 127, ols_header_comment,
			 ctx->num_enabled_probes, num_probes, frequency_s);
		free(frequency_s);
	}

	/* Columns / channels */
	wbuf[0] = '\0';
	for (i = 0; i < ctx->num_enabled_probes; i++) {
		c = (char *)&wbuf + strlen((char *)&wbuf);
		sprintf(c, "# %d\t\t%s\n", i + 1, ctx->probelist[i]);
	}

	if (!(frequency_s = sigrok_period_string(samplerate))) {
		free(ctx->header);
		free(ctx);
		return SR_ERR;
	}

	t = time(NULL);

	/* TODO: Special handling for the demo driver. */
	/* TODO: Don't hardcode numsamples! */
	/* TODO: Check if num_enabled_probes/channels setting is correct. */
	b = snprintf(ctx->header, MAX_HEADER_LEN, OLS_HEADER,
		     PACKAGE_STRING, ctime(&t),
		     comment,
		     (const char *)&wbuf,
		     (uint64_t)10000 /* numsamples */,
		     samplerate,
		     ctx->num_enabled_probes,
		     (uint64_t)10000 /* numsamples */);

	/* TODO: Yield errors on stuff the OLS format doesn't support. */

	free(frequency_s);

	if (b < 0) {
		free(ctx->header);
		free(ctx);
		return SR_ERR;
	}

	return 0;
}

static int event(struct sr_output *o, int event_type, char **data_out,
		 uint64_t *length_out)
{
	struct context *ctx;

	ctx = o->internal;
	switch (event_type) {
	case DF_TRIGGER:
		/* TODO */
		break;
	case DF_END:
		free(o->internal);
		o->internal = NULL;
		break;
	}

	*data_out = NULL;
	*length_out = 0;

	return SR_OK;
}

static int data(struct sr_output *o, char *data_in, uint64_t length_in,
		char **data_out, uint64_t *length_out)
{
	struct context *ctx;
	unsigned int max_linelen, outsize, i;
	uint64_t sample;
	static uint64_t samplecount = 0;
	char *outbuf, *c;

	ctx = o->internal;
	max_linelen = 100; /* FIXME */
	outsize = length_in / ctx->unitsize * max_linelen;
	if (ctx->header)
		outsize += strlen(ctx->header);

	if (!(outbuf = calloc(1, outsize)))
		return SR_ERR_MALLOC;

	outbuf[0] = '\0';
	if (ctx->header) {
		/* The header is still here, this must be the first packet. */
		strncpy(outbuf, ctx->header, outsize);
		free(ctx->header);
		ctx->header = NULL;
	}

	for (i = 0; i <= length_in - ctx->unitsize; i += ctx->unitsize) {
		memcpy(&sample, data_in + i, ctx->unitsize);

		c = outbuf + strlen(outbuf);
		/* FIXME: OLS seems to only support 2^31 total samples? */
		sprintf(c, "%08x@%" PRIu64 "\n", (uint32_t)sample,
			samplecount++);
	}

	*data_out = outbuf;
	*length_out = strlen(outbuf);

	return SR_OK;
}

struct sr_output_format output_ols = {
	"ols",
	"OpenBench Logic Sniffer",
	DF_LOGIC,
	init,
	data,
	event,
};

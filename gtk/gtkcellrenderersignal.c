/*
 * This file is part of the sigrok project.
 *
 * Copyright (C) 2011 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>

#include "gtkcellrenderersignal.h"

enum
{
	PROP_0,
	PROP_DATA,
	PROP_PROBE,
	PROP_FOREGROUND,
};

struct _GtkCellRendererSignalPrivate
{
	GArray *data;
	guint32 probe;
	GdkColor foreground;
};

static void gtk_cell_renderer_signal_finalize(GObject *object);
static void gtk_cell_renderer_signal_get_property(GObject *object,
				guint param_id, GValue *value,
				GParamSpec *pspec);
static void gtk_cell_renderer_signal_set_property(GObject *object,
				guint param_id, const GValue *value,
				GParamSpec *pspec);
static void gtk_cell_renderer_signal_get_size(GtkCellRenderer *cell,
				GtkWidget *widget,
				const GdkRectangle *cell_area,
				gint *x_offset,
				gint *y_offset,
				gint *width,
				gint *height);
static void gtk_cell_renderer_signal_render(GtkCellRenderer *cell,
				GdkWindow *window,
				GtkWidget *widget,
				const GdkRectangle *background_area,
				const GdkRectangle *cell_area,
				GtkCellRendererState flags);


G_DEFINE_TYPE(GtkCellRendererSignal, gtk_cell_renderer_signal, GTK_TYPE_CELL_RENDERER);

static void
gtk_cell_renderer_signal_class_init (GtkCellRendererSignalClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);

	object_class->finalize = gtk_cell_renderer_signal_finalize;
	object_class->get_property = gtk_cell_renderer_signal_get_property;
	object_class->set_property = gtk_cell_renderer_signal_set_property;

	cell_class->get_size = gtk_cell_renderer_signal_get_size;
	cell_class->render = gtk_cell_renderer_signal_render;

	g_object_class_install_property(object_class,
				PROP_DATA,
				g_param_spec_pointer("data",
						"Data",
						"Binary samples data",
						G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
				PROP_PROBE,
				g_param_spec_int("probe",
						"Probe",
						"Bit in Data to display",
						-1, G_MAXINT, -1,
						G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
				PROP_FOREGROUND,
				g_param_spec_string("foreground",
						"Foreground",
						"Foreground Colour",
						NULL,
						G_PARAM_WRITABLE));

	g_type_class_add_private (object_class,
			sizeof (GtkCellRendererSignalPrivate));
}

static void gtk_cell_renderer_signal_init(GtkCellRendererSignal *cel)
{
	GtkCellRendererSignalPrivate *priv;

	cel->priv = G_TYPE_INSTANCE_GET_PRIVATE(cel,
				GTK_TYPE_CELL_RENDERER_SIGNAL,
				GtkCellRendererSignalPrivate);
	priv = cel->priv;

	priv->data = NULL;
	priv->probe = -1;
}

GtkCellRenderer *gtk_cell_renderer_signal_new(void)
{
	return g_object_new(GTK_TYPE_CELL_RENDERER_SIGNAL, NULL);
}

static void gtk_cell_renderer_signal_finalize(GObject *object)
{
	GtkCellRendererSignal *cel = GTK_CELL_RENDERER_SIGNAL(object);
	GtkCellRendererSignalPrivate *priv = cel->priv;

	G_OBJECT_CLASS(gtk_cell_renderer_signal_parent_class)->finalize(object);
}

static void
gtk_cell_renderer_signal_get_property(GObject *object,
				guint param_id,
				GValue *value,
				GParamSpec *pspec)
{
	GtkCellRendererSignal *cel = GTK_CELL_RENDERER_SIGNAL(object);
	GtkCellRendererSignalPrivate *priv = cel->priv;

	switch (param_id) {
	case PROP_DATA:
		g_value_set_pointer(value, priv->data);
		break;
	case PROP_PROBE:
		g_value_set_int (value, priv->probe);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	}
}

static void
gtk_cell_renderer_signal_set_property(GObject *object,
				guint param_id,
				const GValue *value,
				GParamSpec *pspec)
{
	GtkCellRendererSignal *cel = GTK_CELL_RENDERER_SIGNAL(object);
	GtkCellRendererSignalPrivate *priv = cel->priv;

	switch (param_id) {
	case PROP_DATA:
		priv->data = g_value_get_pointer(value);
		break;
	case PROP_PROBE:
		priv->probe = g_value_get_int(value);
		break;
	case PROP_FOREGROUND:
		gdk_color_parse(g_value_get_string(value), &priv->foreground);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	}
}

static void
gtk_cell_renderer_signal_get_size(GtkCellRenderer *cell,
				GtkWidget *widget,
				const GdkRectangle *cell_area,
				gint *x_offset,
				gint *y_offset,
				gint *width,
				gint *height)
{
	/* FIXME: What about cell_area? */
	if(width) *width = 0;
	if(height) *height = 30;

	if(x_offset) *x_offset = 0;
	if(y_offset) *y_offset = 0;
}


static void
gtk_cell_renderer_signal_render(GtkCellRenderer *cell,
				GdkWindow *window,
				GtkWidget *widget,
				const GdkRectangle *background_area,
				const GdkRectangle *cell_area,
				GtkCellRendererState flags)
{
	guint16 *tmp16;
	guint32 *tmp32;
	int i;
	GtkCellRendererSignal *cel = GTK_CELL_RENDERER_SIGNAL(cell);
	GtkCellRendererSignalPrivate *priv= cel->priv;
	guint nsamples = priv->data->len / g_array_get_element_size(priv->data);

	cairo_t *cr = gdk_cairo_create(GDK_DRAWABLE(window));
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
	gdk_cairo_set_source_color(cr, &priv->foreground);
	/*cairo_set_line_width(cr, 1);*/
	cairo_new_path(cr);

	switch(g_array_get_element_size(priv->data)) {
	case 1:
		for(i = 1; i < nsamples; i++) {
			cairo_line_to(cr, cell_area->x + i*5,
					cell_area->y + ((priv->data->data[i-1] & (1 << priv->probe))?0:cell_area->height));
			cairo_line_to(cr, cell_area->x + i*5,
					cell_area->y + ((priv->data->data[i] & (1 << priv->probe))?0:cell_area->height));
		}
		break;
	case 2:
		tmp16 = (guint16*)priv->data->data;
		for(i = 1; i < nsamples; i++) {
			cairo_line_to(cr, cell_area->x + i*5,
					cell_area->y + ((tmp16[i-1] & (1 << priv->probe))?0:cell_area->height));
			cairo_line_to(cr, cell_area->x + i*5,
					cell_area->y + ((tmp16[i] & (1 << priv->probe))?0:cell_area->height));
		}
		break;
	case 4:
		tmp32 = (guint32*)priv->data->data;
		for(i = 1; i < nsamples; i++) {
			cairo_line_to(cr, cell_area->x + i*5,
					cell_area->y + ((tmp32[i-1] & (1 << priv->probe))?0:cell_area->height));
			cairo_line_to(cr, cell_area->x + i*5,
					cell_area->y + ((tmp32[i] & (1 << priv->probe))?0:cell_area->height));
		}
		break;
	default:
		g_critical("Sample size not 8, 16 or 32 bits!");
	}
	cairo_stroke(cr);
}
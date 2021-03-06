/* Calf DSP Library
 * Custom controls (line graph, knob).
 * Copyright (C) 2007-2010 Krzysztof Foltman, Torben Hohn, Markus Schmidt
 * and others
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
#include "config.h"
#include <calf/giface.h>
#include <calf/custom_ctl.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <sys/time.h>
#include <iostream>

using namespace calf_plugins;
using namespace dsp;

///////////////////////////////////////// utility functions ///////////////////////////////////////////////

void line_graph_background(cairo_t* c, int x, int y, int sx, int sy, int ox, int oy, float brightness, int shadow, float lights, float dull) 
{
    float br = brightness * 0.5 + 0.5;

    // outer frame (black)
    int pad = 0;

    cairo_rectangle(
        c, pad + x, pad + y, sx + ox * 2 - pad * 2, sy + oy * 2 - pad * 2);
    cairo_set_source_rgb(c, 0, 0, 0);
    cairo_fill(c);

    // black light effect
    pad = 1;
    cairo_rectangle(
        c, pad + x, pad + y, sx + ox * 2 - pad * 2, sy + oy * 2 - pad * 2);
    cairo_pattern_t *pat2 = cairo_pattern_create_linear (
        x, y, x, y + sy + oy * 2 - pad * 2);
    cairo_pattern_add_color_stop_rgba (pat2, 0, 0.23, 0.23, 0.23, 1);
    cairo_pattern_add_color_stop_rgba (pat2, 0.33, 0.13, 0.13, 0.13, 1);
    cairo_pattern_add_color_stop_rgba (pat2, 0.33, 0.05, 0.05, 0.05, 1);
    cairo_pattern_add_color_stop_rgba (pat2, 0.5, 0, 0, 0, 1);
    cairo_set_source (c, pat2);
    cairo_fill(c);
    cairo_pattern_destroy(pat2);

    cairo_rectangle(c, x + ox - 1, y + oy - 1, sx + 2, sy + 2);
    cairo_set_source_rgb (c, 0, 0, 0);
    cairo_fill(c);

    // inner yellowish screen
    cairo_pattern_t *pt = cairo_pattern_create_linear(x + ox, y + oy, x + ox, y + sy);
    cairo_pattern_add_color_stop_rgb(pt, 0.0, br * 0.71, br * 0.82, br * 0.33);
    cairo_pattern_add_color_stop_rgb(pt, 1.0, br * 0.89, br * 1.00, br * 0.54);
    cairo_set_source (c, pt);
    cairo_rectangle(c, x + ox, y + oy, sx, sy);
    cairo_fill(c);
    cairo_pattern_destroy(pt);

    if (shadow) {
        // top shadow
        pt = cairo_pattern_create_linear(x + ox, y + oy, x + ox, y + oy + shadow);
        cairo_pattern_add_color_stop_rgba(pt, 0.0, 0,0,0,0.6);
        cairo_pattern_add_color_stop_rgba(pt, 1.0, 0,0,0,0);
        cairo_set_source (c, pt);
        cairo_rectangle(c, x + ox, y + oy, sx, shadow);
        cairo_fill(c);
        cairo_pattern_destroy(pt);

        // left shadow
        pt = cairo_pattern_create_linear(x + ox, y + oy, x + ox + (float)shadow * 0.7, y + oy);
        cairo_pattern_add_color_stop_rgba(pt, 0.0, 0,0,0,0.3);
        cairo_pattern_add_color_stop_rgba(pt, 1.0, 0,0,0,0);
        cairo_set_source (c, pt);
        cairo_rectangle(c, x + ox, y + oy, (float)shadow * 0.7, sy);
        cairo_fill(c);
        cairo_pattern_destroy(pt);

        // right shadow
        pt = cairo_pattern_create_linear(x + ox + sx - (float)shadow * 0.7, y + oy, x + ox + sx, y + oy);
        cairo_pattern_add_color_stop_rgba(pt, 0.0, 0,0,0,0);
        cairo_pattern_add_color_stop_rgba(pt, 1.0, 0,0,0,0.3);
        cairo_set_source (c, pt);
        cairo_rectangle(c, x + ox + sx - (float)shadow * 0.7, y + oy, 5, sy);
        cairo_fill(c);
        cairo_pattern_destroy(pt);
    }

    if(dull) {
        // left dull
        pt = cairo_pattern_create_linear(x + ox, y + oy, x + ox + sx / 2, y + oy);
        cairo_pattern_add_color_stop_rgba(pt, 0.0, 0,0,0,dull);
        cairo_pattern_add_color_stop_rgba(pt, 1.0, 0,0,0,0);
        cairo_set_source (c, pt);
        cairo_rectangle(c, x + ox, y + oy, sx / 2, sy);
        cairo_fill(c);
        cairo_pattern_destroy(pt);

        // right dull
        pt = cairo_pattern_create_linear(x + ox + sx / 2, y + oy, x + ox + sx, y + oy);
        cairo_pattern_add_color_stop_rgba(pt, 0.0, 0,0,0,0);
        cairo_pattern_add_color_stop_rgba(pt, 1.0, 0,0,0,dull);
        cairo_set_source (c, pt);
        cairo_rectangle(c, x + ox + sx / 2, y + oy, sx / 2, sy);
        cairo_fill(c);
        cairo_pattern_destroy(pt);
    }

    if(lights > 0) {
        // light sources
        int div = 1;
        int light_w = sx;
        while(light_w / div > 300)
            div += 1;
        int w = sx / div;
        cairo_rectangle(c, x + ox, y + oy, sx, sy);
        for(int i = 0; i < div; i ++) {
            cairo_pattern_t *pt = cairo_pattern_create_radial(
               x + ox + w * i + w / 2.f, y + oy, 1,
               x + ox + w * i + w / 2.f, y + ox + sy * 0.25, w / 2.f);
            cairo_pattern_add_color_stop_rgba (pt, 0, 1, 1, 0.8, lights);
            cairo_pattern_add_color_stop_rgba (pt, 1, 0.89, 1.00, 0.45, 0);
            cairo_set_source (c, pt);
            cairo_fill_preserve(c);
            pt = cairo_pattern_create_radial(
               x + ox + w * i + w / 2.f, y + oy + sy, 1,
               x + ox + w * i + w / 2.f, y + ox + sy * 0.75, w / 2.f);
            cairo_pattern_add_color_stop_rgba (pt, 0, 1, 1, 0.8, lights);
            cairo_pattern_add_color_stop_rgba (pt, 1, 0.89, 1.00, 0.45, 0);
            cairo_set_source (c, pt);
            cairo_fill_preserve(c);
        }
    }
}

///////////////////////////////////////// phase graph ///////////////////////////////////////////////

static void
calf_phase_graph_draw_background( cairo_t *ctx, int sx, int sy, int ox, int oy )
{
    int cx = ox + sx / 2;
    int cy = oy + sy / 2;
    
    line_graph_background(ctx, 0, 0, sx, sy, ox, oy);
    cairo_set_source_rgb(ctx, 0.35, 0.4, 0.2);
    cairo_select_font_face(ctx, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(ctx, 9);
    cairo_text_extents_t te;
    
    cairo_text_extents (ctx, "M", &te);
    cairo_move_to (ctx, cx + 5, oy + 12);
    cairo_show_text (ctx, "M");
    
    cairo_text_extents (ctx, "S", &te);
    cairo_move_to (ctx, ox + 5, cy - 5);
    cairo_show_text (ctx, "S");
    
    cairo_text_extents (ctx, "L", &te);
    cairo_move_to (ctx, ox + 18, oy + 12);
    cairo_show_text (ctx, "L");
    
    cairo_text_extents (ctx, "R", &te);
    cairo_move_to (ctx, ox + sx - 22, oy + 12);
    cairo_show_text (ctx, "R");
    
    cairo_set_line_width(ctx, 1);
    
    cairo_move_to(ctx, ox, oy + sy * 0.5);
    cairo_line_to(ctx, ox + sx, oy + sy * 0.5);
    cairo_stroke(ctx);
    
    cairo_move_to(ctx, ox + sx * 0.5, oy);
    cairo_line_to(ctx, ox + sx * 0.5, oy + sy);
    cairo_stroke(ctx);
    
    cairo_set_source_rgba(ctx, 0, 0, 0, 0.2);
    cairo_move_to(ctx, ox, oy);
    cairo_line_to(ctx, ox + sx, oy + sy);
    cairo_stroke(ctx);
    
    cairo_move_to(ctx, ox, oy + sy);
    cairo_line_to(ctx, ox + sx, oy);
    cairo_stroke(ctx);
}
static void
calf_phase_graph_copy_surface(cairo_t *ctx, cairo_surface_t *source, float fade = 1.f)
{
    // copy a surface to a cairo context
    cairo_save(ctx);
    cairo_set_source_surface(ctx, source, 0, 0);
    if (fade < 1.0) {
        float rnd = (float)rand() / (float)RAND_MAX / 100;
        cairo_paint_with_alpha(ctx, fade * 0.35 + 0.05 + rnd);
    } else {
        cairo_paint(ctx);
    }
    cairo_restore(ctx);
}
static gboolean
calf_phase_graph_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_PHASE_GRAPH(widget));
    CalfPhaseGraph *pg = CALF_PHASE_GRAPH(widget);
    if (!pg->source) 
        return FALSE;
    
    // dimensions
    int ox = 5, oy = 5;
    int sx = widget->allocation.width - ox * 2, sy = widget->allocation.height - oy * 2;
    sx += sx % 2 - 1;
    sy += sy % 2 - 1;
    int rad = sx / 2;
    int cx = ox + sx / 2;
    int cy = oy + sy / 2;
    
    // some values as pointers for the audio plug-in call
    std::string legend;
    float *data = new float[2 * sx];
    float * phase_buffer = 0;
    int length = 0;
    int mode = 2;
    float fade = 0.05;
    bool use_fade = true;
    int accuracy = 1;
    bool display = true;
    
    // cairo initialization stuff
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));
    cairo_t *ctx_back;
    cairo_t *ctx_cache;
    
    if( pg->background == NULL ) {
        // looks like its either first call or the widget has been resized.
        // create the background surface (stolen from line graph)...
        cairo_surface_t *window_surface = cairo_get_target(c);
        pg->background = cairo_surface_create_similar(window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );
        pg->cache = cairo_surface_create_similar(window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );
        
        // ...and draw some bling bling onto it...
        ctx_back = cairo_create(pg->background);
        calf_phase_graph_draw_background(ctx_back, sx, sy, ox, oy);
        // ...and copy it to the cache
        ctx_cache = cairo_create(pg->cache); 
        calf_phase_graph_copy_surface(ctx_cache, pg->background, 1);
    } else {
        ctx_back = cairo_create(pg->background);
        ctx_cache = cairo_create(pg->cache); 
    }
    
    pg->source->get_phase_graph(&phase_buffer, &length, &mode, &use_fade,
                                &fade, &accuracy, &display);
    
    // process some values set by the plug-in
    accuracy *= 2;
    accuracy = 12 - accuracy;
    
    calf_phase_graph_copy_surface(ctx_cache, pg->background, use_fade ? fade : 1);
    
    if(display) {
        cairo_rectangle(ctx_cache, ox, oy, sx, sy);
        cairo_clip(ctx_cache);
        cairo_set_source_rgba(ctx_cache, 0.35, 0.4, 0.2, 1);
        double _a;
        for(int i = 0; i < length; i+= accuracy) {
            float l = phase_buffer[i];
            float r = phase_buffer[i + 1];
            if(l == 0.f and r == 0.f) continue;
            else if(r == 0.f and l > 0.f) _a = M_PI / 2.f;
            else if(r == 0.f and l < 0.f) _a = 3.f *M_PI / 2.f;
            else _a = pg->_atan(l / r, l, r);
            double _R = sqrt(pow(l, 2) + pow(r, 2));
            _a += M_PI / 4.f;
            float x = (-1.f)*_R * cos(_a);
            float y = _R * sin(_a);
            // mask the cached values
            switch(mode) {
                case 0:
                    // small dots
                    cairo_rectangle (ctx_cache, x * rad + cx, y * rad + cy, 1, 1);
                    break;
                case 1:
                    // medium dots
                    cairo_rectangle (ctx_cache, x * rad + cx - 0.25, y * rad + cy - 0.25, 1.5, 1.5);
                    break;
                case 2:
                    // big dots
                    cairo_rectangle (ctx_cache, x * rad + cx - 0.5, y * rad + cy - 0.5, 2, 2);
                    break;
                case 3:
                    // fields
                    if(i == 0) cairo_move_to(ctx_cache,
                        x * rad + cx, y * rad + cy);
                    else cairo_line_to(ctx_cache,
                        x * rad + cx, y * rad + cy);
                    break;
                case 4:
                    // lines
                    if(i == 0) cairo_move_to(ctx_cache,
                        x * rad + cx, y * rad + cy);
                    else cairo_line_to(ctx_cache,
                        x * rad + cx, y * rad + cy);
                    break;
            }
        }
        // draw
        switch(mode) {
            case 0:
            case 1:
            case 2:
                cairo_fill(ctx_cache);
                break;
            case 3:
                cairo_fill(ctx_cache);
                break;
            case 4:
                cairo_set_line_width(ctx_cache, 0.5);
                cairo_stroke(ctx_cache);
                break;
        }
    }
    
    calf_phase_graph_copy_surface(c, pg->cache, 1);
    
    cairo_destroy(c);
    cairo_destroy(ctx_back);
    cairo_destroy(ctx_cache);
    delete []data;
    // printf("exposed %p %dx%d %d+%d\n", widget->window, event->area.x, event->area.y, event->area.width, event->area.height);
    return TRUE;
}

static void
calf_phase_graph_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_PHASE_GRAPH(widget));
    // CalfLineGraph *lg = CALF_LINE_GRAPH(widget);
}

static void
calf_phase_graph_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_PHASE_GRAPH(widget));
    CalfPhaseGraph *lg = CALF_PHASE_GRAPH(widget);

    GtkWidgetClass *parent_class = (GtkWidgetClass *) g_type_class_peek_parent( CALF_PHASE_GRAPH_GET_CLASS( lg ) );

    if(lg->background)
        cairo_surface_destroy(lg->background);
    lg->background = NULL;
    
    widget->allocation = *allocation;
    GtkAllocation &a = widget->allocation;
    if (a.width > a.height) {
        a.x += (a.width - a.height) / 2;
        a.width = a.height;
    }
    if (a.width < a.height) {
        a.y += (a.height - a.width) / 2;
        a.height = a.width;
    }
    parent_class->size_allocate(widget, &a);
}

static void
calf_phase_graph_class_init (CalfPhaseGraphClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_phase_graph_expose;
    widget_class->size_request = calf_phase_graph_size_request;
    widget_class->size_allocate = calf_phase_graph_size_allocate;
}

static void
calf_phase_graph_unrealize (GtkWidget *widget, CalfPhaseGraph *pg)
{
    if( pg->background )
        cairo_surface_destroy(pg->background);
    pg->background = NULL;
}

static void
calf_phase_graph_init (CalfPhaseGraph *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 40;
    self->background = NULL;
    g_signal_connect(GTK_OBJECT(widget), "unrealize", G_CALLBACK(calf_phase_graph_unrealize), (gpointer)self);
}

GtkWidget *
calf_phase_graph_new()
{
    return GTK_WIDGET(g_object_new (CALF_TYPE_PHASE_GRAPH, NULL));
}

GType
calf_phase_graph_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfPhaseGraphClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_phase_graph_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfPhaseGraph),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_phase_graph_init
        };

        GTypeInfo *type_info_copy = new GTypeInfo(type_info);

        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfPhaseGraph%u%d", ((unsigned int)(intptr_t)calf_phase_graph_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static( GTK_TYPE_DRAWING_AREA,
                                           name,
                                           type_info_copy,
                                           (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}


///////////////////////////////////////// toggle ///////////////////////////////////////////////

static gboolean
calf_toggle_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_TOGGLE(widget));
    
    CalfToggle *self = CALF_TOGGLE(widget);
    
    float sx = self->size ? self->size : 1.f / 3.f * 2.f;
    float sy = self->size ? self->size : 1;
    int x = widget->allocation.x + widget->allocation.width / 2 - sx * 15 - sx * 2;
    int y = widget->allocation.y + widget->allocation.height / 2 - sy * 10 - sy * 3;
    int width  = sx * 34;
    int height = sy * 26;
    
    gdk_draw_pixbuf(GDK_DRAWABLE(widget->window),
                    widget->style->fg_gc[0],
                    self->toggle_image[self->size],
                    20 - sx * 2,
                    20 - sy * 3 + (sy * 20 + 40) * floor(.5 + gtk_range_get_value(GTK_RANGE(widget))),
                    x,
                    y,
                    width,
                    height,
                    GDK_RGB_DITHER_NORMAL, 0, 0);
    return TRUE;
}

static void
calf_toggle_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_TOGGLE(widget));

    CalfToggle *self = CALF_TOGGLE(widget);
    
    float sx = self->size ? self->size : 1.f / 3.f * 2.f;
    float sy = self->size ? self->size : 1;
    
    requisition->width  = 30 * sx;
    requisition->height = 20 * sy;
}

static gboolean
calf_toggle_button_press (GtkWidget *widget, GdkEventButton *event)
{
    g_assert(CALF_IS_TOGGLE(widget));
    GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(widget));
    if (gtk_range_get_value(GTK_RANGE(widget)) == adj->lower)
    {
        gtk_range_set_value(GTK_RANGE(widget), adj->upper);
    } else {
        gtk_range_set_value(GTK_RANGE(widget), adj->lower);
    }
    return TRUE;
}

static gboolean
calf_toggle_key_press (GtkWidget *widget, GdkEventKey *event)
{
    switch(event->keyval)
    {
        case GDK_Return:
        case GDK_KP_Enter:
        case GDK_space:
            return calf_toggle_button_press(widget, NULL);
    }
    return FALSE;
}

static void
calf_toggle_class_init (CalfToggleClass *klass)
{
    // GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_toggle_expose;
    widget_class->size_request = calf_toggle_size_request;
    widget_class->button_press_event = calf_toggle_button_press;
    widget_class->key_press_event = calf_toggle_key_press;
}

static void
calf_toggle_init (CalfToggle *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET(self), GTK_CAN_FOCUS);
    widget->requisition.width = 30;
    widget->requisition.height = 20;
    self->size = 1;
    GError *error = NULL;
    self->toggle_image[0] = gdk_pixbuf_new_from_file(PKGLIBDIR "/toggle0_silver.png", &error);
    self->toggle_image[1] = gdk_pixbuf_new_from_file(PKGLIBDIR "/toggle1_silver.png", &error);
    self->toggle_image[2] = gdk_pixbuf_new_from_file(PKGLIBDIR "/toggle2_silver.png", &error);
    g_assert(self->toggle_image != NULL);
}

GtkWidget *
calf_toggle_new()
{
    GtkAdjustment *adj = (GtkAdjustment *)gtk_adjustment_new(0, 0, 1, 1, 0, 0);
    return calf_toggle_new_with_adjustment(adj);
}

static gboolean calf_toggle_value_changed(gpointer obj)
{
    GtkWidget *widget = (GtkWidget *)obj;
    CalfToggle *self = CALF_TOGGLE(widget);
    float sx = self->size ? self->size : 1.f / 3.f * 2.f;
    float sy = self->size ? self->size : 1;
    gtk_widget_queue_draw_area(widget,
                               widget->allocation.x - sx * 2,
                               widget->allocation.y - sy * 3,
                               self->size * 34,
                               self->size * 26);
    return FALSE;
}

GtkWidget *calf_toggle_new_with_adjustment(GtkAdjustment *_adjustment)
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_TOGGLE, NULL ));
    if (widget) {
        gtk_range_set_adjustment(GTK_RANGE(widget), _adjustment);
        g_signal_connect(GTK_OBJECT(widget), "value-changed", G_CALLBACK(calf_toggle_value_changed), widget);
    }
    return widget;
}

GType
calf_toggle_get_type (void)
{
    static GType type = 0;
    if (!type) {
        
        static const GTypeInfo type_info = {
            sizeof(CalfToggleClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_toggle_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfToggle),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_toggle_init
        };
        
        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfToggle%u%d", 
                ((unsigned int)(intptr_t)calf_toggle_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static( GTK_TYPE_RANGE,
                                           name,
                                           &type_info,
                                           (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}

///////////////////////////////////////// frame ///////////////////////////////////////////////


GtkWidget *
calf_frame_new(const char *label)
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_FRAME, NULL ));
    CalfFrame *self = CALF_FRAME(widget);
    gtk_frame_set_label(GTK_FRAME(self), label);
    return widget;
}
static gboolean
calf_frame_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_FRAME(widget));
    if (gtk_widget_is_drawable (widget)) {
        
        GdkWindow *window = widget->window;
        cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(window));
        cairo_text_extents_t extents;
        
        int ox = widget->allocation.x;
        int oy = widget->allocation.y;
        int sx = widget->allocation.width;
        int sy = widget->allocation.height;
        
        double rad  = 8;
        double a    = 1.5;
        double pad  = 4;
        double txp  = 4;
        double m    = 1;
        double size = 10;
        
        cairo_rectangle(c, ox, oy, sx, sy);
        cairo_clip(c);
        
        
        const gchar *lab = gtk_frame_get_label(GTK_FRAME(widget));
        
        cairo_select_font_face(c, "Sans",
              CAIRO_FONT_SLANT_NORMAL,
              CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(c, size);
        
        cairo_text_extents(c, lab, &extents);
        
        double lw = extents.width + txp * 2.;
        
        cairo_set_line_width(c, 1.);
        
        cairo_move_to(c, ox + rad + txp + m, oy + size - 2 + m);
        cairo_set_source_rgb(c, 0.99,0.99,0.99);
        cairo_show_text(c, lab);
        
        cairo_set_source_rgb(c, 0.9,0.9,0.9);
        
        rad = 8;
        cairo_move_to(c, ox + a + m, oy + pad + rad + a + m);
        cairo_arc (c, ox + rad + a + m, oy + rad + a + pad + m, rad, 1 * M_PI, 1.5 * M_PI);
        cairo_move_to(c, ox + rad + a + lw + m, oy + a + pad + m);
        cairo_line_to(c, ox + sx + a - rad - m - 1, oy + a + pad + m);
        cairo_arc (c, ox + sx - rad + a - 2*m - 1, oy + rad + a + pad + m, rad, 1.5 * M_PI, 2 * M_PI);
        cairo_line_to(c, ox + sx + a - 2*m - 1, oy + a + sy - rad - 2*m - 1);
        rad = 9;
        cairo_arc (c, ox + sx - rad + a - 2*m - 1, oy + sy - rad + a - 2*m - 1, rad, 0 * M_PI, 0.5 * M_PI);
        rad = 8;
        cairo_line_to(c, ox + a + rad + m, oy + sy + a - 2*m - 1);
        cairo_arc (c, ox + rad + a + m, oy + sy - rad + a - 2*m - 1, rad, 0.5 * M_PI, 1 * M_PI);
        cairo_line_to(c, ox + a + m, oy + a + rad + pad + m);
        cairo_stroke(c);
        
        a = 0.5;
        
        cairo_set_source_rgb(c, 0.66,0.66,0.66);
        
        rad = 9;
        cairo_move_to(c, ox + a + m, oy + pad + rad + a + m);
        cairo_arc (c, ox + rad + a + m, oy + rad + a + pad + m, rad, 1 * M_PI, 1.5 * M_PI);
        cairo_move_to(c, ox + rad + a + lw + m, oy + a + pad + m);
        rad = 8;
        cairo_line_to(c, ox + sx + a - rad - m, oy + a + pad + m);
        cairo_arc (c, ox + sx - rad + a - 2*m - 1, oy + rad + a + pad + m, rad, 1.5 * M_PI, 2 * M_PI);
        cairo_line_to(c, ox + sx + a - 2*m - 1, oy + a + sy - rad - 2*m);
        cairo_arc (c, ox + sx - rad + a - 2*m - 1, oy + sy - rad + a - 2*m - 1, rad, 0 * M_PI, 0.5 * M_PI);
        cairo_line_to(c, ox + a + rad + m, oy + sy + a - 2*m - 1);
        cairo_arc (c, ox + rad + a + m, oy + sy - rad + a - 2*m - 1, rad, 0.5 * M_PI, 1 * M_PI);
        cairo_line_to(c, ox + a + m, oy + a + rad + pad + m);
        cairo_stroke(c);
        
        cairo_destroy(c);
    }
    if (gtk_bin_get_child(GTK_BIN(widget))) {
        gtk_container_propagate_expose(GTK_CONTAINER(widget),
                                       gtk_bin_get_child(GTK_BIN(widget)),
                                       event);
    }
    return FALSE;
}

static void
calf_frame_class_init (CalfFrameClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_frame_expose;
}

static void
calf_frame_init (CalfFrame *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 40;
}

GType
calf_frame_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfFrameClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_frame_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfFrame),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_frame_init
        };

        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfFrame%u%d", 
                ((unsigned int)(intptr_t)calf_frame_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_FRAME,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}

///////////////////////////////////////// combo box ///////////////////////////////////////////////


GtkWidget *
calf_combobox_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_COMBOBOX, NULL ));
    GtkCellRenderer *column = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), column, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget), column,
                                   "text", 0,
                                   NULL);
    return widget;
}
static gboolean
calf_combobox_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_COMBOBOX(widget));
    
    if (gtk_widget_is_drawable (widget)) {
        
        int pad = 4;
        
        GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX (widget));
        GtkTreeIter iter;
        gchar *lab;
        if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter))
            gtk_tree_model_get (model, &iter, 0, &lab, -1);
        else
            lab = g_strdup("");
        
        GdkWindow *window = widget->window;
        cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(window));
        
        int x  = widget->allocation.x;
        int y  = widget->allocation.y;
        int sx = widget->allocation.width;
        int sy = widget->allocation.height;
        gint mx, my;
        bool hover = false;
        
        cairo_rectangle(c, x, y, sx, sy);
        cairo_clip(c);
        
        gtk_widget_get_pointer(GTK_WIDGET(widget), &mx, &my);
        if (mx >= 0 and mx < sx and my >= 0 and my < sy)
            hover = true;
            
        line_graph_background(c, x, y, sx - pad * 2, sy - pad * 2, pad, pad, g_ascii_isspace(lab[0]) ? 0 : 1, 4, hover ? 0.5 : 0, hover ? 0.1 : 0.25);
        
        cairo_select_font_face(c, "Sans",
              CAIRO_FONT_SLANT_NORMAL,
              CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(c, 12);
        
        cairo_move_to(c, x + pad + 3, y + sy / 2 + 5);
        cairo_set_source_rgb(c, 0.,0.,0.);
        cairo_show_text(c, lab);
        g_free(lab);
        
        cairo_surface_t *image;
        image = cairo_image_surface_create_from_png(PKGLIBDIR "combo_arrow.png");
        cairo_set_source_surface(c, image, x + sx - 20, y + sy / 2 - 5);
        cairo_rectangle(c, x, y, sx, sy);
        cairo_fill(c);
        
        cairo_destroy(c);
    }
    return FALSE;
}

static void
calf_combobox_class_init (CalfComboboxClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_combobox_expose;
}

static void
calf_combobox_init (CalfCombobox *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 20;
}

GType
calf_combobox_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfComboboxClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_combobox_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfCombobox),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_combobox_init
        };

        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfCombobox%u%d", 
                ((unsigned int)(intptr_t)calf_combobox_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_COMBO_BOX,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}


///////////////////////////////////////// notebook ///////////////////////////////////////////////

#define GTK_NOTEBOOK_PAGE(_glist_)         ((GtkNotebookPage *)((GList *)(_glist_))->data)
struct _GtkNotebookPage
{
  GtkWidget *child;
  GtkWidget *tab_label;
  GtkWidget *menu_label;
  GtkWidget *last_focus_child;  /* Last descendant of the page that had focus */

  guint default_menu : 1;   /* If true, we create the menu label ourself */
  guint default_tab  : 1;   /* If true, we create the tab label ourself */
  guint expand       : 1;
  guint fill         : 1;
  guint pack         : 1;
  guint reorderable  : 1;
  guint detachable   : 1;

  /* if true, the tab label was visible on last allocation; we track this so
   * that we know to redraw the tab area if a tab label was hidden then shown
   * without changing position */
  guint tab_allocated_visible : 1;

  GtkRequisition requisition;
  GtkAllocation allocation;

  gulong mnemonic_activate_signal;
  gulong notify_visible_handler;
};

GtkWidget *
calf_notebook_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_NOTEBOOK, NULL ));
    return widget;
}
static gboolean
calf_notebook_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_NOTEBOOK(widget));
    
    GtkNotebook *notebook;
    notebook = GTK_NOTEBOOK (widget);
    CalfNotebookClass *klass = CALF_NOTEBOOK_CLASS(GTK_OBJECT_GET_CLASS(widget));
    
    if (gtk_widget_is_drawable (widget)) {
        
        GdkWindow *window = widget->window;
        cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(window));
        cairo_pattern_t *pat = NULL;
        
        int x  = widget->allocation.x;
        int y  = widget->allocation.y;
        int sx = widget->allocation.width;
        int sy = widget->allocation.height;
        int tx = widget->style->xthickness;
        int ty = widget->style->ythickness;
        int lh = 19;
        int bh = lh + 2 * ty;
        
        cairo_rectangle(c, x, y, sx, sy);
        cairo_clip(c);
        
        int add = 0;
        
        if (notebook->show_tabs) {
            GtkNotebookPage *page;
            GList *pages;
            
            gint sp;
            gtk_widget_style_get(widget, "tab-overlap", &sp, NULL);
            
            pages = notebook->children;
            
            int cn = 0;
            while (pages) {
                page = GTK_NOTEBOOK_PAGE (pages);
                pages = pages->next;
                if (page->tab_label->window == event->window &&
                    gtk_widget_is_drawable (page->tab_label)) {
                    int lx = page->tab_label->allocation.x;
                    int lw = page->tab_label->allocation.width;
                    
                    // fix the labels position
                    page->tab_label->allocation.y = y + ty;
                    page->tab_label->allocation.height = lh;
                    
                    // draw tab background
                    cairo_rectangle(c, lx - tx, y, lw + 2 * tx, bh);
                    cairo_set_source_rgba(c, 0,0,0, page != notebook->cur_page ? 0.25 : 0.5);
                    cairo_fill(c);
                    
                    if (page == notebook->cur_page) {
                        // draw tab light
                        cairo_rectangle(c, lx - tx + 2, y + 2, lw + 2 * tx - 4, 2);
                        pat = cairo_pattern_create_radial(lx + lw / 2, y + bh / 2, 1, lx + lw / 2, y + bh / 2, lw + tx * 2);
                        cairo_pattern_add_color_stop_rgb(pat, 0,   50. / 255, 1, 1);
                        cairo_pattern_add_color_stop_rgb(pat, 0.3,  2. / 255, 180. / 255, 1);
                        cairo_pattern_add_color_stop_rgb(pat, 0.5, 19. / 255, 220. / 255, 1);
                        cairo_pattern_add_color_stop_rgb(pat, 1,    2. / 255, 120. / 255, 1);
                        cairo_set_source(c, pat);
                        cairo_fill(c);
                        
                        cairo_rectangle(c, lx - tx + 2, y + 1, lw + 2 * tx - 4, 1);
                        cairo_set_source_rgba(c, 0,0,0,0.5);
                        cairo_fill(c);
                        
                        cairo_rectangle(c, lx - tx + 2, y + 4, lw + 2 * tx - 4, 1);
                        cairo_set_source_rgba(c, 1,1,1,0.3);
                        cairo_fill(c);
                    
                    }
                    // draw labels
                    gtk_container_propagate_expose (GTK_CONTAINER (notebook), page->tab_label, event);
                }
                cn++;
            }
            add = bh;
        }
        
        // draw main body
        cairo_rectangle(c, x, y + add, sx, sy - add);
        cairo_set_source_rgba(c, 0,0,0,0.5);
        cairo_fill(c);
        
        // draw frame
        cairo_rectangle(c, x + 0.5, y + add + 0.5, sx - 1, sy - add - 1);
        pat = cairo_pattern_create_linear(x, y + add, x, y + sy - add);
        cairo_pattern_add_color_stop_rgba(pat,   0,   0,   0,   0, 0.3);
        cairo_pattern_add_color_stop_rgba(pat, 0.5, 0.5, 0.5, 0.5,   0);
        cairo_pattern_add_color_stop_rgba(pat,   1,   1,   1,   1, 0.2);
        cairo_set_source (c, pat);
        cairo_set_line_width(c, 1);
        cairo_stroke_preserve(c);
                    
        int sw = gdk_pixbuf_get_width(klass->screw);
        int sh = gdk_pixbuf_get_height(klass->screw);
        
        // draw screws
        gdk_cairo_set_source_pixbuf(c, klass->screw, x, y + add);
        cairo_fill_preserve(c);
        gdk_cairo_set_source_pixbuf(c, klass->screw, x + sx - sw, y + add);
        cairo_fill_preserve(c);
        gdk_cairo_set_source_pixbuf(c, klass->screw, x, y + sy - sh);
        cairo_fill_preserve(c);
        gdk_cairo_set_source_pixbuf(c, klass->screw, x + sx - sh, y + sy - sh);
        cairo_fill_preserve(c);
        
        // propagate expose to all children
        if (notebook->cur_page)
            gtk_container_propagate_expose (GTK_CONTAINER (notebook),
                notebook->cur_page->child,
                event);
        
        cairo_pattern_destroy(pat);
        cairo_destroy(c);
        
    }
    return FALSE;
}

static void
calf_notebook_class_init (CalfNotebookClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_notebook_expose;
    
    klass->screw = gdk_pixbuf_new_from_file (PKGLIBDIR "screw_black.png", 0);
}

static void
calf_notebook_init (CalfNotebook *self)
{
    //GtkWidget *widget = GTK_WIDGET(self);
}

GType
calf_notebook_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfNotebookClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_notebook_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfNotebook),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_notebook_init
        };

        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfNotebook%u%d", 
                ((unsigned int)(intptr_t)calf_notebook_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_NOTEBOOK,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}

///////////////////////////////////////// fader ///////////////////////////////////////////////

static void calf_fader_set_layout(GtkWidget *widget)
{
    GtkRange  *range        = GTK_RANGE(widget);
    CalfFader *fader        = CALF_FADER(widget);
    CalfFaderLayout layout  = fader->layout;
    
    int hor = fader->horizontal;
    
    // widget layout
    layout.x = widget->allocation.x;
    layout.y = widget->allocation.y;
    layout.w = widget->allocation.width;
    layout.h = widget->allocation.height;
    
    // trough layout
    layout.tx = range->range_rect.x + layout.x;
    layout.ty = range->range_rect.y + layout.y;
    layout.tw = range->range_rect.width;
    layout.th = range->range_rect.height;
    layout.tc = hor ? layout.ty + layout.th / 2 : layout.tx + layout.tw / 2; 
    
    // screw layout
    layout.scw  = gdk_pixbuf_get_width(fader->screw);
    layout.sch  = gdk_pixbuf_get_height(fader->screw);
    layout.scx1 = hor ? layout.tx : layout.tc - layout.scw / 2;
    layout.scy1 = hor ? layout.tc - layout.sch / 2 : layout.ty;
    layout.scx2 = hor ? layout.tx + layout.tw - layout.scw : layout.tc - layout.scw / 2;
    layout.scy2 = hor ? layout.tc - layout.sch / 2 : layout.ty + layout.th - layout.sch;
    
    // slit layout
    layout.sw = hor ? layout.tw - layout.scw * 2 - 2: 2;
    layout.sh = hor ? 2 : layout.th - layout.sch * 2 - 2;
    layout.sx = hor ? layout.tx + layout.scw + 1 : layout.tc - 1;
    layout.sy = hor ? layout.tc - 1 : layout.ty + layout.sch + 1;
    
    // slider layout
    layout.slw = gdk_pixbuf_get_width(fader->slider);
    layout.slh = gdk_pixbuf_get_height(fader->slider);
    layout.slx = fader->horizontal ? 0 : layout.tc - layout.slw / 2;
    layout.sly = fader->horizontal ? layout.tc - layout.slh / 2 : 0;
    
    //printf("widg %d %d %d %d | trgh %d %d %d %d %d | slid %d %d %d %d | slit %d %d %d %d\n",
            //layout.x, layout.y, layout.w, layout.h,
            //layout.tx, layout.ty, layout.tw, layout.th, layout.tc,
            //layout.slx, layout.sly, layout.slw, layout.slh,
            //layout.sx, layout.sy, layout.sw, layout.sh);
            
    fader->layout = layout;
}

GtkWidget *
calf_fader_new(const int horiz = 0, const int size = 2, const double min = 0, const double max = 1, const double step = 0.1)
{
    GtkObject *adj;
    gint digits;
    
    adj = gtk_adjustment_new (min, min, max, step, 10 * step, 0);
    
    if (fabs (step) >= 1.0 || step == 0.0)
        digits = 0;
    else
        digits = std::min(5, abs((gint) floor (log10 (fabs (step)))));
        
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_FADER, NULL ));
    CalfFader *self = CALF_FADER(widget);
    
    GTK_RANGE(widget)->orientation = horiz ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
    gtk_range_set_adjustment(GTK_RANGE(widget), GTK_ADJUSTMENT(adj));
    gtk_scale_set_digits(GTK_SCALE(widget), digits);
    
    self->size = size;
    self->horizontal = horiz;
    self->hover = 0;
    
    gchar * file = g_strdup_printf("%sslider%d-%s.png", PKGLIBDIR, size, horiz ? "horiz" : "vert");
    self->slider = gdk_pixbuf_new_from_file (file, 0);
    file = g_strdup_printf("%sslider%d-%s-prelight.png", PKGLIBDIR, size, horiz ? "horiz" : "vert");
    self->sliderpre = gdk_pixbuf_new_from_file(file, 0);
    
    self->screw = gdk_pixbuf_new_from_file (PKGLIBDIR "screw_silver.png", 0);
    
    return widget;
}

static bool calf_fader_hover(GtkWidget *widget)
{
    CalfFader *fader        = CALF_FADER(widget);
    CalfFaderLayout  layout = fader->layout;
    gint mx, my;
    gtk_widget_get_pointer(GTK_WIDGET(widget), &mx, &my);

    if ((fader->horizontal and mx + layout.x >= layout.tx
                           and mx + layout.x < layout.tx + layout.tw
                           and my + layout.y >= layout.sly
                           and my + layout.y < layout.sly + layout.slh)
    or (!fader->horizontal and mx + layout.x >= layout.slx
                           and mx + layout.x < layout.slx + layout.slw
                           and my + layout.y >= layout.ty
                           and my + layout.y < layout.ty + layout.th))
        return true;
    return false;
}
static void calf_fader_check_hover_change(GtkWidget *widget)
{
    CalfFader *fader = CALF_FADER(widget);
    bool hover = calf_fader_hover(widget);
    if (hover != fader->hover)
        gtk_widget_queue_draw(widget);
    fader->hover = hover;
}
static gboolean
calf_fader_motion (GtkWidget *widget, GdkEventMotion *event)
{
    calf_fader_check_hover_change(widget);
    return FALSE;
}

static gboolean
calf_fader_enter (GtkWidget *widget, GdkEventCrossing *event)
{
    calf_fader_check_hover_change(widget);
    return FALSE;
}

static gboolean
calf_fader_leave (GtkWidget *widget, GdkEventCrossing *event)
{
    CALF_FADER(widget)->hover = false;
    gtk_widget_queue_draw(widget);
    return FALSE;
}
static void
calf_fader_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
    calf_fader_set_layout(widget);
}
static gboolean
calf_fader_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_FADER(widget));
    if (gtk_widget_is_drawable (widget)) {
        
        GdkWindow *window       = widget->window;
        GtkScale  *scale        = GTK_SCALE(widget);
        GtkRange  *range        = GTK_RANGE(widget);
        CalfFader *fader        = CALF_FADER(widget);
        CalfFaderLayout layout  = fader->layout;
        cairo_t   *c            = gdk_cairo_create(GDK_DRAWABLE(window));
        
        cairo_rectangle(c, layout.x, layout.y, layout.w, layout.h);
        cairo_clip(c);
        
        // draw slit
        double r  = range->adjustment->upper - range->adjustment->lower;
        double v0 = range->adjustment->value - range->adjustment->lower;
        if ((fader->horizontal and gtk_range_get_inverted(range))
        or (!fader->horizontal and gtk_range_get_inverted(range)))
            v0 = -v0 + 1;
        int vp  = v0 / r * (fader->horizontal ? layout.w - layout.slw : layout.h - layout.slh)
                         + (fader->horizontal ? layout.slw : layout.slh) / 2
                         + (fader->horizontal ? layout.x : layout.y);
        layout.slx = fader->horizontal ? vp - layout.slw / 2 : layout.tc - layout.slw / 2;
        layout.sly = fader->horizontal ? layout.tc - layout.slh / 2 : vp - layout.slh / 2;
        
        cairo_rectangle(c, layout.sx - 1, layout.sy - 1, layout.sw, layout.sh);
        cairo_set_source_rgba(c, 0,0,0, 0.25);
        cairo_fill(c);
        cairo_rectangle(c, layout.sx + 1, layout.sy + 1, layout.sw, layout.sh);
        cairo_set_source_rgba(c, 1,1,1, 0.125);
        cairo_fill(c);
        cairo_rectangle(c, layout.sx, layout.sy, layout.sw, layout.sh);
        cairo_set_source_rgb(c, 0,0,0);
        cairo_fill(c);
        
        // draw screws
        cairo_rectangle(c, layout.x, layout.y, layout.w, layout.h);
        gdk_cairo_set_source_pixbuf(c, fader->screw, layout.scx1, layout.scy1);
        cairo_fill_preserve(c);
        gdk_cairo_set_source_pixbuf(c, fader->screw, layout.scx2, layout.scy2);
        cairo_fill_preserve(c);
        
        // draw slider
        if (fader->hover or gtk_grab_get_current() == widget)
            gdk_cairo_set_source_pixbuf(c, fader->sliderpre, layout.slx, layout.sly);
        else
            gdk_cairo_set_source_pixbuf(c, fader->slider, layout.slx, layout.sly);
        cairo_fill(c);
        
        // draw value label
        if (scale->draw_value) {
            PangoLayout *layout;
            gint _x, _y;
            layout = gtk_scale_get_layout (scale);
            gtk_scale_get_layout_offsets (scale, &_x, &_y);
            gtk_paint_layout (widget->style, window, GTK_STATE_NORMAL, FALSE, NULL,
                              widget, fader->horizontal ? "hscale" : "vscale", _x, _y, layout);
        }
        
        cairo_destroy(c);
    }
    return FALSE;
}

static void
calf_fader_class_init (CalfFaderClass *klass)
{
    GtkWidgetClass *widget_class      = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event        = calf_fader_expose;
}

static void
calf_fader_init (CalfFader *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 40;
    
    gtk_signal_connect(GTK_OBJECT(widget), "motion-notify-event", GTK_SIGNAL_FUNC (calf_fader_motion), NULL);
    gtk_signal_connect(GTK_OBJECT(widget), "enter-notify-event", GTK_SIGNAL_FUNC (calf_fader_enter), NULL);
    gtk_signal_connect(GTK_OBJECT(widget), "leave-notify-event", GTK_SIGNAL_FUNC (calf_fader_leave), NULL);
    gtk_signal_connect(GTK_OBJECT(widget), "size-allocate", GTK_SIGNAL_FUNC (calf_fader_allocate), NULL);
}

GType
calf_fader_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfFaderClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_fader_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfFader),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_fader_init
        };

        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfFader%u%d", 
                ((unsigned int)(intptr_t)calf_fader_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_SCALE,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}


///////////////////////////////////////// button ///////////////////////////////////////////////

GtkWidget *
calf_button_new(const gchar *label)
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_BUTTON, NULL ));
    gtk_button_set_label(GTK_BUTTON(widget), label);
    return widget;
}
static gboolean
calf_button_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_BUTTON(widget) || CALF_IS_TOGGLE_BUTTON(widget) || CALF_IS_RADIO_BUTTON(widget));
    
    if (gtk_widget_is_drawable (widget)) {
        
        int pad = 2;
        
        GdkWindow *window    = widget->window;
        GtkWidget *child     = GTK_BIN (widget)->child;
        cairo_t *c           = gdk_cairo_create(GDK_DRAWABLE(window));
        cairo_pattern_t *pat = NULL;
        
        int x  = widget->allocation.x;
        int y  = widget->allocation.y;
        int sx = widget->allocation.width;
        int sy = widget->allocation.height;
        
        cairo_rectangle(c, x, y, sx, sy);
        cairo_clip(c);
        
        cairo_rectangle(c, x, y, sx, sy);
        pat = cairo_pattern_create_radial(x + sx / 2, y + sy / 2, 1, x + sx / 2, y + sy / 2, sx / 2);
        switch (gtk_widget_get_state(widget)) {
            case GTK_STATE_NORMAL:
            default:
                cairo_pattern_add_color_stop_rgb(pat, 0.3,  39. / 255,  52. / 255,  87. / 255);
                cairo_pattern_add_color_stop_rgb(pat, 1.0,   6. / 255,   5. / 255,  14. / 255);
                break;
            case GTK_STATE_PRELIGHT:
                cairo_pattern_add_color_stop_rgb(pat, 0.3,  19. / 255, 237. / 255, 254. / 255);
                cairo_pattern_add_color_stop_rgb(pat, 1.0,   0. / 255,  45. / 255, 206. / 255);
                break;
            case GTK_STATE_ACTIVE:
            case GTK_STATE_SELECTED:
                cairo_pattern_add_color_stop_rgb(pat, 0.0,  19. / 255, 237. / 255, 254. / 255);
                cairo_pattern_add_color_stop_rgb(pat, 0.3,   10. / 255, 200. / 255, 240. / 255);
                cairo_pattern_add_color_stop_rgb(pat, 0.7,  19. / 255, 237. / 255, 254. / 255);
                cairo_pattern_add_color_stop_rgb(pat, 1.0,   2. / 255, 168. / 255, 230. / 255);
                break;
            
        }
        cairo_set_source(c, pat);
        cairo_fill(c);
        
        cairo_rectangle(c, x + pad, y + pad, sx - pad * 2, sy - pad * 2);
        if (CALF_IS_TOGGLE_BUTTON(widget) or CALF_IS_RADIO_BUTTON(widget)) {
            cairo_new_sub_path (c);
            cairo_rectangle(c, x + sx - pad * 2 - 23, y + sy / 2 - 1, 22, 2);
            cairo_set_fill_rule(c, CAIRO_FILL_RULE_EVEN_ODD);
        }
        pat = cairo_pattern_create_linear(x + pad, y + pad, x + pad, y + sy - pad * 2);
        cairo_pattern_add_color_stop_rgb(pat, 0.0,  0.92, 0.92, 0.92);
        cairo_pattern_add_color_stop_rgb(pat, 1.0,  0.70, 0.70, 0.70);
        cairo_set_source(c, pat);
        cairo_fill(c);
        
        int _h = GTK_WIDGET(GTK_BIN(widget)->child)->allocation.height + 0;
        int _y = y + (sy - _h) / 2;
        cairo_rectangle(c, x + pad, _y, sx - pad * 2, _h);
        if (CALF_IS_TOGGLE_BUTTON(widget) or CALF_IS_RADIO_BUTTON(widget)) {
            cairo_new_sub_path (c);
            cairo_rectangle(c, x + sx - pad * 2 - 23, y + sy / 2 - 1, 22, 2);
            cairo_set_fill_rule(c, CAIRO_FILL_RULE_EVEN_ODD);
        }
        pat = cairo_pattern_create_linear(x + pad, _y, x + pad, _y + _h);
        cairo_pattern_add_color_stop_rgb(pat, 1.0,  0.92, 0.92, 0.92);
        cairo_pattern_add_color_stop_rgb(pat, 0.0,  0.70, 0.70, 0.70);
        cairo_set_source(c, pat);
        cairo_fill(c);
        
        cairo_destroy(c);
        gtk_container_propagate_expose (GTK_CONTAINER (widget), child, event);
    }
    return FALSE;
}

static void
calf_button_class_init (CalfButtonClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_button_expose;
}

static void
calf_button_init (CalfButton *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 20;
}

GType
calf_button_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfButtonClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_button_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfButton),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_button_init
        };

        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfButton%u%d", 
                ((unsigned int)(intptr_t)calf_button_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_BUTTON,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}


///////////////////////////////////////// toggle button ///////////////////////////////////////////////

GtkWidget *
calf_toggle_button_new(const gchar *label)
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_TOGGLE_BUTTON, NULL ));
    gtk_button_set_label(GTK_BUTTON(widget), label);
    return widget;
}

static void
calf_toggle_button_class_init (CalfToggleButtonClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_button_expose;
}

static void
calf_toggle_button_init (CalfToggleButton *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 20;
}

GType
calf_toggle_button_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfToggleButtonClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_toggle_button_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfToggleButton),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_toggle_button_init
        };

        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfToggleButton%u%d", 
                ((unsigned int)(intptr_t)calf_toggle_button_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_TOGGLE_BUTTON,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}

///////////////////////////////////////// radio button ///////////////////////////////////////////////

GtkWidget *
calf_radio_button_new(const gchar *label)
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_RADIO_BUTTON, NULL ));
    gtk_button_set_label(GTK_BUTTON(widget), label);
    return widget;
}

static void
calf_radio_button_class_init (CalfRadioButtonClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_button_expose;
}

static void
calf_radio_button_init (CalfRadioButton *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 20;
}

GType
calf_radio_button_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfRadioButtonClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_radio_button_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfRadioButton),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_radio_button_init
        };

        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfRadioButton%u%d", 
                ((unsigned int)(intptr_t)calf_radio_button_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_RADIO_BUTTON,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}

///////////////////////////////////////// tap button ///////////////////////////////////////////////

GtkWidget *
calf_tap_button_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_TAP_BUTTON, NULL ));
    return widget;
}

static gboolean
calf_tap_button_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_TAP_BUTTON(widget));
    CalfTapButton *self = CALF_TAP_BUTTON(widget);
    
    int x = widget->allocation.x + widget->allocation.width / 2 - 35;
    int y = widget->allocation.y + widget->allocation.height / 2 - 35;
    int width = 70;
    int height = 70;
    
    gdk_draw_pixbuf(GDK_DRAWABLE(widget->window),
                    widget->style->fg_gc[0],
                    self->image[self->state],
                    0,
                    0,
                    x,
                    y,
                    width,
                    height,
                    GDK_RGB_DITHER_NORMAL, 0, 0);
    return TRUE;
}

static void
calf_tap_button_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_TAP_BUTTON(widget));
    requisition->width  = 70;
    requisition->height = 70;
}
static void
calf_tap_button_class_init (CalfTapButtonClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_tap_button_expose;
    widget_class->size_request = calf_tap_button_size_request;
}
static void
calf_tap_button_init (CalfTapButton *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 70;
    widget->requisition.height = 70;
    self->state = 0;
    GError *error = NULL;
    self->image[0] = gdk_pixbuf_new_from_file(PKGLIBDIR "/tap_inactive.png", &error);
    self->image[1] = gdk_pixbuf_new_from_file(PKGLIBDIR "/tap_prelight.png", &error);
    self->image[2] = gdk_pixbuf_new_from_file(PKGLIBDIR "/tap_active.png", &error);
}

GType
calf_tap_button_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfTapButtonClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_tap_button_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfTapButton),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_tap_button_init
        };

        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfTapButton%u%d", 
                ((unsigned int)(intptr_t)calf_tap_button_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_BUTTON,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}

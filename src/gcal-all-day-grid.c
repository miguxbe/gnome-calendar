/* -*- mode: c; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * gcal-all-day-grid.c
 *
 * Copyright (C) 2012 - Erick Pérez Castellanos
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gcal-all-day-grid.h"
#include "gcal-event-widget.h"

#include <string.h>

enum
{
  PROP_0,
  PROP_COLUMNS,
  PROP_SPACING
};

struct _GcalAllDayGridPrivate
{
  /* property */
  guint           columns_nr;
  gint            spacing;

  GList          *column_headers;
  GList          *children;

  GdkWindow      *event_window;
};

static void           gcal_all_day_grid_finalize              (GObject         *object);

static void           gcal_all_day_grid_set_property          (GObject         *object,
                                                               guint            property_id,
                                                               const GValue    *value,
                                                               GParamSpec      *pspec);

static void           gcal_all_day_grid_get_property          (GObject         *object,
                                                               guint            property_id,
                                                               GValue          *value,
                                                               GParamSpec      *pspec);

static void           gcal_all_day_grid_get_preferred_width   (GtkWidget       *widget,
                                                               gint            *minimum,
                                                               gint            *natural);

static void           gcal_all_day_grid_get_preferred_height  (GtkWidget       *widget,
                                                               gint            *minimum,
                                                               gint            *natural);

static void           gcal_all_day_grid_realize               (GtkWidget       *widget);

static void           gcal_all_day_grid_unrealize             (GtkWidget       *widget);

static void           gcal_all_day_grid_map                   (GtkWidget       *widget);

static void           gcal_all_day_grid_unmap                 (GtkWidget       *widget);

static void           gcal_all_day_grid_size_allocate         (GtkWidget       *widget,
                                                               GtkAllocation   *allocation);

static gboolean       gcal_all_day_grid_draw                  (GtkWidget       *widget,
                                                               cairo_t         *cr);

static gboolean       gcal_all_day_grid_button_press_event    (GtkWidget       *widget,
                                                               GdkEventButton  *event);

static gboolean       gcal_all_day_grid_button_release_event  (GtkWidget       *widget,
                                                               GdkEventButton  *event);

static void           gcal_all_day_grid_add                   (GtkContainer    *container,
                                                               GtkWidget       *widget);

static void           gcal_all_day_grid_remove                (GtkContainer    *container,
                                                               GtkWidget       *widget);

static void           gcal_all_day_grid_forall                (GtkContainer    *container,
                                                               gboolean         include_internals,
                                                               GtkCallback      callback,
                                                               gpointer         callback_data);

G_DEFINE_TYPE (GcalAllDayGrid,gcal_all_day_grid, GTK_TYPE_CONTAINER);

static void
gcal_all_day_grid_class_init (GcalAllDayGridClass *klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gcal_all_day_grid_finalize;
  object_class->set_property = gcal_all_day_grid_set_property;
  object_class->get_property = gcal_all_day_grid_get_property;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->get_preferred_width = gcal_all_day_grid_get_preferred_width;
  widget_class->get_preferred_height = gcal_all_day_grid_get_preferred_height;
  widget_class->realize = gcal_all_day_grid_realize;
  widget_class->unrealize = gcal_all_day_grid_unrealize;
  widget_class->map = gcal_all_day_grid_map;
  widget_class->unmap = gcal_all_day_grid_unmap;
  widget_class->size_allocate = gcal_all_day_grid_size_allocate;
  widget_class->draw = gcal_all_day_grid_draw;
  widget_class->button_press_event = gcal_all_day_grid_button_press_event;
  widget_class->button_release_event = gcal_all_day_grid_button_release_event;

  container_class = GTK_CONTAINER_CLASS (klass);
  container_class->add   = gcal_all_day_grid_add;
  container_class->remove = gcal_all_day_grid_remove;
  container_class->forall = gcal_all_day_grid_forall;
  gtk_container_class_handle_border_width (container_class);

  g_object_class_install_property (
      object_class,
      PROP_COLUMNS,
      g_param_spec_uint ("columns",
                         "Columns",
                         "The number of columns in which to split the grid",
                         0,
                         G_MAXUINT,
                         0,
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_READWRITE));

  g_object_class_install_property (
      object_class,
      PROP_SPACING,
      g_param_spec_int ("spacing",
                        "Spacing",
                        "The horizontal space between the header and the side and below lines",
                        0,
                        G_MAXINT,
                        0,
                        G_PARAM_CONSTRUCT |
                        G_PARAM_READWRITE));

  g_type_class_add_private ((gpointer)klass, sizeof (GcalAllDayGridPrivate));
}



static void
gcal_all_day_grid_init (GcalAllDayGrid *self)
{
  GcalAllDayGridPrivate *priv;

  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            GCAL_TYPE_ALL_DAY_GRID,
                                            GcalAllDayGridPrivate);
  priv = self->priv;
  priv->column_headers = NULL;
  priv->children = NULL;
}

static void
gcal_all_day_grid_finalize (GObject *object)
{
  GcalAllDayGridPrivate *priv;

  priv = GCAL_ALL_DAY_GRID (object)->priv;

  if (priv->children != NULL)
    g_list_free_full (priv->children, (GDestroyNotify) g_list_free);

  if (priv->column_headers != NULL)
    g_list_free_full (priv->column_headers, g_free);

  /* Chain up to parent's finalize() method. */
  G_OBJECT_CLASS (gcal_all_day_grid_parent_class)->finalize (object);
}

static void
gcal_all_day_grid_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GcalAllDayGridPrivate *priv;
  priv = GCAL_ALL_DAY_GRID (object)->priv;

  switch (property_id)
    {
    case PROP_COLUMNS:
      {
        gint i;
        priv->columns_nr = g_value_get_uint (value);
        for (i = 0; i < priv->columns_nr; ++i)
          {
            priv->children = g_list_prepend (priv->children, NULL);
          }
        break;
      }
    case PROP_SPACING:
      priv->spacing = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gcal_all_day_grid_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GcalAllDayGridPrivate *priv;
  priv = GCAL_ALL_DAY_GRID (object)->priv;

  switch (property_id)
    {
    case PROP_COLUMNS:
      g_value_set_uint (value, priv->columns_nr);
      break;
    case PROP_SPACING:
      g_value_set_int (value, priv->spacing);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* GtkWidget API */
static void
gcal_all_day_grid_get_preferred_width (GtkWidget *widget,
                                       gint      *minimum,
                                       gint      *natural)
{
  GcalAllDayGridPrivate* priv;

  GtkBorder padding;

  GList *headers;
  gchar *longest_header;
  gint length;

  PangoLayout *layout;
  PangoRectangle logical_rect;

  priv = GCAL_ALL_DAY_GRID (widget)->priv;

  length = 0;
  longest_header = NULL;
  for (headers = priv->column_headers;
       headers != NULL;
       headers = g_list_next (headers))
    {
      gchar *header = (gchar*) headers->data;
      if (length < strlen (header))
        {
          length = strlen (header);
          longest_header = header;
        }
    }

  layout = gtk_widget_create_pango_layout (widget, longest_header);

  pango_layout_get_extents (layout, NULL, &logical_rect);
  pango_extents_to_pixels (&logical_rect, NULL);

  gtk_style_context_get_padding (gtk_widget_get_style_context (widget),
                                 gtk_widget_get_state_flags (widget),
                                 &padding);

  if (minimum != NULL)
    {
      *minimum =
        (priv->columns_nr * logical_rect.width) + padding.left + padding.right +
        (priv->columns_nr * 2 * priv->spacing);
    }
  if (natural != NULL)
    {
      *natural =
        (priv->columns_nr * logical_rect.width) + padding.left + padding.right +
        (priv->columns_nr * 2 * priv->spacing);
    }

  g_object_unref (layout);
}

static void
gcal_all_day_grid_get_preferred_height (GtkWidget *widget,
                                        gint      *minimum,
                                        gint      *natural)
{
  GcalAllDayGridPrivate* priv;

  GList *columns;

  gint min_height;
  guint min_keeper;
  gint nat_height;
  guint nat_keeper;

  GtkBorder padding;
  PangoLayout *layout;
  PangoRectangle logical_rect;

  priv = GCAL_ALL_DAY_GRID (widget)->priv;

  layout = gtk_widget_create_pango_layout (widget, "0");

  pango_layout_get_extents (layout, NULL, &logical_rect);
  pango_extents_to_pixels (&logical_rect, NULL);

  gtk_style_context_get_padding (gtk_widget_get_style_context (widget),
                                 gtk_widget_get_state_flags (widget),
                                 &padding);
  g_object_unref (layout);

  min_keeper = G_MAXUINT;
  nat_keeper = 0;

  for (columns = priv->children;
       columns != NULL;
       columns = g_list_next (columns))
    {
      GList *column_children = (GList*) columns->data;
      gint column_height;

      for (;column_children != NULL;
           column_children = g_list_next (column_children))
        {
          GtkWidget *widget;
          widget = (GtkWidget*) column_children->data;

          gtk_widget_get_preferred_height (widget, &min_height, &nat_height);

          /* minimum is calculated as the minimum required
           by any child */
          if (min_keeper < min_height)
              min_keeper = min_height;

          column_height += nat_height;
        }

      if (nat_keeper < column_height)
        nat_keeper = column_height;
    }

  if (minimum != NULL)
    {
      *minimum = min_keeper + logical_rect.height + priv->spacing + padding.top + padding.bottom;
    }
  if (natural != NULL)
    {
      *natural = nat_keeper + logical_rect.height + priv->spacing + padding.top + padding.bottom;
    }
}

static void
gcal_all_day_grid_realize (GtkWidget *widget)
{
  GcalAllDayGridPrivate *priv;
  GdkWindow *parent_window;
  GdkWindowAttr attributes;
  gint attributes_mask;
  GtkAllocation allocation;

  priv = GCAL_ALL_DAY_GRID (widget)->priv;
  gtk_widget_set_realized (widget, TRUE);

  parent_window = gtk_widget_get_parent_window (widget);
  gtk_widget_set_window (widget, parent_window);
  g_object_ref (parent_window);

  gtk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
                            GDK_BUTTON_RELEASE_MASK |
                            GDK_BUTTON1_MOTION_MASK |
                            GDK_POINTER_MOTION_HINT_MASK |
                            GDK_POINTER_MOTION_MASK |
                            GDK_ENTER_NOTIFY_MASK |
                            GDK_LEAVE_NOTIFY_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y;

  priv->event_window = gdk_window_new (parent_window,
                                       &attributes,
                                       attributes_mask);
  gdk_window_set_user_data (priv->event_window, widget);
}

static void
gcal_all_day_grid_unrealize (GtkWidget *widget)
{
  GcalAllDayGridPrivate *priv;

  priv = GCAL_ALL_DAY_GRID (widget)->priv;
  if (priv->event_window != NULL)
    {
      gdk_window_set_user_data (priv->event_window, NULL);
      gdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  GTK_WIDGET_CLASS (gcal_all_day_grid_parent_class)->unrealize (widget);
}

static void
gcal_all_day_grid_map (GtkWidget *widget)
{
  GcalAllDayGridPrivate *priv;

  priv = GCAL_ALL_DAY_GRID (widget)->priv;
  if (priv->event_window != NULL)
    gdk_window_show (priv->event_window);

  GTK_WIDGET_CLASS (gcal_all_day_grid_parent_class)->map (widget);
}

static void
gcal_all_day_grid_unmap (GtkWidget *widget)
{
  GcalAllDayGridPrivate *priv;

  priv = GCAL_ALL_DAY_GRID (widget)->priv;
  if (priv->event_window != NULL)
    gdk_window_hide (priv->event_window);

  GTK_WIDGET_CLASS (gcal_all_day_grid_parent_class)->map (widget);
}

static void
gcal_all_day_grid_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  GcalAllDayGridPrivate *priv;

  GtkBorder padding;
  gint y_gap;

  PangoLayout *layout;
  PangoRectangle logical_rect;

  GList *columns;
  gint idx;

  gint width;

  priv = GCAL_ALL_DAY_GRID (widget)->priv;
  gtk_widget_set_allocation (widget, allocation);

  gtk_style_context_get_padding (
      gtk_widget_get_style_context (widget),
      gtk_widget_get_state_flags (widget),
      &padding);

  width = allocation->width - (padding.left + padding.right);

  layout = gtk_widget_create_pango_layout (widget, "0");

  pango_layout_get_extents (layout, NULL, &logical_rect);
  pango_extents_to_pixels (&logical_rect, NULL);
  y_gap = allocation->y + padding.top + logical_rect.height + priv->spacing;

  g_object_unref (layout);

  /* Placing the event_window */
  if (gtk_widget_get_realized (widget))
    {
      gdk_window_move_resize (priv->event_window,
                              allocation->x,
                              y_gap,
                              allocation->width,
                              allocation->height - y_gap);
    }

  /* Placing children */
  for (columns = priv->children, idx = 0;
       columns != NULL;
       columns = g_list_next (columns), ++idx)
    {
      GtkAllocation child_allocation;
      GList *column;

      column = (GList*) columns->data;

      child_allocation.x = idx * (width / priv->columns_nr) + allocation->x + padding.left;
      child_allocation.width = (width / priv->columns_nr);

      for (; column != NULL; column = g_list_next (column))
        {
          gint natural_height;
          GtkWidget *item = (GtkWidget*) column->data;
          gtk_widget_get_preferred_height (item, NULL, &natural_height);

          child_allocation.y = y_gap;
          child_allocation.height = natural_height;

          y_gap += natural_height;

          gtk_widget_size_allocate (item, &child_allocation);
        }

      y_gap = allocation->y + padding.top + logical_rect.height + priv->spacing;
    }
}

static gboolean
gcal_all_day_grid_draw (GtkWidget *widget,
                        cairo_t   *cr)
{
  GcalAllDayGridPrivate *priv;

  GtkBorder padding;
  GtkAllocation alloc;
  gint width;
  gint height;
  gint y_gap;

  PangoLayout *layout;
  PangoRectangle logical_rect;
  PangoFontDescription *font_desc;

  GList *header;
  gint i;

  GdkRGBA ligther_color;

  priv = GCAL_ALL_DAY_GRID (widget)->priv;

  gtk_style_context_get_padding (
      gtk_widget_get_style_context (widget),
      gtk_widget_get_state_flags (widget),
      &padding);

  gtk_widget_get_allocation (widget, &alloc);
  width = alloc.width - (padding.left + padding.right);
  height = alloc.height - (padding.top + padding.bottom);

  y_gap = padding.top + priv->spacing;

  gtk_style_context_get (
      gtk_widget_get_style_context (widget),
      gtk_widget_get_state_flags (widget),
      "font", &font_desc, NULL);

  cairo_save (cr);

  layout = pango_cairo_create_layout (cr);
  pango_layout_set_font_description (layout, font_desc);

  pango_layout_get_extents (layout, NULL, &logical_rect);
  pango_extents_to_pixels (&logical_rect, NULL);
  y_gap += logical_rect.height;

  for (header = priv->column_headers, i = 0;
       header != NULL;
       header = g_list_next (header), i++)
    {
      gchar *text = (gchar*) header->data;
      pango_layout_set_text (layout, text, -1);
      pango_cairo_update_layout (cr, layout);

      cairo_move_to (cr,
                     (width / priv->columns_nr) * i + priv->spacing + padding.left,
                     padding.top);
      pango_cairo_show_layout (cr, layout);
    }

  /* drawing lines */
  gtk_style_context_get_color (
      gtk_widget_get_style_context (widget),
      gtk_widget_get_state_flags (widget) | GTK_STATE_FLAG_INSENSITIVE,
      &ligther_color);
  cairo_set_source_rgb (cr,
                        ligther_color.red,
                        ligther_color.green,
                        ligther_color.blue);
  cairo_set_line_width (cr, 0.3);

  cairo_move_to (cr, padding.left, y_gap + 0.4);
  cairo_rel_line_to (cr, width, 0);

  for (i = 0; i < priv->columns_nr + 1; ++i)
    {
      cairo_move_to (cr, padding.left + i * (width / priv->columns_nr) + 0.4, y_gap);
      cairo_rel_line_to (cr, 0, height - y_gap);
    }

  cairo_stroke (cr);
  cairo_restore (cr);

  /* drawing children */
  if (GTK_WIDGET_CLASS (gcal_all_day_grid_parent_class)->draw != NULL)
    GTK_WIDGET_CLASS (gcal_all_day_grid_parent_class)->draw (widget, cr);

  pango_font_description_free (font_desc);
  g_object_unref (layout);

  return FALSE;
}

static gboolean
gcal_all_day_grid_button_press_event (GtkWidget      *widget,
                                      GdkEventButton *event)
{
  g_debug ("Button pressed on all-day area");
  return FALSE;
}

static gboolean
gcal_all_day_grid_button_release_event (GtkWidget      *widget,
                                        GdkEventButton *event)
{
  g_debug ("Button released on all-day area");
  return FALSE;
}

/* GtkContainer API */
/**
 * gcal_all_day_grid_add:
 * @container: a #GcalAllDayGrid
 * @widget: a widget to add
 *
 * @gtk_container_add implementation. It assumes the widget will go
 * to the first column.
 *
 **/
static void
gcal_all_day_grid_add (GtkContainer *container,
                       GtkWidget    *widget)
{
  GcalAllDayGridPrivate* priv;

  priv = GCAL_ALL_DAY_GRID (container)->priv;
  g_return_if_fail (priv->columns_nr != 0);

  gcal_all_day_grid_place (GCAL_ALL_DAY_GRID (container),
                           widget,
                           0);
}

static void
gcal_all_day_grid_remove (GtkContainer *container,
                          GtkWidget    *widget)
{
  GcalAllDayGridPrivate *priv;

  GList *columns;

  priv = GCAL_ALL_DAY_GRID (container)->priv;

  for (columns = priv->children;
       columns != NULL;
       columns = g_list_next (columns))
    {
      GList* column = (GList*) columns->data;
      for (;column != NULL; column = g_list_next (column))
        {
          if (widget == (GtkWidget*) column->data)
            {
              GList* orig = (GList*) columns->data;
              orig = g_list_delete_link (orig, column);

              gtk_widget_unparent (widget);

              columns->data = orig;

              columns = NULL;
              break;
            }
        }
    }
}

static void
gcal_all_day_grid_forall (GtkContainer *container,
                          gboolean      include_internals,
                          GtkCallback   callback,
                          gpointer      callback_data)
{
  GcalAllDayGridPrivate *priv;

  GList* columns;

  priv = GCAL_ALL_DAY_GRID (container)->priv;

  columns = priv->children;
  while (columns)
    {
      GList *column;

      column = columns->data;
      columns = columns->next;

      while (column)
        {
          GtkWidget *child = (GtkWidget*) column->data;
          column  = column->next;

          (* callback) (child, callback_data);
        }
    }
}

/* Public API */
/**
 * gcal_all_day_grid_new:
 *
 * Since: 0.1
 * Return value: A new #GcalAllDayGrid
 * Returns: (transfer full):
 **/
GtkWidget*
gcal_all_day_grid_new (guint columns)
{
  return g_object_new (GCAL_TYPE_ALL_DAY_GRID, "columns", columns, NULL);
}

/**
 * gcal_all_day_grid_set_column_headers:
 * @all_day: A #GcalAllDayGrid
 * @...: a list of strings
 *
 * Set the columns headers. If #GcalAllDayGrid:columns property
 * has not been set, this function does nothing.
 **/
void
gcal_all_day_grid_set_column_headers (GcalAllDayGrid *all_day,
                                      ...)
{
  GcalAllDayGridPrivate *priv;
  va_list args;
  gint i;

  priv = all_day->priv;

  if (priv->columns_nr == 0)
    return;

  if (priv->column_headers != NULL)
    g_list_free_full (priv->column_headers, g_free);

  va_start (args, all_day);
  for (i = 0; i < priv->columns_nr; ++i)
    {
      gchar *header;
      header = g_strdup (va_arg(args, gchar*));

      priv->column_headers = g_list_append (priv->column_headers,
                                            header);
    }

  va_end (args);
}

/**
 * gcal_all_day_grid_place:
 * @all_day: A #GcalAllDayGrid widget
 * @widget: The child widget to add
 * @column_idx: The index of the column, starting with zero
 *
 * Adding a widget to a specified column. If the column index
 * is bigger than the #GcalAllDayGrid:columns property set,
 * it will do nothing.
 **/
void
gcal_all_day_grid_place (GcalAllDayGrid *all_day,
                         GtkWidget      *widget,
                         guint           column_idx)
{
  GcalAllDayGridPrivate *priv;
  GList* children_link;
  GList* column;

  priv = all_day->priv;

  g_return_if_fail (column_idx < priv->columns_nr);

  children_link = g_list_nth (priv->children, column_idx);
  column = (GList*) children_link->data;
  children_link->data = g_list_append (column, widget);

  gtk_widget_set_parent (widget, GTK_WIDGET (all_day));
}

GtkWidget*
gcal_all_day_grid_get_by_uuid (GcalAllDayGrid *all_day,
                               const gchar    *uuid)
{
  GcalAllDayGridPrivate *priv;

  GList* columns;

  priv = all_day->priv;

  columns = priv->children;
  while (columns)
    {
      GList *column;

      column = columns->data;
      columns = columns->next;

      while (column)
        {
          GcalEventWidget *child = GCAL_EVENT_WIDGET ((GtkWidget*) column->data);
          column  = column->next;

          if (g_strcmp0 (uuid,
                         gcal_event_widget_peek_uuid (child)) == 0)
            {
              return (GtkWidget*) child;
            }
        }
    }

  return NULL;
}
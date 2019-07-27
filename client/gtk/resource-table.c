/* A custom widget for selecting resources
 *
 * The code is based on the TICTACTOE and DIAL examples
 * http://www.gtk.org/tutorial/app-codeexamples.html
 * http://www.gtk.org/tutorial/sec-gtkdial.html
 *
 * Adaptation for Pioneers: 2004 Roland Clobus
 *
 */

#include "config.h"
#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>

#include "resource-table.h"
#include "callback.h"

/* The signals */
enum {
	CHANGE,
	LAST_SIGNAL
};

static void resource_table_class_init(ResourceTableClass * klass);
static void resource_table_init(ResourceTable * rt);
static void resource_table_update(ResourceTable * rt);

static void value_changed_cb(GtkSpinButton * widget, gpointer user_data);

/* All signals */
static guint resource_table_signals[LAST_SIGNAL] = { 0 };

/* Register the class */
GType resource_table_get_type(void)
{
	static GType rt_type = 0;

	if (!rt_type) {
		static const GTypeInfo rt_info = {
			sizeof(ResourceTableClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) resource_table_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(ResourceTable),
			0,
			(GInstanceInitFunc) resource_table_init,
			NULL
		};
		rt_type =
		    g_type_register_static(GTK_TYPE_GRID, "ResourceTable",
					   &rt_info, 0);
	}
	return rt_type;
}

/* Register the signals.
 * ResourceTable will emit this signal:
 * 'change'         when any change in the amount occurs.
 */
static void resource_table_class_init(ResourceTableClass * klass)
{
	resource_table_signals[CHANGE] = g_signal_new("change",
						      G_TYPE_FROM_CLASS
						      (klass),
						      G_SIGNAL_RUN_FIRST |
						      G_SIGNAL_ACTION,
						      G_STRUCT_OFFSET
						      (ResourceTableClass,
						       change), NULL, NULL,
						      g_cclosure_marshal_VOID__VOID,
						      G_TYPE_NONE, 0);
}

/* Initialise the composite widget */
static void resource_table_init(ResourceTable * rt)
{
	gint i;

	for (i = 0; i < NO_RESOURCE; i++) {
		rt->row[i].hand = 0;
		rt->row[i].bank = 0;
		rt->row[i].amount = 0;
		rt->row[i].hand_widget = NULL;
		rt->row[i].bank_widget = NULL;
		rt->row[i].amount_widget = NULL;
	}
	rt->total_target = 0;
	rt->total_current = 0;
	rt->total_widget = NULL;

	rt->limit_bank = FALSE;
	rt->with_bank = FALSE;
	rt->with_total = FALSE;
	rt->direction = RESOURCE_TABLE_MORE_IN_HAND;

}

static void resource_table_set_limit(ResourceTable * rt, gint row)
{
	gint limit;

	if (rt->with_total)
		limit =
		    rt->with_bank ?
		    MIN(rt->total_target, rt->row[row].bank) :
		    MIN(rt->total_target, rt->row[row].hand);
	else
		limit = rt->with_bank ? rt->row[row].bank :
		    rt->direction ==
		    RESOURCE_TABLE_MORE_IN_HAND ? 99 : rt->row[row].hand;
	rt->row[row].limit = limit;
	gtk_spin_button_set_range(GTK_SPIN_BUTTON
				  (rt->row[row].amount_widget), 0, limit);
	gtk_widget_set_sensitive(rt->row[row].amount_widget, limit > 0);
}

/* Create a new ResourceTable */
GtkWidget *resource_table_new(const gchar * title,
			      ResourceTableDirection direction,
			      gboolean with_bank)
{
	ResourceTable *rt;

	gchar *temp;
	GtkWidget *widget;
	gint i;
	guint row;

	rt = g_object_new(resource_table_get_type(), NULL);

	rt->direction = direction;

	rt->with_bank = with_bank;
	/* Don't set rt->with_total yet, wait for _set_total */

	rt->bank_offset = with_bank ? 1 : 0;
	gtk_grid_set_row_spacing(GTK_GRID(rt), 3);
	gtk_grid_set_column_spacing(GTK_GRID(rt), 5);

	temp = g_strconcat("<b>", title, "</b>", NULL);
	widget = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(widget), temp);
	g_free(temp);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(rt), widget, 0, 0, 3 + rt->bank_offset,
			1);
	gtk_label_set_xalign(GTK_LABEL(widget), 0.0);

	row = 1;
	for (i = 0; i < NO_RESOURCE; i++) {
		rt->row[i].filter = FALSE;

		widget = rt->row[i].label_widget =
		    gtk_label_new(resource_name(i, TRUE));
		gtk_widget_show(widget);
		gtk_grid_attach(GTK_GRID(rt), widget, 0, row, 1, 1);
		gtk_label_set_xalign(GTK_LABEL(widget), 0.0);

		widget = rt->row[i].hand_widget = gtk_entry_new();
		gtk_widget_show(widget);
		gtk_grid_attach(GTK_GRID(rt), widget, 1, row, 1, 1);
		gtk_entry_set_width_chars(GTK_ENTRY(widget), 3);
		gtk_widget_set_sensitive(widget, FALSE);
		gtk_entry_set_alignment(GTK_ENTRY(widget), 1.0);
		gtk_widget_set_tooltip_text(widget,
					    /* Tooltip for the amount of resources in the hand */
					    _("Amount in hand"));

		rt->row[i].hand = resource_asset(i);

		if (with_bank) {
			rt->row[i].bank = get_bank()[i];
			widget = rt->row[i].bank_widget = gtk_entry_new();
			gtk_widget_show(widget);
			gtk_grid_attach(GTK_GRID(rt), widget, 2, row, 1,
					1);
			gtk_entry_set_width_chars(GTK_ENTRY(widget), 3);
			gtk_widget_set_sensitive(widget, FALSE);
			gtk_entry_set_alignment(GTK_ENTRY(widget), 1.0);
			gtk_widget_set_tooltip_text(widget,
						    /* Tooltip for the amount of resources in the bank */
						    _(""
						      "Amount in the bank"));
		}

		widget = rt->row[i].amount_widget =
		    gtk_spin_button_new_with_range(0, 99, 1);
		gtk_widget_show(widget);
		gtk_grid_attach(GTK_GRID(rt), widget, 2 + rt->bank_offset,
				row, 1, 1);
		gtk_entry_set_width_chars(GTK_ENTRY(widget), 3);
		gtk_entry_set_alignment(GTK_ENTRY(widget), 1.0);
		g_signal_connect(G_OBJECT(widget), "value-changed",
				 G_CALLBACK(value_changed_cb),
				 &rt->row[i]);
		gtk_widget_set_tooltip_text(widget,
					    /* Tooltip for the selected amount */
					    _("Selected amount"));

		resource_table_set_limit(rt, i);
		row++;
	}
	resource_table_update(rt);
	return GTK_WIDGET(rt);
}

void resource_table_limit_bank(ResourceTable * rt, gboolean limit)
{
	rt->limit_bank = limit;
	resource_table_update(rt);
}

void resource_table_set_total(ResourceTable * rt, const gchar * text,
			      gint total)
{
	GtkWidget *widget;
	guint row;
	gint i;

	g_assert(IS_RESOURCETABLE(rt));
	rt->with_total = TRUE;
	rt->total_target = total;
	rt->total_current = 0;
	row = NO_RESOURCE + 1;

	widget = gtk_label_new(text);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(rt), widget, 0, row, 2 + rt->bank_offset,
			1);
	gtk_label_set_xalign(GTK_LABEL(widget), 1.0);

	widget = rt->total_widget =
	    gtk_spin_button_new_with_range(0, total, 1);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(rt), widget, 2 + rt->bank_offset, row, 1,
			1);
	gtk_entry_set_width_chars(GTK_ENTRY(widget), 3);
	gtk_widget_set_sensitive(widget, FALSE);
	gtk_entry_set_alignment(GTK_ENTRY(widget), 1.0);
	gtk_widget_set_tooltip_text(widget,
				    /* Tooltip for the total selected amount */
				    _("Total selected amount"));

	for (i = 0; i < NO_RESOURCE; i++) {
		resource_table_set_limit(rt, i);
	}
	resource_table_update(rt);
}

void resource_table_set_bank(ResourceTable * rt, const gint * bank)
{
	gint i;

	for (i = 0; i < NO_RESOURCE; i++) {
		rt->row[i].bank = bank[i];
		resource_table_set_limit(rt, i);
		if (rt->limit_bank && rt->with_total
		    && bank[i] > rt->total_target) {
			gtk_widget_set_tooltip_text(rt->row[i].bank_widget,
						    /* Tooltip when the bank cannot be emptied */
						    _(""
						      "The bank cannot be emptied"));
		}
	}
	resource_table_update(rt);
}

/* Update the display to the current state */
static void resource_table_update(ResourceTable * rt)
{
	gchar buff[16];
	gint i;
	struct _ResourceRow *row;
	gboolean less_enabled;
	gboolean more_enabled;

	g_assert(IS_RESOURCETABLE(rt));
	rt->total_current = 0;
	for (i = 0; i < NO_RESOURCE; i++)
		rt->total_current += rt->row[i].amount;

	if (rt->with_total) {
		sprintf(buff, "%d", rt->total_current);
		gtk_entry_set_text(GTK_ENTRY(rt->total_widget), buff);
	}

	for (i = 0; i < NO_RESOURCE; i++) {
		row = &rt->row[i];

		sprintf(buff, "%d", row->amount);
		gtk_entry_set_text(GTK_ENTRY(row->amount_widget), buff);

		less_enabled = row->amount > 0;
		more_enabled = row->amount < row->limit;
		if (rt->with_total
		    && rt->total_current >= rt->total_target)
			more_enabled = FALSE;

		if (rt->direction == RESOURCE_TABLE_MORE_IN_HAND)
			sprintf(buff, "%d", row->hand + row->amount);
		else
			sprintf(buff, "%d", row->hand - row->amount);

		gtk_entry_set_text(GTK_ENTRY(row->hand_widget), buff);

		if (rt->with_bank) {
			if (rt->limit_bank && rt->with_total
			    && row->bank > rt->total_target)
				sprintf(buff, "%s", "++");
			else
				sprintf(buff, "%d",
					row->bank - row->amount);
			gtk_entry_set_text(GTK_ENTRY(row->bank_widget),
					   buff);
		}

		gtk_widget_set_sensitive(row->label_widget, !row->filter);
		gtk_widget_set_sensitive(row->amount_widget,
					 (less_enabled || more_enabled)
					 && !row->filter);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON
					  (row->amount_widget), 0,
					  more_enabled ? row->
					  limit : row->amount);
	}
}

static void value_changed_cb(GtkSpinButton * widget, gpointer user_data)
{
	struct _ResourceRow *row = user_data;
	ResourceTable *rt =
	    RESOURCETABLE(gtk_widget_get_parent(GTK_WIDGET(widget)));

	row->amount = gtk_spin_button_get_value_as_int(widget);
	resource_table_update(rt);
	g_signal_emit(G_OBJECT(rt), resource_table_signals[CHANGE], 0);
}

void resource_table_get_amount(ResourceTable * rt, gint * amount)
{
	gint i;

	g_assert(IS_RESOURCETABLE(rt));
	for (i = 0; i < NO_RESOURCE; i++)
		amount[i] = rt->row[i].amount;
}

gboolean resource_table_is_total_reached(ResourceTable * rt)
{
	g_assert(IS_RESOURCETABLE(rt));
	return (rt->total_current == rt->total_target);
}

void resource_table_update_hand(ResourceTable * rt)
{
	gint i;

	g_assert(IS_RESOURCETABLE(rt));
	for (i = 0; i < NO_RESOURCE; i++) {
		rt->row[i].hand = resource_asset(i);
		resource_table_set_limit(rt, i);
	}
	resource_table_update(rt);
}

void resource_table_set_filter(ResourceTable * rt, const gint * resource)
{
	gint i;

	g_assert(IS_RESOURCETABLE(rt));
	for (i = 0; i < NO_RESOURCE; i++) {
		rt->row[i].filter = resource[i] == 0;
	}
	resource_table_update(rt);
}

void resource_table_clear(ResourceTable * rt)
{
	gint i;

	g_assert(IS_RESOURCETABLE(rt));
	for (i = 0; i < NO_RESOURCE; i++) {
		rt->row[i].amount = 0;
	}
	resource_table_update(rt);
}

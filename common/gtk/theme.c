/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2005, 2009 Roland Clobus <rclobus@bigfoot.com>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "theme.h"
#include "config-gnome.h"

/*

  Description of theme.cfg:
  -------------------------

  A theme.cfg file is a series of definitions, one per line. Lines starting
  with '#' are comments. A definition looks like

    VARIABLE = VALUE

  There are three types of variables: pixmaps, colors, and the scaling mode.
  The value for a pixmap is a filename, relative to the theme directory. A
  color can be either 'none' or 'transparent' (both equivalent), or '#rrggbb'.
  (It's also allowed to use 1, 3, or 4 digits per color component.)

  Pixmaps can be defined for hex backgrounds (hill-tile, field-tile,
  mountain-tile, pasture-tile, forest-tile, desert-tile, sea-tile, board-tile)
  and port backgrounds (brick-port-tile, grain-port-tile, ore-port-tile,
  wool-port-tile, and lumber-port-tile). If a hex tile is not defined, the
  pixmap from the default theme will be used. If a port tile is not given, the
  corresponding 2:1 ports will be painted in solid background color.

  chip-bg-color, chip-fg-color, and chip-bd-color are the colors for the dice
  roll chips in the middle of a hex. port-{bg,fg,bd}-color are the same for
  ports. chip-hi-bg-color is used as background for chips that correspond to
  the current roll, chip-hi-fg-color is chips showing 6 or 8. You can also
  define robber-{fg,bg}-color for the robber and hex-bd-color for the border
  of hexes (around the background image). If any color is not defined, the
  default color will be used. If a color is 'none' the corresponding element
  won't be painted at all.

  The five chip colors can also be different for each terrain type. To
  override the general colors, add color definitions after the name of the
  pixmap, e.g.:

    field-tile	= field_grain.png none #d0d0d0 none #303030 #ffffff

  The order is bg, fg, bd, hi-bg, hi-fg. Sorry, you can't skip definitions at
  the beginning...

  The chip colors for SEA_TILE are used in the GUI to display stand-alone
  chips. Transparency should not be used here.

  Normally, all pixmaps are used in their native size and repeat over their
  area as needed (tiling). This doesn't look good for photo-like images, so
  you can add "scaling = always" in that case. Then images will always be
  scaled to the current size of a hex. (BTW, the height should be 1/cos(pi/6)
  (~ 1.1547) times the width of the image.) Two other available modes are
  'only-downscale' and 'only-upscale' to make images only smaller or larger,
  resp. (in case it's needed sometimes...)

*/

#define TCOL_INIT(r,g,b)	{ TRUE, FALSE, { r, g, b, 1.0 } }

static TColor default_colors[] = {
	TCOL_INIT(1.00, 0.85, 0.72),
	TCOL_INIT(0.00, 0.00, 0.00),
	TCOL_INIT(0.00, 0.00, 0.00),
	TCOL_INIT(0.00, 1.00, 0.00),
	TCOL_INIT(1.00, 0.00, 0.00),
	TCOL_INIT(0.00, 0.00, 1.00),
	TCOL_INIT(1.00, 1.00, 1.00),
	TCOL_INIT(0.00, 0.00, 0.00),
	TCOL_INIT(0.00, 0.00, 0.00),
	TCOL_INIT(1.00, 1.00, 1.00),
	TCOL_INIT(1.00, 0.85, 0.72)
};

#define offs(elem)           ((size_t)(&(((MapTheme *)0)->elem)))
#define telem(type,theme,tv) (*((type *)((char *)theme + tv->offset)))

typedef enum { FNAME, STR, COL, SCMODE } vartype;

static struct tvars {
	const char *name;
	gboolean optional;
	vartype type;
	int override;
	size_t offset;
} theme_vars[] = {
	{
	"name", TRUE, STR, -1, offs(name)}, {
	"hill-tile", FALSE, FNAME, HILL_TILE,
		    offs(terrain_tile_names[HILL_TILE])}, {
	"field-tile", FALSE, FNAME, FIELD_TILE,
		    offs(terrain_tile_names[FIELD_TILE])}, {
	"mountain-tile", FALSE, FNAME, MOUNTAIN_TILE,
		    offs(terrain_tile_names[MOUNTAIN_TILE])}, {
	"pasture-tile", FALSE, FNAME, PASTURE_TILE,
		    offs(terrain_tile_names[PASTURE_TILE])}, {
	"forest-tile", FALSE, FNAME, FOREST_TILE,
		    offs(terrain_tile_names[FOREST_TILE])}, {
	"desert-tile", FALSE, FNAME, -1,
		    offs(terrain_tile_names[DESERT_TILE])}, {
	"sea-tile", FALSE, FNAME, SEA_TILE,
		    offs(terrain_tile_names[SEA_TILE])}, {
	"gold-tile", FALSE, FNAME, GOLD_TILE,
		    offs(terrain_tile_names[GOLD_TILE])}, {
	"board-tile", FALSE, FNAME, -1,
		    offs(terrain_tile_names[BOARD_TILE])}, {
	"brick-port-tile", TRUE, FNAME, -1,
		    offs(port_tile_names[HILL_PORT_TILE])}, {
	"grain-port-tile", TRUE, FNAME, -1,
		    offs(port_tile_names[FIELD_PORT_TILE])}, {
	"ore-port-tile", TRUE, FNAME, -1,
		    offs(port_tile_names[MOUNTAIN_PORT_TILE])}, {
	"wool-port-tile", TRUE, FNAME, -1,
		    offs(port_tile_names[PASTURE_PORT_TILE])}, {
	"lumber-port-tile", TRUE, FNAME, -1,
		    offs(port_tile_names[FOREST_PORT_TILE])}, {
	"nores-port-tile", TRUE, FNAME, -1,
		    offs(port_tile_names[ANY_PORT_TILE])}, {
	"chip-bg-color", TRUE, COL, -1, offs(colors[TC_CHIP_BG])}, {
	"chip-fg-color", TRUE, COL, -1, offs(colors[TC_CHIP_FG])}, {
	"chip-bd-color", TRUE, COL, -1, offs(colors[TC_CHIP_BD])}, {
	"chip-hi-bg-color", TRUE, COL, -1, offs(colors[TC_CHIP_H_BG])},
	{
	"chip-hi-fg-color", TRUE, COL, -1, offs(colors[TC_CHIP_H_FG])},
	{
	"port-bg-color", TRUE, COL, -1, offs(colors[TC_PORT_BG])}, {
	"port-fg-color", TRUE, COL, -1, offs(colors[TC_PORT_FG])}, {
	"port-bd-color", TRUE, COL, -1, offs(colors[TC_PORT_BD])}, {
	"robber-fg-color", TRUE, COL, -1, offs(colors[TC_ROBBER_FG])},
	{
	"robber-bd-color", TRUE, COL, -1, offs(colors[TC_ROBBER_BD])},
	{
	"hex-bd-color", TRUE, COL, -1, offs(colors[TC_HEX_BD])}, {
	"scaling", FALSE, SCMODE, -1, offs(scaling)}
};

static GList *theme_list = NULL;
static MapTheme *current_theme = NULL;
static GList *callback_list = NULL;

static gboolean theme_initialize(MapTheme * t);
static void theme_cleanup(MapTheme * t);
static void theme_scan_dir(const gchar * themes_path);
static gint getvar(gchar ** p, const gchar * filename, gint lno);
static char *getval(char **p, const gchar * filename, int lno);
static gboolean parsecolor(char *p, TColor * tc, const gchar * filename,
			   int lno);
static MapTheme *theme_config_parse(const gchar * themename,
				    const gchar * subdir);
static gboolean theme_load_pixbuf(const gchar * file,
				  const gchar * themename,
				  GdkPixbuf ** pixbuf);

/** Find a theme with the given name */
static gint theme_list_locate(gconstpointer item, gconstpointer data)
{
	const MapTheme *theme = item;
	const gchar *name = data;
	return strcmp(theme->name, name);
}

/** Insert the theme alphabetically in the list */
static gint theme_insert_sorted(gconstpointer new, gconstpointer first)
{
	const MapTheme *newTheme = new;
	const MapTheme *firstTheme = first;
	return strcmp(newTheme->name, firstTheme->name);
}

void themes_init(void)
{
	gchar *path;
	MapTheme *t;
	gint novar;
	gchar *user_theme;

	g_assert(theme_list == NULL);

	/* scan global theme directory */
	theme_scan_dir(THEMEDIR);

	/* scan user theme directory */
	path =
	    g_build_filename(g_get_user_data_dir(), "pioneers", "themes",
			     NULL);
	theme_scan_dir(path);

	if (theme_list == NULL) {
		g_error("No theme found: %s or %s", THEMEDIR, path);
	}
	g_free(path);

	t = NULL;
	user_theme = config_get_string("settings/theme=Tiny", &novar);
	if (!(!user_theme || !*user_theme)) {
		GList *result = g_list_find_custom(theme_list, user_theme,
						   theme_list_locate);
		if (result)
			t = result->data;
	}
	g_free(user_theme);
	if (!t) {
		t = g_list_first(theme_list)->data;
	}
	current_theme = t;
}

void themes_cleanup(void)
{
	GList *list = theme_list;
	while (list) {
		theme_cleanup(list->data);
		list = g_list_next(list);
	}
	g_list_free(theme_list);
	g_list_free(callback_list);
}

void theme_scan_dir(const gchar * themes_path)
{
	GDir *dir;
	const gchar *dirname;
	gchar *fname;
	MapTheme *t;

	/* scan image dir for theme descriptor files */
	if (!(dir = g_dir_open(themes_path, 0, NULL)))
		return;
	while ((dirname = g_dir_read_name(dir))) {
		fname = g_build_filename(themes_path, dirname, NULL);
		if (g_file_test(fname, G_FILE_TEST_IS_DIR)) {
			if ((t = theme_config_parse(dirname, fname))) {
				if (theme_initialize(t)) {
					theme_list =
					    g_list_insert_sorted
					    (theme_list, t,
					     theme_insert_sorted);
				} else {
					g_warning
					    ("Theme %s not loaded due to errors.",
					     t->name);
				}
			} else {
				g_warning
				    ("Theme %s not loaded due to errors.",
				     dirname);
			}
		}
		g_free(fname);
	}
	g_dir_close(dir);

}

void theme_set_current(MapTheme * t)
{
	GList *list = callback_list;
	current_theme = t;
	while (list) {
		G_CALLBACK(list->data) ();
		list = g_list_next(list);
	}
}

MapTheme *theme_get_current(void)
{
	return current_theme;
}

GList *theme_get_list(void)
{
	return theme_list;
}

/** Load a pixbuf.
 *  If loading fails, no objects need to be freed.
 *  @return TRUE if successful
 */
gboolean theme_load_pixbuf(const gchar * file, const gchar * themename,
			   GdkPixbuf ** pixbuf)
{
	g_return_val_if_fail(themename != NULL, FALSE);
	g_return_val_if_fail(pixbuf != NULL, FALSE);
	g_return_val_if_fail(file != NULL, FALSE);

	*pixbuf = NULL;

	/* check that file exists */
	if (!g_file_test(file, G_FILE_TEST_EXISTS)) {
		g_warning
		    ("Could not find \'%s\' pixmap file in theme \'%s\'.",
		     file, themename);
		return FALSE;
	}

	/* load pixmap/mask */
	*pixbuf = gdk_pixbuf_new_from_file(file, NULL);

	/* check result */
	if (*pixbuf == NULL) {
		g_warning
		    ("Could not load \'%s\' pixmap file in theme \'%s\'.",
		     file, themename);
		return FALSE;
	}
	return TRUE;
}

/** Initialize the theme.
 *  @return TRUE if successful
 */
static gboolean theme_initialize(MapTheme * t)
{
	guint i;

	/* load terrain tiles */
	for (i = 0; i < G_N_ELEMENTS(t->terrain_tiles); ++i) {
		GdkPixbuf *pixbuf;
		GdkPixbuf *pixbuf_copy;
		if (!theme_load_pixbuf
		    (t->terrain_tile_names[i], t->name, &pixbuf)) {
			g_error("Could not find pixmap file: %s",
				t->terrain_tile_names[i]);
		};
		t->terrain_tiles[i] = pixbuf;
		pixbuf_copy = gdk_pixbuf_copy(pixbuf);
		if (pixbuf_copy == NULL) {
			return FALSE;
		}
		t->scaledata[i].native_image = pixbuf_copy;
		t->scaledata[i].native_width =
		    gdk_pixbuf_get_width(pixbuf);
		t->scaledata[i].aspect = 1.0 *
		    gdk_pixbuf_get_width(pixbuf) /
		    gdk_pixbuf_get_height(pixbuf);
	}

	/* load port tiles */
	for (i = 0; i < G_N_ELEMENTS(t->port_tiles); ++i) {
		/* if a theme doesn't define a port tile, it will be drawn with
		 * its resource letter instead */
		if (t->port_tile_names[i]) {
			if (theme_load_pixbuf
			    (t->port_tile_names[i], t->name,
			     &(t->port_tiles[i]))) {
				t->port_tiles_width[i] =
				    gdk_pixbuf_get_width(t->port_tiles[i]);
				t->port_tiles_height[i] =
				    gdk_pixbuf_get_height(t->port_tiles
							  [i]);
			}
		} else
			t->port_tiles[i] = NULL;
	}

	for (i = 0; i < G_N_ELEMENTS(t->colors); ++i) {
		TColor *tc = &(t->colors[i]);
		if (!tc->set)
			*tc = default_colors[i];
	}

	return TRUE;
}

static void theme_cleanup(MapTheme * t)
{
	guint i;

	/* terrain tiles */
	for (i = 0; i < G_N_ELEMENTS(t->terrain_tiles); ++i) {
		g_object_unref(t->terrain_tiles[i]);
		g_object_unref(t->scaledata[i].native_image);
	}
	/* port tiles */
	for (i = 0; i < G_N_ELEMENTS(t->port_tiles); ++i) {
		if (t->port_tiles[i] != NULL) {
			g_object_unref(t->port_tiles[i]);
		}
	}
	for (i = 0; i < G_N_ELEMENTS(theme_vars); i++) {
		switch (theme_vars[i].type) {
		case STR:
		case FNAME:
			g_free(telem(char *, t, (&theme_vars[i])));
			break;
		default:
			/* No action required */
			break;
		}
	}
	g_free(t);
}

void theme_rescale(int new_width)
{
	guint i;

	switch (current_theme->scaling) {
	case NEVER:
		return;

	case ONLY_DOWNSCALE:
		if (new_width > current_theme->scaledata[0].native_width)
			new_width =
			    current_theme->scaledata[0].native_width;
		break;

	case ONLY_UPSCALE:
		if (new_width < current_theme->scaledata[0].native_width)
			new_width =
			    current_theme->scaledata[0].native_width;
		break;

	case ALWAYS:
		break;
	}

	/* if the size is 0, gdk_pixbuf_scale_simple fails */
	if (new_width <= 0)
		new_width = 1;

	/* no need to scale again */
	if (new_width == current_theme->current_width) {
		return;
	}
	current_theme->current_width = new_width;

	for (i = 0; i < G_N_ELEMENTS(current_theme->terrain_tiles); ++i) {
		int new_height;
		if (i == BOARD_TILE)
			continue;	/* Don't scale the board-tile */
		new_height =
		    new_width / current_theme->scaledata[i].aspect;
		/* gdk_pixbuf_scale_simple cannot handle 0 height */
		if (new_height <= 0)
			new_height = 1;
		/* rescale the pixbuf */
		g_object_unref(current_theme->terrain_tiles[i]);
		current_theme->terrain_tiles[i] =
		    gdk_pixbuf_scale_simple(current_theme->scaledata[i].
					    native_image, new_width,
					    new_height,
					    GDK_INTERP_BILINEAR);
	}
}

#define ERR1(formatstring, argument) \
	g_warning("While reading %s at line %d:", filename, lno); \
	g_warning(formatstring, argument);
#define ERR0(string) \
	ERR1("%s", string);

/**
 * Find a variable name in the file.
 * @retval p The line to parse (returns the remainder)
 * @param filename Filename for the error message
 * @param lno Line number for the error message
 * @return -1 on error, otherwise the index in theme_vars
*/
static gint getvar(gchar ** p, const gchar * filename, int lno)
{
	char *q, qsave;
	struct tvars *tv;
	guint idx;
	gboolean found;

	*p += strspn(*p, " \t");
	if (!**p || **p == '\n')
		return -1;	/* empty line */

	q = *p + strcspn(*p, " \t=\n");
	if (q == *p) {
		ERR1("variable name missing: %s", *p);
		return -1;
	}
	qsave = *q;
	*q++ = '\0';

	idx = 0;
	found = FALSE;
	for (tv = theme_vars; idx < G_N_ELEMENTS(theme_vars); ++tv, ++idx) {
		if (strcmp(*p, tv->name) == 0) {
			found = TRUE;
			break;
		}
	}
	if (!found) {
		ERR1("unknown config variable '%s'", *p);
		return -1;
	}

	*p = q;
	if (qsave != '=') {
		*p += strspn(*p, " \t");
		if (**p != '=') {
			ERR1("'=' missing: %s", *p);
			return -1;
		}
		++*p;
	}
	*p += strspn(*p, " \t");

	return idx;
}

static char *getval(char **p, const gchar * filename, int lno)
{
	char *q;

	q = *p;
	*p += strcspn(*p, " \t\n");
	if (q == *p) {
		ERR0("missing value");
		return FALSE;
	}
	if (**p) {
		*(*p)++ = '\0';
		*p += strspn(*p, " \t");
	}
	return q;
}

static gboolean checkend(const char *p)
{
	p += strspn(p, " \t");
	return !*p || *p == '\n';
}

static gboolean parsecolor(char *p, TColor * tc, const gchar * filename,
			   int lno)
{
	if (strcmp(p, "none") == 0 || strcmp(p, "transparent") == 0) {
		tc->set = TRUE;
		tc->transparent = TRUE;
		return TRUE;
	}
	tc->transparent = FALSE;
	if (!gdk_rgba_parse(&tc->color, p)) {
		ERR1("invalid color: %s", p);
		return FALSE;
	}

	tc->set = TRUE;
	return TRUE;
}

static MapTheme *theme_config_parse(const gchar * themename,
				    const gchar * subdir)
{
	FILE *f;
	char *line = NULL;
	gchar *p, *q;
	gint lno;
	MapTheme *t;
	struct tvars *tv;
	gboolean ok = TRUE;
	gboolean *used;
	guint idx;
	gint var;
	gchar *filename;

	filename = g_build_filename(subdir, "theme.cfg", NULL);
	if (!(f = fopen(filename, "r"))) {
		g_warning("could not open '%s'", filename);
		g_free(filename);
		return NULL;
	}

	t = g_malloc0(sizeof(MapTheme));
	/* Initially the theme name is equal to the directory name */
	t->name = g_strdup(themename);
	t->current_width = -1;

	used = g_malloc0(G_N_ELEMENTS(theme_vars) * sizeof(gboolean));

	lno = 0;
	while (read_line_from_file(&line, f)) {
		++lno;
		if (line[0] == '#') {
			g_free(line);
			continue;
		}
		p = line;
		var = getvar(&p, filename, lno);
		if ((var == -1) || !(q = getval(&p, filename, lno))) {
			ok = FALSE;
			g_free(line);
			continue;
		}
		idx = var;
		tv = &theme_vars[idx];

		switch (tv->type) {
		case STR:
			if (telem(char *, t, tv)) {
				g_free(telem(char *, t, tv));
			}
			telem(char *, t, tv) = g_strdup(q);
			break;
		case FNAME:
			telem(char *, t, tv) =
			    g_build_filename(subdir, q, NULL);
			if (tv->override >= 0 && !checkend(p)) {
				int terrain = tv->override;
				int i;

				for (i = 0; i < TC_MAX_OVERRIDE; ++i) {
					if (checkend(p))
						break;
					if (!
					    (q =
					     getval(&p, filename, lno))) {
						ok = FALSE;
						break;
					}
					if (!parsecolor
					    (q,
					     &(t->ovr_colors[terrain][i]),
					     filename, lno)) {
						ok = FALSE;
						break;
					}
				}
			}
			break;
		case COL:
			if (!parsecolor
			    (q, &telem(TColor, t, tv), filename, lno)) {
				ok = FALSE;
				g_free(line);
				continue;
			}
			break;
		case SCMODE:
			if (strcmp(q, "never") == 0)
				t->scaling = NEVER;
			else if (strcmp(q, "always") == 0)
				t->scaling = ALWAYS;
			else if (strcmp(q, "only-downscale") == 0)
				t->scaling = ONLY_DOWNSCALE;
			else if (strcmp(q, "only-upscale") == 0)
				t->scaling = ONLY_UPSCALE;
			else {
				ERR1("bad scaling mode '%s'", q);
				ok = FALSE;
			}
			break;
		}
		used[idx] = TRUE;

		if (!checkend(p)) {
			ERR1("unexpected rest at end of line: '%s'", p);
			ok = FALSE;
		}
		g_free(line);
	}
	fclose(f);

	for (idx = 0; idx < G_N_ELEMENTS(theme_vars); idx++) {
		if (!used[idx] && !theme_vars[idx].optional) {
			ERR1("option '%s' missing", theme_vars[idx].name);
			ok = FALSE;
		}
	};

	g_free(used);
	g_free(filename);

	if (ok)
		return t;
	g_free(t->name);
	g_free(t);
	return NULL;
}

void theme_register_callback(GCallback callback)
{
	callback_list = g_list_append(callback_list, callback);
}

GdkPixbuf *theme_get_terrain_pixbuf(Terrain terrain)
{
	return theme_get_current()->terrain_tiles[terrain];
}

/* This entire file is licensed under GNU General Public License v3.0
 *
 * Copyright 2022- sfwbar maintainers
 */

#include <glib.h>
#include <stdio.h>
#include <gio/gio.h>
#include <unistd.h>
#include "sfwbar.h"
#include "trayitem.h"
#include "tray.h"
#include "scaleimage.h"

struct sni_prop_wrapper {
  guint prop;
  SniItem *sni;
};

static GList *sni_items;
static guint pix_counter;

static gchar *sni_properties[] = { "Category", "Id", "Title", "Status",
  "IconName", "OverlayIconName", "AttentionIconName", "AttentionMovieName",
  "XAyatanaLabel", "XAyatanaLabelGuide", "IconThemePath", "IconPixmap",
  "OverlayIconPixmap", "AttentionIconPixmap", "ToolTip", "WindowId",
  "ItemIsMenu", "Menu", "XAyatanaOrderingIndex" };

gchar *sni_item_get_pixbuf ( GVariant *v )
{
  GVariant *img,*child;
  cairo_surface_t *cs;
  GdkPixbuf *res;
  gint32 x,y;
  guint32 *ptr;
  gsize len, i;
  gchar *name;

  if(!v || !g_variant_check_format_string(v, "a(iiay)", FALSE) ||
      g_variant_n_children(v) < 1)
    return NULL;

  child = g_variant_get_child_value(v, 0);

  g_variant_get(child, "(ii@ay)", &x, &y, &img);
  ptr = (guint32 *)g_variant_get_fixed_array(img, &len, sizeof(guchar));

  if(!len || !ptr || len != x*y*4)
  {
    g_variant_unref(img);
    g_variant_unref(child);
    return NULL;
  }

  ptr = g_memdup2(ptr, len);
  g_variant_unref(img);
  g_variant_unref(child);
  for(i=0; i<x*y; i++)
    ptr[i] = g_ntohl(ptr[i]);

  cs = cairo_image_surface_create_for_data((guchar *)ptr, CAIRO_FORMAT_ARGB32,
      x, y, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, x));
  res = gdk_pixbuf_get_from_surface(cs, 0, 0, x, y);
  cairo_surface_destroy(cs);
  g_free(ptr);

  name = g_strdup_printf("<pixbufcache/>sni-%u", pix_counter++);
  scale_image_cache_insert(name, res);

  return name;
}

void sni_item_prop_cb ( GDBusConnection *con, GAsyncResult *res,
    struct sni_prop_wrapper *wrap)
{
  GVariant *result, *inner;

  wrap->sni->ref--;

  result = g_dbus_connection_call_finish(con, res, NULL);
  if(result)
  {
    g_variant_get(result, "(v)",&inner);
    g_variant_unref(result);
  }

  if(!result || !inner)
  {
    g_free(wrap);
    return;
  }

  if(wrap->prop<=SNI_PROP_THEME &&
      g_variant_is_of_type(inner,G_VARIANT_TYPE_STRING))
  {
    g_free(wrap->sni->string[wrap->prop]);
    g_variant_get(inner, "s", &(wrap->sni->string[wrap->prop]));
    g_debug("sni %s: property %s = %s", wrap->sni->dest,
        sni_properties[wrap->prop], wrap->sni->string[wrap->prop]);
  }
  else if(wrap->prop>=SNI_PROP_ICONPIX && wrap->prop<=SNI_PROP_ATTNPIX)
  {
    scale_image_cache_remove(wrap->sni->string[wrap->prop]);
    g_clear_pointer(&(wrap->sni->string[wrap->prop]), g_free);
    wrap->sni->string[wrap->prop] = sni_item_get_pixbuf(inner);
    g_debug("sni %s: property %s received", wrap->sni->dest,
        sni_properties[wrap->prop]);
  }
  else if(wrap->prop == SNI_PROP_MENU &&
      g_variant_is_of_type(inner,G_VARIANT_TYPE_OBJECT_PATH))
  {
    g_free(wrap->sni->menu_path);
    g_variant_get(inner, "o", &(wrap->sni->menu_path));
    g_debug("sni %s: property %s = %s", wrap->sni->dest,
        sni_properties[wrap->prop], wrap->sni->menu_path);
  }
  else if(wrap->prop == SNI_PROP_ISMENU)
  {
    g_variant_get(inner, "b", &(wrap->sni->menu));
    g_debug("sni %s: property %s = %d", wrap->sni->dest,
        sni_properties[wrap->prop], wrap->sni->menu);
  }
  else if(wrap->prop == SNI_PROP_ORDER)
  {
    g_variant_get(inner, "u", &(wrap->sni->order));
    g_debug("sni %s: property %s = %u", wrap->sni->dest,
        sni_properties[wrap->prop], wrap->sni->order);
  }

  g_variant_unref(inner);
  tray_invalidate_all(wrap->sni);
  g_free(wrap);
}

void sni_item_get_prop ( GDBusConnection *con, SniItem *sni,
    guint prop )
{
  struct sni_prop_wrapper *wrap;

  wrap = g_malloc(sizeof(struct sni_prop_wrapper));
  wrap->prop = prop;
  wrap->sni = sni;
  wrap->sni->ref++;

  g_dbus_connection_call(con, sni->dest, sni->path,
    "org.freedesktop.DBus.Properties", "Get",
    g_variant_new("(ss)", sni->host->item_iface, sni_properties[prop]), NULL,
    G_DBUS_CALL_FLAGS_NONE, -1, sni->cancel,
    (GAsyncReadyCallback)sni_item_prop_cb, wrap);
}

void sni_item_signal_cb (GDBusConnection *con, const gchar *sender,
         const gchar *path, const gchar *interface, const gchar *signal,
         GVariant *parameters, gpointer data)
{
  g_debug("sni: received signal %s from %s", signal, sender);
  if(!g_strcmp0(signal, "NewTitle"))
    sni_item_get_prop(con, data,  SNI_PROP_TITLE);
  else if(!g_strcmp0(signal, "NewStatus"))
    sni_item_get_prop(con, data, SNI_PROP_STATUS);
  else if(!g_strcmp0(signal, "NewToolTip"))
    sni_item_get_prop(con, data, SNI_PROP_TOOLTIP);
  else if(!g_strcmp0(signal, "NewIconThemePath"))
    sni_item_get_prop(con, data, SNI_PROP_THEME);
  else if(!g_strcmp0(signal, "NewIcon"))
  {
    sni_item_get_prop(con, data, SNI_PROP_ICON);
    sni_item_get_prop(con, data, SNI_PROP_ICONPIX);
  }
  else if(!g_strcmp0(signal, "NewOverlayIcon"))
  {
    sni_item_get_prop(con, data, SNI_PROP_OVLAY);
    sni_item_get_prop(con, data, SNI_PROP_OVLAYPIX);
  }
  else if(!g_strcmp0(signal, "NewAttentionIcon"))
  {
    sni_item_get_prop(con, data, SNI_PROP_ATTN);
    sni_item_get_prop(con, data, SNI_PROP_ATTNPIX);
  }
  else if(!g_strcmp0(signal, "XAyatanaNewLabel"))
    sni_item_get_prop(con, data, SNI_PROP_LABEL);
}

SniItem *sni_item_new (GDBusConnection *con, SniHost *host,
    const gchar *uid)
{
  SniItem *sni;
  gchar *path;
  guint i;

  sni = g_malloc0(sizeof(SniItem));
  sni->uid = g_strdup(uid);
  sni->cancel = g_cancellable_new();
  sni->menu = TRUE;
  path = strchr(uid,'/');
  if(path!=NULL)
  {
    sni->dest = g_strndup(uid, path-uid);
    sni->path = g_strdup(path);
  }
  else
  {
    sni->path = g_strdup("/StatusNotifierItem");
    sni->dest = g_strdup(uid);
  }
  sni->host = host;
  sni->signal = g_dbus_connection_signal_subscribe(con, sni->dest,
      host->item_iface, NULL, sni->path, NULL, 0, sni_item_signal_cb,
      sni, NULL);
  sni_items = g_list_append(sni_items, sni);
  tray_item_init_for_all(sni);
  for(i=0; i<SNI_MAX_PROP; i++)
    sni_item_get_prop(con, sni, i);

  return sni;
}

void sni_item_free ( SniItem *sni )
{
  gint i;

  tray_invalidate_all(sni);
  g_dbus_connection_signal_unsubscribe(sni_get_connection(), sni->signal);
  tray_item_destroy(sni);
  g_cancellable_cancel(sni->cancel);
  g_object_unref(sni->cancel);
  for(i=0; i<3; i++)
    scale_image_cache_remove(sni->string[SNI_PROP_ICONPIX+i]);
  for(i=0; i<SNI_MAX_STRING; i++)
    g_free(sni->string[i]);

  g_free(sni->menu_path);
  g_free(sni->uid);
  g_free(sni->path);
  g_free(sni->dest);
  g_free(sni);
}

GList *sni_item_get_list ( void )
{
  return sni_items;
}

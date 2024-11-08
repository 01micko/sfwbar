#ifndef __SFWBAR_STRING_H__
#define __SFWBAR_STRING_H__

#include <glib.h>

guint str_nhash ( gchar *str );
gboolean str_nequal ( gchar *str1, gchar *str2 );
void *ptr_pass ( void *ptr );
gchar *str_replace ( gchar *str, gchar *old, gchar *new );
int md5_file( gchar *path, guchar output[16] );
gboolean pattern_match ( gchar **dict, gchar *string );
gboolean regex_match_list ( GList *dict, gchar *string );

#endif

/* This entire file is licensed under GNU General Public License v3.0
 *
 * Copyright 2022- sfwbar maintainers
 */

#include <glib.h>
#include "sfwbar.h"

gboolean jpath_filter_test ( GScanner *scanner, gint idx, gchar *key,
    gboolean eq, struct json_object *obj, GTokenType type, GTokenValue val )
{
  struct json_object *tmp;

  switch(type)
  {
    case ']':
      return TRUE;
      break;
    case G_TOKEN_INT:
      if(idx==val.v_int)
        return TRUE;
      break;
    case G_TOKEN_STRING:
      json_object_object_get_ex(obj,key,&tmp);
      if(tmp && !eq)
        return TRUE;
      if(tmp && eq && scanner->token == G_TOKEN_STRING)
        if(!g_ascii_strcasecmp(val.v_string,json_object_get_string(tmp)))
          return TRUE;
      if(tmp && eq && scanner->token == G_TOKEN_INT)
        if(val.v_int == json_object_get_int64(tmp))
          return TRUE;
      if(tmp && eq && scanner->token == G_TOKEN_FLOAT)
        if(val.v_float == json_object_get_double(tmp))
          return TRUE;
      break;
    default:
      break;
  }
  return FALSE;
}

struct json_object *jpath_filter (GScanner *scanner, struct json_object *obj )
{
  struct json_object *next, *iter, *jiter;
  gint i,j;
  gchar *key=NULL;
  GTokenType type;
  GTokenValue val;
  gboolean eq = FALSE;

  next = json_object_new_array();
  type = g_scanner_get_next_token(scanner);
  switch(type)
  {
    case G_TOKEN_STRING:
      key = g_strdup(scanner->value.v_string);
      if(g_scanner_peek_next_token(scanner)=='=')
      {
        eq = TRUE;
        g_scanner_get_next_token(scanner);
        scanner->config->scan_float = 1;
        g_scanner_get_next_token(scanner);
        val = scanner->value;
        scanner->config->scan_float = 0;
      }
      else
        eq = FALSE;
      break;
    case ']':
      break;
    case G_TOKEN_INT:
      val = scanner->value;
      break;
    default:
      return next;
      break;
  }

  for(i=0;i<json_object_array_length(obj);i++)
  {
    iter = json_object_array_get_idx(obj,i);
    if(json_object_is_type(iter,json_type_array))
      for(j=0;j<json_object_array_length(iter);j++)
      {
        jiter = json_object_array_get_idx(iter,j);
        if(jpath_filter_test(scanner,j,key,eq,jiter,type,val))
          json_object_array_add(next,jiter);
      }
    else
      if(jpath_filter_test(scanner,-1,key,eq,iter,type,val))
        json_object_array_add(next,iter);
  }

  if(type == G_TOKEN_STRING || type == G_TOKEN_INT)
    if(g_scanner_get_next_token(scanner)!=']')
      g_scanner_error(scanner,"missing ']'");
  g_free(key);

  return next;
}

struct json_object *jpath_key ( GScanner *scanner, struct json_object *obj )
{
  struct json_object *next, *iter, *tmp;
  gint i,j;

  next = json_object_new_array();

  for(i=0;i<json_object_array_length(obj);i++)
  {
    iter = json_object_array_get_idx(obj,i);
    if(json_object_is_type(iter,json_type_array))
      for(j=0;j<json_object_array_length(iter);j++)
      {
        json_object_object_get_ex(json_object_array_get_idx(iter,j),
          scanner->value.v_string,&tmp);
        if(tmp)
          json_object_array_add(next,tmp);
      }
    else
    {
      json_object_object_get_ex(json_object_array_get_idx(obj,i),
        scanner->value.v_string,&tmp);
      if(tmp)
        json_object_array_add(next,tmp);
    }
  }
  return next;
}

struct json_object *jpath_index ( GScanner *scanner, struct json_object *obj )
{
  struct json_object *next, *iter;
  gint i;

  next = json_object_new_array();

  for(i=0;i<json_object_array_length(obj);i++)
  {
    iter = json_object_array_get_idx(obj,i);
    if(json_object_is_type(iter,json_type_array))
      json_object_array_add(next,
           json_object_array_get_idx(iter,(gint)scanner->value.v_int));
  }
  return next;
}

struct json_object *jpath_parse ( gchar *path, struct json_object *obj )
{
  GScanner *scanner;
  struct json_object *cur,*next;
  gint i, sep;

  if(!path || !obj)
    return NULL;
  scanner = g_scanner_new(NULL);
  scanner->config->scan_octal = 0;
  scanner->config->symbol_2_token = 1;
  scanner->config->char_2_token = 0;
  scanner->config->scan_float = 0;
  scanner->config->case_sensitive = 0;
  scanner->config->numbers_2_int = 1;
  scanner->config->identifier_2_string = 1;
  scanner->input_name = path;
  scanner->user_data = path;
  g_scanner_input_text(scanner, path, strlen(path));

  if(g_scanner_get_next_token(scanner)!=G_TOKEN_CHAR)
    return NULL;

  sep = scanner->value.v_char;
  scanner->config->char_2_token = 1;

  json_object_get(obj);
  if(json_object_is_type(obj,json_type_array))
    cur = obj;
  else
  {
    cur = json_object_new_array();
    json_object_array_add(cur,obj);
  }

  do 
  {
    switch((gint)g_scanner_get_next_token(scanner))
    {
      case '[':
        next = jpath_filter(scanner,cur);
        break;
      case G_TOKEN_STRING:
        next = jpath_key(scanner,cur);
        break;
      case G_TOKEN_INT:
        next = jpath_index(scanner,cur);
        break;
      default:
        g_scanner_error(scanner,"invalid token in json path %d %d",scanner->token,G_TOKEN_ERROR);
        next = NULL;
        break;
    }
    if(next)
    {
      for(i=0;i<json_object_array_length(next);i++)
        json_object_get(json_object_array_get_idx(next,i));
      json_object_put(cur);
      cur = next;
    }
  } while ( g_scanner_get_next_token(scanner) == sep );

  g_scanner_destroy( scanner );

  return cur;
}

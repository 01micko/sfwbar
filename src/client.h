#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <gio/gio.h>
#include "scanner.h"

typedef struct _Client Client;
struct _Client {
  ScanFile *file;
  GSocketConnection *scon;
  GSocketConnectable *addr;
  GSocketClient *sclient;
  GIOChannel *in,*out;
  void *data;
  gboolean (*connect) ( Client * );
  GIOStatus (*respond) ( Client * );
  GIOStatus (*consume) ( Client *, gsize *size );
};

void client_exec ( ScanFile *file );
void client_socket ( ScanFile *file );
void client_mpd ( ScanFile *file );
void client_attach ( Client *client );
gboolean client_socket_connect ( Client *client );
void client_send ( gchar *addr, gchar *command );
void client_mpd_command ( gchar *command );

#endif

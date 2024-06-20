#ifndef __TASKBARSHELL_H__
#define __TASKBARSHELL_H__

#include "taskbar.h"

#define TASKBAR_SHELL_TYPE          (taskbar_shell_get_type())
#define TASKBAR_SHELL(obj)          (G_TYPE_CHECK_INSTANCE_CAST((obj), TASKBAR_SHELL_TYPE, TaskbarShell))
#define TASKBAR_SHELL_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass), TASKBAR_SHELL_TYPE, TaskbarShellClass))
#define IS_TASKBAR_SHELL(obj)       (G_TYPE_CHECK_INSTANCE_TYPE((obj), TASKBAR_SHELL_TYPE))
#define IS_TASKBARSHELLCLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), TASKBAR_SHELL_TYPE))

typedef struct _TaskbarShell TaskbarShell;
typedef struct _TaskbarShellClass TaskbarShellClass;

struct _TaskbarShell
{
  Taskbar item;
};

struct _TaskbarShellClass
{
  TaskbarClass parent_class;
};

typedef struct _TaskbarShellPrivate TaskbarShellPrivate;

struct _TaskbarShellPrivate
{
  GtkWidget *(*get_taskbar)(GtkWidget *, window_t *, gboolean);
  gboolean icons, labels, sort, floating_filter;
  gint rows, cols, filter, title_width;
  gchar *style;
  GList *css;
};

GType taskbar_shell_get_type ( void );

void taskbar_shell_populate ( void );
void taskbar_shell_update_all ( void );
void taskbar_shell_item_invalidate ( window_t *win );
void taskbar_shell_init_child ( GtkWidget *self, GtkWidget *child );
void taskbar_shell_invalidate_all ( void );
void taskbar_shell_item_init_for_all ( window_t *win );
void taskbar_shell_item_destroy_for_all ( window_t *win );
void taskbar_shell_set_filter ( GtkWidget *self, gint filter );
gint taskbar_shell_get_filter ( GtkWidget *self, gboolean * );
void taskbar_shell_set_api ( GtkWidget *self,
   GtkWidget *(*)(GtkWidget *, window_t*, gboolean) );
void taskbar_shell_set_group_labels ( GtkWidget *self, gboolean labels );
void taskbar_shell_set_group_icons ( GtkWidget *self, gboolean icons );
void taskbar_shell_set_group_labels ( GtkWidget *self, gboolean labels );
void taskbar_shell_set_group_cols ( GtkWidget *self, gint );
void taskbar_shell_set_group_rows ( GtkWidget *self, gint );
void taskbar_shell_set_group_sort ( GtkWidget *self, gboolean );
void taskbar_shell_set_group_title_width ( GtkWidget *self, gint );
void taskbar_shell_set_group_style ( GtkWidget *self, gchar * );
void taskbar_shell_set_group_css ( GtkWidget *self, gchar * );

#endif

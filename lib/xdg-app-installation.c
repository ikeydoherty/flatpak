/*
 * Copyright © 2015 Red Hat, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *       Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"

#include <string.h>

#include "libgsystem.h"
#include "xdg-app-utils.h"
#include "xdg-app-installation.h"
#include "xdg-app-installed-ref-private.h"
#include "xdg-app-remote-private.h"
#include "xdg-app-remote-ref-private.h"
#include "xdg-app-enum-types.h"
#include "xdg-app-dir.h"
#include "xdg-app-run.h"
#include "xdg-app-error.h"

/**
 * SECTION:xdg-app-installation
 * @Title: FlatpakInstallation
 * @Short_description: Installation information
 *
 * FlatpakInstallation is the toplevel object that software installers
 * should use to operate on an xdg-apps.
 *
 * An FlatpakInstallation object provides information about an installation
 * location for xdg-app applications. Typical installation locations are either
 * system-wide (in /var/lib/xdg-app) or per-user (in ~/.local/share/xdg-app).
 *
 * FlatpakInstallation can list configured remotes as well as installed application
 * and runtime references (in short: refs). It can also run, install, update and
 * uninstall applications and runtimes.
 */

typedef struct _FlatpakInstallationPrivate FlatpakInstallationPrivate;

struct _FlatpakInstallationPrivate
{
  FlatpakDir *dir;
};

G_DEFINE_TYPE_WITH_PRIVATE (FlatpakInstallation, flatpak_installation, G_TYPE_OBJECT)

enum {
  PROP_0,
};

static void
flatpak_installation_finalize (GObject *object)
{
  FlatpakInstallation *self = FLATPAK_INSTALLATION (object);
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  g_object_unref (priv->dir);

  G_OBJECT_CLASS (flatpak_installation_parent_class)->finalize (object);
}

static void
flatpak_installation_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
flatpak_installation_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
flatpak_installation_class_init (FlatpakInstallationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = flatpak_installation_get_property;
  object_class->set_property = flatpak_installation_set_property;
  object_class->finalize = flatpak_installation_finalize;

}

static void
flatpak_installation_init (FlatpakInstallation *self)
{
}

static FlatpakInstallation *
flatpak_installation_new_for_dir (FlatpakDir   *dir,
                                  GCancellable *cancellable,
                                  GError      **error)
{
  FlatpakInstallation *self;
  FlatpakInstallationPrivate *priv;

  if (!flatpak_dir_ensure_repo (dir, NULL, error))
    {
      g_object_unref (dir);
      return NULL;
    }

  self = g_object_new (FLATPAK_TYPE_INSTALLATION, NULL);
  priv = flatpak_installation_get_instance_private (self);

  priv->dir = dir;

  return self;
}

/**
 * flatpak_get_default_arch:
 *
 * Returns the canonical name for the arch of the current machine.
 *
 * Returns: an arch string
 */
const char  *
flatpak_get_default_arch (void)
{
  return flatpak_get_arch ();
}

/**
 * flatpak_installation_new_system:
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Creates a new #FlatpakInstallation for the system-wide installation.
 *
 * Returns: (transfer full): a new #FlatpakInstallation
 */
FlatpakInstallation *
flatpak_installation_new_system (GCancellable *cancellable,
                                 GError      **error)
{
  return flatpak_installation_new_for_dir (flatpak_dir_get_system (), cancellable, error);
}

/**
 * flatpak_installation_new_user:
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Creates a new #FlatpakInstallation for the per-user installation.
 *
 * Returns: (transfer full): a new #FlatpakInstallation
 */
FlatpakInstallation *
flatpak_installation_new_user (GCancellable *cancellable,
                               GError      **error)
{
  return flatpak_installation_new_for_dir (flatpak_dir_get_user (), cancellable, error);
}

/**
 * flatpak_installation_new_for_path:
 * @path: a #GFile
 * @user: whether this is a user-specific location
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Creates a new #FlatpakInstallation for the installation at the given @path.
 *
 * Returns: (transfer full): a new #FlatpakInstallation
 */
FlatpakInstallation *
flatpak_installation_new_for_path (GFile *path, gboolean user,
                                   GCancellable *cancellable,
                                   GError **error)
{
  return flatpak_installation_new_for_dir (flatpak_dir_new (path, user), cancellable, error);
}

/**
 * flatpak_installation_get_is_user:
 * @self: a #FlatpakInstallation
 *
 * Returns whether the installation is for a user-specific location.
 *
 * Returns: %TRUE if @self is a per-user installation
 */
gboolean
flatpak_installation_get_is_user (FlatpakInstallation *self)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  return flatpak_dir_is_user (priv->dir);
}

/**
 * flatpak_installation_get_path:
 * @self: a #FlatpakInstallation
 *
 * Returns the installation location for @self.
 *
 * Returns: (transfer full): an #GFile
 */
GFile *
flatpak_installation_get_path (FlatpakInstallation *self)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  return g_object_ref (flatpak_dir_get_path (priv->dir));
}

/**
 * flatpak_installation_launch:
 * @self: a #FlatpakInstallation
 * @name: name of the app to launch
 * @arch: (nullable): which architecture to launch (default: current architecture)
 * @branch: (nullable): which branch of the application (default: "master")
 * @commit: (nullable): the commit of @branch to launch
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Launch an installed application.
 *
 * You can use flatpak_installation_get_installed_ref() or
 * flatpak_installation_get_current_installed_app() to find out what builds
 * are available, in order to get a value for @commit.
 *
 * Returns: %TRUE, unless an error occurred
 */
gboolean
flatpak_installation_launch (FlatpakInstallation *self,
                             const char          *name,
                             const char          *arch,
                             const char          *branch,
                             const char          *commit,
                             GCancellable        *cancellable,
                             GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);
  g_autofree char *app_ref = NULL;

  g_autoptr(FlatpakDeploy) app_deploy = NULL;

  app_ref =
    flatpak_build_app_ref (name, branch, arch);

  app_deploy =
    flatpak_dir_load_deployed (priv->dir, app_ref,
                               commit,
                               cancellable, error);
  if (app_deploy == NULL)
    return FALSE;

  return flatpak_run_app (app_ref,
                          app_deploy,
                          NULL, NULL,
                          NULL,
                          FLATPAK_RUN_FLAG_BACKGROUND,
                          NULL,
                          NULL, 0,
                          cancellable, error);
}


static FlatpakInstalledRef *
get_ref (FlatpakInstallation *self,
         const char          *full_ref,
         GCancellable        *cancellable)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  g_auto(GStrv) parts = NULL;
  const char *origin = NULL;
  const char *commit = NULL;
  g_autoptr(GFile) deploy_dir = NULL;
  g_autoptr(GFile) deploy_subdir = NULL;
  g_autofree char *deploy_path = NULL;
  g_autofree char *latest_commit = NULL;
  g_autoptr(GVariant) deploy_data = NULL;
  g_autofree const char **subpaths = NULL;
  gboolean is_current = FALSE;
  guint64 installed_size = 0;

  parts = g_strsplit (full_ref, "/", -1);

  deploy_data = flatpak_dir_get_deploy_data (priv->dir, full_ref, cancellable, NULL);
  origin = flatpak_deploy_data_get_origin (deploy_data);
  commit = flatpak_deploy_data_get_commit (deploy_data);
  subpaths = flatpak_deploy_data_get_subpaths (deploy_data);
  installed_size = flatpak_deploy_data_get_installed_size (deploy_data);

  deploy_dir = flatpak_dir_get_deploy_dir (priv->dir, full_ref);
  deploy_subdir = g_file_get_child (deploy_dir, commit);
  deploy_path = g_file_get_path (deploy_subdir);

  if (strcmp (parts[0], "app") == 0)
    {
      g_autofree char *current =
        flatpak_dir_current_ref (priv->dir, parts[1], cancellable);
      if (current && strcmp (full_ref, current) == 0)
        is_current = TRUE;
    }

  latest_commit = flatpak_dir_read_latest (priv->dir, origin, full_ref, NULL, NULL);

  return flatpak_installed_ref_new (full_ref,
                                    commit,
                                    latest_commit,
                                    origin, subpaths,
                                    deploy_path,
                                    installed_size,
                                    is_current);
}

/**
 * flatpak_installation_get_installed_ref:
 * @self: a #FlatpakInstallation
 * @kind: whether this is an app or runtime
 * @name: name of the app/runtime to fetch
 * @arch: (nullable): which architecture to fetch (default: current architecture)
 * @branch: (nullable): which branch to fetch (default: "master")
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Returns information about an installed ref, such as the available builds,
 * its size, location, etc.
 *
 * Returns: (transfer full): an #FlatpakInstalledRef, or %NULL if an error occurred
 */
FlatpakInstalledRef *
flatpak_installation_get_installed_ref (FlatpakInstallation *self,
                                        FlatpakRefKind       kind,
                                        const char          *name,
                                        const char          *arch,
                                        const char          *branch,
                                        GCancellable        *cancellable,
                                        GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  g_autoptr(GFile) deploy = NULL;
  g_autofree char *ref = NULL;

  if (arch == NULL)
    arch = flatpak_get_arch ();

  if (kind == FLATPAK_REF_KIND_APP)
    ref = flatpak_build_app_ref (name, branch, arch);
  else
    ref = flatpak_build_runtime_ref (name, branch, arch);


  deploy = flatpak_dir_get_if_deployed (priv->dir,
                                        ref, NULL, cancellable);
  if (deploy == NULL)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                   "Ref %s no installed", ref);
      return NULL;
    }

  return get_ref (self, ref, cancellable);
}

/**
 * flatpak_installation_get_current_installed_app:
 * @self: a #FlatpakInstallation
 * @name: the name of the app
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Get the last build of reference @name that was installed with
 * flatpak_installation_install(), or %NULL if the reference has
 * never been installed locally.
 *
 * Returns: (transfer full): an #FlatpakInstalledRef
 */
FlatpakInstalledRef *
flatpak_installation_get_current_installed_app (FlatpakInstallation *self,
                                                const char          *name,
                                                GCancellable        *cancellable,
                                                GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  g_autoptr(GFile) deploy = NULL;
  g_autofree char *current =
    flatpak_dir_current_ref (priv->dir, name, cancellable);

  if (current)
    deploy = flatpak_dir_get_if_deployed (priv->dir,
                                          current, NULL, cancellable);

  if (deploy == NULL)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                   "App %s no installed", name);
      return NULL;
    }

  return get_ref (self, current, cancellable);
}

/**
 * flatpak_installation_list_installed_refs:
 * @self: a #FlatpakInstallation
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Lists the installed references.
 *
 * Returns: (transfer container) (element-type FlatpakInstalledRef): an GPtrArray of
 *   #FlatpakInstalledRef instances
 */
GPtrArray *
flatpak_installation_list_installed_refs (FlatpakInstallation *self,
                                          GCancellable        *cancellable,
                                          GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  g_auto(GStrv) raw_refs_app = NULL;
  g_auto(GStrv) raw_refs_runtime = NULL;
  g_autoptr(GPtrArray) refs = g_ptr_array_new_with_free_func (g_object_unref);
  int i;

  if (!flatpak_dir_list_refs (priv->dir,
                              "app",
                              &raw_refs_app,
                              cancellable, error))
    return NULL;

  for (i = 0; raw_refs_app[i] != NULL; i++)
    g_ptr_array_add (refs,
                     get_ref (self, raw_refs_app[i], cancellable));

  if (!flatpak_dir_list_refs (priv->dir,
                              "runtime",
                              &raw_refs_runtime,
                              cancellable, error))
    return NULL;

  for (i = 0; raw_refs_runtime[i] != NULL; i++)
    g_ptr_array_add (refs,
                     get_ref (self, raw_refs_runtime[i], cancellable));

  return g_steal_pointer (&refs);
}

/**
 * flatpak_installation_list_installed_refs_by_kind:
 * @self: a #FlatpakInstallation
 * @kind: the kind of installation
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Lists the installed references of a specific kind.
 *
 * Returns: (transfer container) (element-type FlatpakInstalledRef): an GPtrArray of
 *   #FlatpakInstalledRef instances
 */
GPtrArray *
flatpak_installation_list_installed_refs_by_kind (FlatpakInstallation *self,
                                                  FlatpakRefKind       kind,
                                                  GCancellable        *cancellable,
                                                  GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  g_auto(GStrv) raw_refs = NULL;
  g_autoptr(GPtrArray) refs = g_ptr_array_new_with_free_func (g_object_unref);
  int i;

  if (!flatpak_dir_list_refs (priv->dir,
                              kind == FLATPAK_REF_KIND_APP ? "app" : "runtime",
                              &raw_refs,
                              cancellable, error))
    return NULL;

  for (i = 0; raw_refs[i] != NULL; i++)
    g_ptr_array_add (refs,
                     get_ref (self, raw_refs[i], cancellable));

  return g_steal_pointer (&refs);
}

/**
 * flatpak_installation_list_installed_refs_for_update:
 * @self: a #FlatpakInstallation
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Lists the installed references that has a remote update that is not
 * locally available. However, even though an app is not returned by this
 * it can have local updates available that has not been deployed. Look
 * at commit vs latest_commit on installed apps for this.
 *
 * Returns: (transfer container) (element-type FlatpakInstalledRef): an GPtrArray of
 *   #FlatpakInstalledRef instances
 */
GPtrArray *
flatpak_installation_list_installed_refs_for_update (FlatpakInstallation *self,
                                                     GCancellable        *cancellable,
                                                     GError             **error)
{
  g_autoptr(GPtrArray) updates = NULL;
  g_autoptr(GPtrArray) installed = NULL;
  g_autoptr(GPtrArray) remotes = NULL;
  g_autoptr(GHashTable) ht = NULL;
  int i, j;

  ht = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  remotes = flatpak_installation_list_remotes (self, cancellable, error);
  if (remotes == NULL)
    return NULL;

  for (i = 0; i < remotes->len; i++)
    {
      FlatpakRemote *remote = g_ptr_array_index (remotes, i);
      g_autoptr(GPtrArray) refs = NULL;
      g_autoptr(GError) local_error = NULL;

      /* We ignore errors here. we don't want one remote to fail us */
      refs = flatpak_installation_list_remote_refs_sync (self,
                                                         flatpak_remote_get_name (remote),
                                                         cancellable, &local_error);
      if (refs != NULL)
        {
          for (j = 0; j < refs->len; j++)
            {
              FlatpakRemoteRef *remote_ref = g_ptr_array_index (refs, j);
              g_autofree char *full_ref = flatpak_ref_format_ref (FLATPAK_REF (remote_ref));
              g_autofree char *key = g_strdup_printf ("%s:%s", flatpak_remote_get_name (remote),
                                                      full_ref);

              g_hash_table_insert (ht, g_steal_pointer (&key),
                                   g_strdup (flatpak_ref_get_commit (FLATPAK_REF (remote_ref))));
            }
        }
      else
        {
          g_debug ("Update: Failed to read remote %s: %s\n",
                   flatpak_remote_get_name (remote),
                   local_error->message);
        }
    }

  installed = flatpak_installation_list_installed_refs (self, cancellable, error);
  if (installed == NULL)
    return NULL;

  updates = g_ptr_array_new_with_free_func (g_object_unref);

  for (i = 0; i < installed->len; i++)
    {
      FlatpakInstalledRef *installed_ref = g_ptr_array_index (installed, i);
      g_autofree char *full_ref = flatpak_ref_format_ref (FLATPAK_REF (installed_ref));
      g_autofree char *key = g_strdup_printf ("%s:%s", flatpak_installed_ref_get_origin (installed_ref),
                                              full_ref);
      const char *remote_ref = g_hash_table_lookup (ht, key);

      if (remote_ref != NULL &&
          g_strcmp0 (remote_ref,
                     flatpak_installed_ref_get_latest_commit (installed_ref)) != 0)
        g_ptr_array_add (updates, g_object_ref (installed_ref));
    }

  return g_steal_pointer (&updates);
}


/**
 * flatpak_installation_list_remotes:
 * @self: a #FlatpakInstallation
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Lists the remotes, in priority (highest first) order. For same priority,
 * an earlier added remote comes before a later added one.
 *
 * Returns: (transfer container) (element-type FlatpakRemote): an GPtrArray of
 *   #FlatpakRemote instances
 */
GPtrArray *
flatpak_installation_list_remotes (FlatpakInstallation *self,
                                   GCancellable        *cancellable,
                                   GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  g_auto(GStrv) remote_names = NULL;
  g_autoptr(GPtrArray) remotes = g_ptr_array_new_with_free_func (g_object_unref);
  int i;

  remote_names = flatpak_dir_list_remotes (priv->dir, cancellable, error);
  if (remote_names == NULL)
    return NULL;

  for (i = 0; remote_names[i] != NULL; i++)
    g_ptr_array_add (remotes,
                     flatpak_remote_new (priv->dir, remote_names[i]));

  return g_steal_pointer (&remotes);
}

/**
 * flatpak_installation_get_remote_by_name:
 * @self: a #FlatpakInstallation
 * @name: a remote name
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Looks up a remote by name.
 *
 * Returns: (transfer full): a #FlatpakRemote instances, or %NULL error
 */
FlatpakRemote *
flatpak_installation_get_remote_by_name (FlatpakInstallation *self,
                                         const gchar         *name,
                                         GCancellable        *cancellable,
                                         GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  g_auto(GStrv) remote_names = NULL;
  int i;

  remote_names = flatpak_dir_list_remotes (priv->dir, cancellable, error);
  if (remote_names == NULL)
    return NULL;

  for (i = 0; remote_names[i] != NULL; i++)
    {
      if (strcmp (remote_names[i], name) == 0)
        return flatpak_remote_new (priv->dir, remote_names[i]);
    }

  g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
               "No remote named '%s'", name);
  return NULL;
}

/**
 * flatpak_installation_load_app_overrides:
 * @self: a #FlatpakInstallation
 * @app_id: an application id
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Loads the metadata overrides file for an application.
 *
 * Returns: (transfer full): the contents of the overrides files,
 *    or %NULL if an error occurred
 */
char *
flatpak_installation_load_app_overrides (FlatpakInstallation *self,
                                         const char          *app_id,
                                         GCancellable        *cancellable,
                                         GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);
  g_autofree char *metadata_contents = NULL;
  gsize metadata_size;

  metadata_contents = flatpak_dir_load_override (priv->dir, app_id, &metadata_size, error);
  if (metadata_contents == NULL)
    return NULL;

  return metadata_contents;
}

static void
progress_cb (OstreeAsyncProgress *progress, gpointer user_data)
{
  FlatpakProgressCallback progress_cb = g_object_get_data (G_OBJECT (progress), "callback");
  guint last_progress = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (progress), "last_progress"));
  GString *buf;
  g_autofree char *status = NULL;
  guint outstanding_fetches;
  guint outstanding_metadata_fetches;
  guint outstanding_writes;
  guint n_scanned_metadata;
  guint fetched_delta_parts;
  guint total_delta_parts;
  guint64 bytes_transferred;
  guint64 total_delta_part_size;
  guint fetched;
  guint metadata_fetched;
  guint requested;
  guint64 ellapsed_time;
  guint new_progress = 0;
  gboolean estimating = FALSE;

  buf = g_string_new ("");

  status = ostree_async_progress_get_status (progress);
  outstanding_fetches = ostree_async_progress_get_uint (progress, "outstanding-fetches");
  outstanding_metadata_fetches = ostree_async_progress_get_uint (progress, "outstanding-metadata-fetches");
  outstanding_writes = ostree_async_progress_get_uint (progress, "outstanding-writes");
  n_scanned_metadata = ostree_async_progress_get_uint (progress, "scanned-metadata");
  fetched_delta_parts = ostree_async_progress_get_uint (progress, "fetched-delta-parts");
  total_delta_parts = ostree_async_progress_get_uint (progress, "total-delta-parts");
  total_delta_part_size = ostree_async_progress_get_uint64 (progress, "total-delta-part-size");
  bytes_transferred = ostree_async_progress_get_uint64 (progress, "bytes-transferred");
  fetched = ostree_async_progress_get_uint (progress, "fetched");
  metadata_fetched = ostree_async_progress_get_uint (progress, "metadata-fetched");
  requested = ostree_async_progress_get_uint (progress, "requested");
  ellapsed_time = (g_get_monotonic_time () - ostree_async_progress_get_uint64 (progress, "start-time")) / G_USEC_PER_SEC;

  if (status)
    {
      g_string_append (buf, status);
    }
  else if (outstanding_fetches)
    {
      guint64 bytes_sec = bytes_transferred / ellapsed_time;
      g_autofree char *formatted_bytes_transferred =
        g_format_size_full (bytes_transferred, 0);
      g_autofree char *formatted_bytes_sec = NULL;

      if (!bytes_sec) // Ignore first second
        formatted_bytes_sec = g_strdup ("-");
      else
        formatted_bytes_sec = g_format_size (bytes_sec);

      if (total_delta_parts > 0)
        {
          g_autofree char *formatted_total =
            g_format_size (total_delta_part_size);
          g_string_append_printf (buf, "Receiving delta parts: %u/%u %s/s %s/%s",
                                  fetched_delta_parts, total_delta_parts,
                                  formatted_bytes_sec, formatted_bytes_transferred,
                                  formatted_total);

          new_progress = (100 * bytes_transferred) / total_delta_part_size;
        }
      else if (outstanding_metadata_fetches)
        {
          /* At this point we don't really know how much data there is, so we have to make a guess.
           * Since its really hard to figure out early how much data there is we report 1% until
           * all objects are scanned. */

          new_progress = 1;
          estimating = TRUE;

          g_string_append_printf (buf, "Receiving metadata objects: %u/(estimating) %s/s %s",
                                  metadata_fetched, formatted_bytes_sec, formatted_bytes_transferred);
        }
      else
        {
          new_progress = (100 * fetched) / requested;
          g_string_append_printf (buf, "Receiving objects: %u%% (%u/%u) %s/s %s",
                                  (guint) ((((double) fetched) / requested) * 100),
                                  fetched, requested, formatted_bytes_sec, formatted_bytes_transferred);
        }
    }
  else if (outstanding_writes)
    {
      g_string_append_printf (buf, "Writing objects: %u", outstanding_writes);
    }
  else
    {
      g_string_append_printf (buf, "Scanning metadata: %u", n_scanned_metadata);
    }

  if (new_progress < last_progress)
    new_progress = last_progress;
  g_object_set_data (G_OBJECT (progress), "last_progress", GUINT_TO_POINTER (new_progress));

  progress_cb (buf->str, new_progress, estimating, user_data);

  g_string_free (buf, TRUE);
}

/**
 * flatpak_installation_install_bundle:
 * @self: a #FlatpakInstallation
 * @file: a #GFile that is an xdg-app bundle
 * @progress: (scope call): progress callback
 * @progress_data: user data passed to @progress
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Install an application or runtime from an xdg-app bundle file.
 * See xdg-app-build-bundle(1) for how to create brundles.
 *
 * Returns: (transfer full): The ref for the newly installed app or %NULL on failure
 */
FlatpakInstalledRef *
flatpak_installation_install_bundle (FlatpakInstallation    *self,
                                     GFile                  *file,
                                     FlatpakProgressCallback progress,
                                     gpointer                progress_data,
                                     GCancellable           *cancellable,
                                     GError                **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);
  g_autofree char *ref = NULL;
  gboolean added_remote = FALSE;

  g_autoptr(GFile) deploy_base = NULL;
  g_autoptr(FlatpakDir) dir_clone = NULL;
  FlatpakInstalledRef *result = NULL;
  g_autoptr(GError) local_error = NULL;
  g_auto(GLnxLockFile) lock = GLNX_LOCK_FILE_INIT;
  g_autoptr(GVariant) metadata = NULL;
  g_autofree char *origin = NULL;
  g_auto(GStrv) parts = NULL;
  g_autofree char *basename = NULL;
  g_autoptr(GBytes) gpg_data = NULL;
  g_autofree char *to_checksum = NULL;
  g_autofree char *remote = NULL;

  metadata = flatpak_bundle_load (file, &to_checksum,
                                  &ref,
                                  &origin,
                                  NULL,
                                  &gpg_data,
                                  error);
  if (metadata == NULL)
    return FALSE;

  parts = flatpak_decompose_ref (ref, error);
  if (parts == NULL)
    return FALSE;

  deploy_base = flatpak_dir_get_deploy_dir (priv->dir, ref);

  if (g_file_query_exists (deploy_base, cancellable))
    {
      g_set_error (error,
                   FLATPAK_ERROR, FLATPAK_ERROR_ALREADY_INSTALLED,
                   "%s branch %s already installed", parts[1], parts[3]);
      return NULL;
    }

  /* Add a remote for later updates */
  basename = g_file_get_basename (file);
  remote = flatpak_dir_create_origin_remote (priv->dir,
                                             origin,
                                             parts[1],
                                             basename,
                                             gpg_data,
                                             cancellable,
                                             error);
  if (remote == NULL)
    return FALSE;

  /* From here we need to goto out on error, to clean up */
  added_remote = TRUE;

  /* Pull, prune, etc are not threadsafe, so we work on a copy */
  dir_clone = flatpak_dir_clone (priv->dir);

  if (!flatpak_dir_ensure_repo (dir_clone, cancellable, error))
    goto out;

  if (!flatpak_pull_from_bundle (flatpak_dir_get_repo (dir_clone),
                                 file,
                                 remote,
                                 ref,
                                 gpg_data != NULL,
                                 cancellable,
                                 error))
    goto out;

  if (!flatpak_dir_deploy_install (dir_clone, ref, remote, NULL, cancellable, error))
    goto out;

  result = get_ref (self, ref, cancellable);

out:

  if (added_remote && result == NULL)
    ostree_repo_remote_delete (flatpak_dir_get_repo (priv->dir), remote, NULL, NULL);

  return result;
}

/**
 * flatpak_installation_install:
 * @self: a #FlatpakInstallation
 * @remote_name: name of the remote to use
 * @kind: what this ref contains (an #FlatpakRefKind)
 * @name: name of the app/runtime to fetch
 * @arch: (nullable): which architecture to fetch (default: current architecture)
 * @branch: (nullable): which branch to fetch (default: 'master')
 * @progress: (scope call): progress callback
 * @progress_data: user data passed to @progress
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Install a new application or runtime.
 *
 * Returns: (transfer full): The ref for the newly installed app or %NULL on failure
 */
FlatpakInstalledRef *
flatpak_installation_install (FlatpakInstallation    *self,
                              const char             *remote_name,
                              FlatpakRefKind          kind,
                              const char             *name,
                              const char             *arch,
                              const char             *branch,
                              FlatpakProgressCallback progress,
                              gpointer                progress_data,
                              GCancellable           *cancellable,
                              GError                **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);
  g_autofree char *ref = NULL;

  g_autoptr(GFile) deploy_base = NULL;
  g_autoptr(FlatpakDir) dir_clone = NULL;
  g_autoptr(GMainContext) main_context = NULL;
  g_autoptr(OstreeAsyncProgress) ostree_progress = NULL;
  FlatpakInstalledRef *result = NULL;
  g_autoptr(GError) local_error = NULL;
  g_auto(GLnxLockFile) lock = GLNX_LOCK_FILE_INIT;

  ref = flatpak_compose_ref (kind == FLATPAK_REF_KIND_APP, name, branch, arch, error);
  if (ref == NULL)
    return NULL;

  deploy_base = flatpak_dir_get_deploy_dir (priv->dir, ref);
  if (g_file_query_exists (deploy_base, cancellable))
    {
      g_set_error (error,
                   FLATPAK_ERROR, FLATPAK_ERROR_ALREADY_INSTALLED,
                   "%s branch %s already installed", name, branch ? branch : "master");
      goto out;
    }

  /* Pull, prune, etc are not threadsafe, so we work on a copy */
  dir_clone = flatpak_dir_clone (priv->dir);

  /* Work around ostree-pull spinning the default main context for the sync calls */
  main_context = g_main_context_new ();
  g_main_context_push_thread_default (main_context);

  if (progress)
    {
      ostree_progress = ostree_async_progress_new_and_connect (progress_cb, progress_data);
      g_object_set_data (G_OBJECT (ostree_progress), "callback", progress);
      g_object_set_data (G_OBJECT (ostree_progress), "last_progress", GUINT_TO_POINTER (0));
    }

  if (!flatpak_dir_install (dir_clone, FALSE, FALSE, ref, remote_name, NULL,
                            ostree_progress, cancellable, error))
    goto out;

  result = get_ref (self, ref, cancellable);

out:
  if (main_context)
    g_main_context_pop_thread_default (main_context);

  if (ostree_progress)
    ostree_async_progress_finish (ostree_progress);

  return result;
}

/**
 * flatpak_installation_update:
 * @self: a #FlatpakInstallation
 * @flags: an #FlatpakUpdateFlags variable
 * @kind: whether this is an app or runtime
 * @name: name of the app or runtime to update
 * @arch: (nullable): architecture of the app or runtime to update (default: current architecture)
 * @branch: (nullable): name of the branch of the app or runtime to update (default: master)
 * @progress: (scope call): the callback
 * @progress_data: user data passed to @progress
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Update an application or runtime.
 *
 * Returns: (transfer full): The ref for the newly updated app (or the same if no update) or %NULL on failure
 */
FlatpakInstalledRef *
flatpak_installation_update (FlatpakInstallation    *self,
                             FlatpakUpdateFlags      flags,
                             FlatpakRefKind          kind,
                             const char             *name,
                             const char             *arch,
                             const char             *branch,
                             FlatpakProgressCallback progress,
                             gpointer                progress_data,
                             GCancellable           *cancellable,
                             GError                **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);
  g_autofree char *ref = NULL;

  g_autoptr(GFile) deploy_base = NULL;
  g_autoptr(FlatpakDir) dir_clone = NULL;
  g_autoptr(GMainContext) main_context = NULL;
  g_autoptr(OstreeAsyncProgress) ostree_progress = NULL;
  g_autofree char *remote_name = NULL;
  FlatpakInstalledRef *result = NULL;
  g_auto(GStrv) subpaths = NULL;

  ref = flatpak_compose_ref (kind == FLATPAK_REF_KIND_APP, name, branch, arch, error);
  if (ref == NULL)
    return NULL;

  deploy_base = flatpak_dir_get_deploy_dir (priv->dir, ref);
  if (!g_file_query_exists (deploy_base, cancellable))
    {
      g_set_error (error,
                   FLATPAK_ERROR, FLATPAK_ERROR_NOT_INSTALLED,
                   "%s branch %s is not installed", name, branch ? branch : "master");
      return NULL;
    }

  remote_name = flatpak_dir_get_origin (priv->dir, ref, cancellable, error);
  if (remote_name == NULL)
    return NULL;

  subpaths = flatpak_dir_get_subpaths (priv->dir, ref, cancellable, error);
  if (subpaths == NULL)
    return FALSE;

  /* Pull, prune, etc are not threadsafe, so we work on a copy */
  dir_clone = flatpak_dir_clone (priv->dir);

  /* Work around ostree-pull spinning the default main context for the sync calls */
  main_context = g_main_context_new ();
  g_main_context_push_thread_default (main_context);

  if (progress)
    {
      ostree_progress = ostree_async_progress_new_and_connect (progress_cb, progress_data);
      g_object_set_data (G_OBJECT (ostree_progress), "callback", progress);
      g_object_set_data (G_OBJECT (ostree_progress), "last_progress", GUINT_TO_POINTER (0));
    }

  if (!flatpak_dir_update (dir_clone,
                           (flags & FLATPAK_UPDATE_FLAGS_NO_PULL) != 0,
                           (flags & FLATPAK_UPDATE_FLAGS_NO_DEPLOY) != 0,
                           remote_name, ref, NULL, subpaths,
                           ostree_progress, cancellable, error))
    goto out;

  result = get_ref (self, ref, cancellable);

out:
  if (main_context)
    g_main_context_pop_thread_default (main_context);

  if (ostree_progress)
    ostree_async_progress_finish (ostree_progress);

  return result;
}

/**
 * flatpak_installation_uninstall:
 * @self: a #FlatpakInstallation
 * @kind: what this ref contains (an #FlatpakRefKind)
 * @name: name of the app or runtime to uninstall
 * @arch: architecture of the app or runtime to uninstall
 * @branch: name of the branch of the app or runtime to uninstall
 * @progress: (scope call): the callback
 * @progress_data: user data passed to @progress
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Uninstall an application or runtime.
 *
 * Returns: %TRUE on success
 */
FLATPAK_EXTERN gboolean
flatpak_installation_uninstall (FlatpakInstallation    *self,
                                FlatpakRefKind          kind,
                                const char             *name,
                                const char             *arch,
                                const char             *branch,
                                FlatpakProgressCallback progress,
                                gpointer                progress_data,
                                GCancellable           *cancellable,
                                GError                **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);
  g_autofree char *ref = NULL;
  g_autofree char *remote_name = NULL;
  g_autofree char *current_ref = NULL;

  g_autoptr(GFile) deploy_base = NULL;
  g_autoptr(FlatpakDir) dir_clone = NULL;
  gboolean was_deployed = FALSE;
  g_auto(GLnxLockFile) lock = GLNX_LOCK_FILE_INIT;

  ref = flatpak_compose_ref (kind == FLATPAK_REF_KIND_APP, name, branch, arch, error);
  if (ref == NULL)
    return FALSE;

  /* prune, etc are not threadsafe, so we work on a copy */
  dir_clone = flatpak_dir_clone (priv->dir);

  if (!flatpak_dir_lock (dir_clone, &lock,
                         cancellable, error))
    return FALSE;

  deploy_base = flatpak_dir_get_deploy_dir (priv->dir, ref);
  if (!g_file_query_exists (deploy_base, cancellable))
    {
      g_set_error (error,
                   FLATPAK_ERROR, FLATPAK_ERROR_NOT_INSTALLED,
                   "%s branch %s is not installed", name, branch ? branch : "master");
      return FALSE;
    }

  remote_name = flatpak_dir_get_origin (priv->dir, ref, cancellable, error);
  if (remote_name == NULL)
    return FALSE;

  g_debug ("dropping active ref");
  if (!flatpak_dir_set_active (dir_clone, ref, NULL, cancellable, error))
    return FALSE;

  if (kind == FLATPAK_REF_KIND_APP)
    {
      current_ref = flatpak_dir_current_ref (dir_clone, name, cancellable);
      if (current_ref != NULL && strcmp (ref, current_ref) == 0)
        {
          g_debug ("dropping current ref");
          if (!flatpak_dir_drop_current_ref (dir_clone, name, cancellable, error))
            return FALSE;
        }
    }

  if (!flatpak_dir_undeploy_all (dir_clone, ref, FALSE, &was_deployed, cancellable, error))
    return FALSE;

  if (!flatpak_dir_remove_ref (dir_clone, remote_name, ref, cancellable, error))
    return FALSE;

  glnx_release_lock_file (&lock);

  if (!flatpak_dir_prune (dir_clone, cancellable, error))
    return FALSE;

  flatpak_dir_cleanup_removed (dir_clone, cancellable, NULL);

  if (kind == FLATPAK_REF_KIND_APP)
    {
      if (!flatpak_dir_update_exports (dir_clone, name, cancellable, error))
        return FALSE;
    }

  if (!flatpak_dir_mark_changed (dir_clone, error))
    return FALSE;

  if (!was_deployed)
    {
      g_set_error (error,
                   FLATPAK_ERROR, FLATPAK_ERROR_NOT_INSTALLED,
                   "%s branch %s is not installed", name, branch ? branch : "master");
      return FALSE;
    }

  return TRUE;
}

/**
 * flatpak_installation_fetch_remote_size_sync:
 * @self: a #FlatpakInstallation
 * @remote_name: the name of the remote
 * @commit: the commit
 * @download_size: (out): return location for the download size
 * @installed_size: (out): return location for the installed size
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Gets information about the amount of data that needs to be transferred
 * to pull a commit from a remote repository, and about the amount of
 * local disk space that is required to check out this commit.
 *
 * This is deprectated, use flatpak_installation_fetch_remote_size_sync2 instead.
 *
 * Returns: %TRUE, unless an error occurred
 */
gboolean
flatpak_installation_fetch_remote_size_sync (FlatpakInstallation *self,
                                             const char          *remote_name,
                                             const char          *commit,
                                             guint64             *download_size,
                                             guint64             *installed_size,
                                             GCancellable        *cancellable,
                                             GError             **error)
{
  return flatpak_fail (error, "Deprecated function call flatpak_installation_fetch_remote_size_sync");
}

/**
 * flatpak_installation_fetch_remote_size_sync2:
 * @self: a #FlatpakInstallation
 * @remote_name: the name of the remote
 * @ref: the ref
 * @download_size: (out): return location for the (maximum) download size
 * @installed_size: (out): return location for the installed size
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Gets information about the maximum amount of data that needs to be transferred
 * to pull the ref from a remote repository, and about the amount of
 * local disk space that is required to check out this commit.
 *
 * Note that if there are locally available data that are in the ref, which is commong
 * for instance if you're doing an update then the real download size may be smaller
 * than what is returned here.
 *
 * Returns: %TRUE, unless an error occurred
 */
gboolean
flatpak_installation_fetch_remote_size_sync2 (FlatpakInstallation *self,
                                              const char          *remote_name,
                                              FlatpakRef          *ref,
                                              guint64             *download_size,
                                              guint64             *installed_size,
                                              GCancellable        *cancellable,
                                              GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);
  g_autofree char *full_ref = flatpak_ref_format_ref (ref);

  return flatpak_dir_fetch_ref_cache (priv->dir, remote_name, full_ref,
                                      download_size, installed_size,
                                      NULL,
                                      cancellable,
                                      error);
}

/**
 * flatpak_installation_fetch_remote_metadata_sync:
 * @self: a #FlatpakInstallation
 * @remote_name: the name of the remote
 * @commit: the commit
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Obtains the metadata file from a commit.
 *
 * This is deprecated, use flatpak_installation_fetch_remote_metadata_sync2
 *
 * Returns: (transfer full): a #GBytes containing the xdg-app metadata file,
 *   or %NULL if an error occurred
 */
GBytes *
flatpak_installation_fetch_remote_metadata_sync (FlatpakInstallation *self,
                                                 const char          *remote_name,
                                                 const char          *commit,
                                                 GCancellable        *cancellable,
                                                 GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  g_autoptr(GBytes) bytes = NULL;

  bytes = flatpak_dir_fetch_metadata (priv->dir,
                                      remote_name,
                                      commit,
                                      cancellable,
                                      error);
  if (bytes == NULL)
    return NULL;

  return g_steal_pointer (&bytes);
}

/**
 * flatpak_installation_fetch_remote_metadata_sync2:
 * @self: a #FlatpakInstallation
 * @remote_name: the name of the remote
 * @ref: the ref
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Obtains the metadata file from a commit.
 *
 * Returns: (transfer full): a #GBytes containing the xdg-app metadata file,
 *   or %NULL if an error occurred
 */
GBytes *
flatpak_installation_fetch_remote_metadata_sync2 (FlatpakInstallation *self,
                                                  const char          *remote_name,
                                                  FlatpakRef          *ref,
                                                  GCancellable        *cancellable,
                                                  GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);
  g_autofree char *full_ref = flatpak_ref_format_ref (ref);
  char *res = NULL;

  if (!flatpak_dir_fetch_ref_cache (priv->dir, remote_name, full_ref,
                                    NULL, NULL,
                                    &res,
                                    cancellable, error))
    return NULL;

  return g_bytes_new_take (res, strlen (res));
}

/**
 * flatpak_installation_list_remote_refs_sync:
 * @self: a #FlatpakInstallation
 * @remote_name: the name of the remote
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Lists all the applications and runtimes in a remote.
 *
 * Returns: (transfer container) (element-type FlatpakRemoteRef): an GPtrArray of
 *   #FlatpakRemoteRef instances
 */
GPtrArray *
flatpak_installation_list_remote_refs_sync (FlatpakInstallation *self,
                                            const char          *remote_name,
                                            GCancellable        *cancellable,
                                            GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  g_autoptr(GPtrArray) refs = g_ptr_array_new_with_free_func (g_object_unref);
  g_autoptr(GHashTable) ht = NULL;
  GHashTableIter iter;
  gpointer key;
  gpointer value;

  if (!flatpak_dir_list_remote_refs (priv->dir,
                                     remote_name,
                                     &ht,
                                     cancellable,
                                     error))
    return NULL;

  g_hash_table_iter_init (&iter, ht);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      const char *refspec = key;
      const char *checksum = value;
      FlatpakRemoteRef *ref;

      ref = flatpak_remote_ref_new (refspec, checksum, remote_name);

      if (ref)
        g_ptr_array_add (refs, ref);
    }

  return g_steal_pointer (&refs);
}

/**
 * flatpak_installation_fetch_remote_ref_sync:
 * @self: a #FlatpakInstallation
 * @remote_name: the name of the remote
 * @kind: what this ref contains (an #FlatpakRefKind)
 * @name: name of the app/runtime to fetch
 * @arch: (nullable): which architecture to fetch (default: current architecture)
 * @branch: (nullable): which branch to fetch (default: 'master')
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Gets the current remote branch of a ref in the remote.
 *
 * Returns: (transfer full): a #FlatpakRemoteRef instance, or %NULL
 */
FlatpakRemoteRef *
flatpak_installation_fetch_remote_ref_sync (FlatpakInstallation *self,
                                            const char          *remote_name,
                                            FlatpakRefKind       kind,
                                            const char          *name,
                                            const char          *arch,
                                            const char          *branch,
                                            GCancellable        *cancellable,
                                            GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  g_autoptr(GHashTable) ht = NULL;
  g_autofree char *ref = NULL;
  const char *checksum;

  if (branch == NULL)
    branch = "master";

  if (!flatpak_dir_list_remote_refs (priv->dir,
                                     remote_name,
                                     &ht,
                                     cancellable,
                                     error))
    return NULL;

  if (kind == FLATPAK_REF_KIND_APP)
    ref = flatpak_build_app_ref (name,
                                 branch,
                                 arch);
  else
    ref = flatpak_build_runtime_ref (name,
                                     branch,
                                     arch);

  checksum = g_hash_table_lookup (ht, ref);

  if (checksum != NULL)
    return flatpak_remote_ref_new (ref, checksum, remote_name);

  g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
               "Reference %s doesn't exist in remote\n", ref);
  return NULL;
}

static void
no_progress_cb (OstreeAsyncProgress *progress, gpointer user_data)
{
}

/**
 * flatpak_installation_update_appstream_sync:
 * @self: a #FlatpakInstallation
 * @remote_name: the name of the remote
 * @arch: Architecture to update, or %NULL for the local machine arch
 * @out_changed: (nullable): Set to %TRUE if the contents of the appstream changed, %FALSE if nothing changed
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Updates the local copy of appstream for @remote_name for the specified @arch.
 *
 * Returns: %TRUE on success, or %FALSE on error
 */
gboolean
flatpak_installation_update_appstream_sync (FlatpakInstallation *self,
                                            const char          *remote_name,
                                            const char          *arch,
                                            gboolean            *out_changed,
                                            GCancellable        *cancellable,
                                            GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  g_autoptr(FlatpakDir) dir_clone = NULL;
  g_autoptr(OstreeAsyncProgress) ostree_progress = NULL;
  g_autoptr(GMainContext) main_context = NULL;
  gboolean res;

  /* Pull, prune, etc are not threadsafe, so we work on a copy */
  dir_clone = flatpak_dir_clone (priv->dir);

  if (main_context)
    g_main_context_pop_thread_default (main_context);

  /* Work around ostree-pull spinning the default main context for the sync calls */
  main_context = g_main_context_new ();
  g_main_context_push_thread_default (main_context);

  ostree_progress = ostree_async_progress_new_and_connect (no_progress_cb, NULL);

  res = flatpak_dir_update_appstream (dir_clone,
                                      remote_name,
                                      arch,
                                      out_changed,
                                      ostree_progress,
                                      cancellable,
                                      error);

  g_main_context_pop_thread_default (main_context);

  if (ostree_progress)
    ostree_async_progress_finish (ostree_progress);

  return res;
}

/**
 * flatpak_installation_create_monitor:
 * @self: a #FlatpakInstallation
 * @cancellable: (nullable): a #GCancellable
 * @error: return location for a #GError
 *
 * Gets monitor object for the installation. The returned file monitor will
 * emit the #GFileMonitor::changed signal whenever an application or runtime
 * was installed, uninstalled or updated.
 *
 * Returns: (transfer full): a new #GFileMonitor instance, or %NULL on error
 */
GFileMonitor *
flatpak_installation_create_monitor (FlatpakInstallation *self,
                                     GCancellable        *cancellable,
                                     GError             **error)
{
  FlatpakInstallationPrivate *priv = flatpak_installation_get_instance_private (self);

  g_autoptr(GFile) path = NULL;

  path = flatpak_dir_get_changed_path (priv->dir);

  return g_file_monitor_file (path, G_FILE_MONITOR_NONE,
                              cancellable, error);
}

/*
 * Copyright 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include <transfer/dm-source.h>

#include <click.h>
#include <ubuntu-app-launch.h>

#include <glib/gstdio.h>
#include <json-glib/json-glib.h>
#include <gio/gdesktopappinfo.h>

#include <algorithm>
#include <iostream>

namespace unity {
namespace indicator {
namespace transfer {

namespace {

static constexpr char const * DM_BUS_NAME {"com.canonical.applications.Downloader"};
static constexpr char const * DM_MANAGER_IFACE_NAME {"com.canonical.applications.DownloadManager"};
static constexpr char const * DM_DOWNLOAD_IFACE_NAME {"com.canonical.applications.Download"};

/**
 * A Transfer whose state comes from content-hub and ubuntu-download-manager.
 *
 * Each DMTransfer tracks a com.canonical.applications.Download (ccad) object
 * from ubuntu-download-manager. The ccad is used for pause/resume/cancel,
 * state change / download progress signals, etc.
 *
 */
class DMTransfer: public Transfer
{
public:

  DMTransfer(GDBusConnection* connection,
             const std::string& ccad_path):
    m_bus(G_DBUS_CONNECTION(g_object_ref(connection))),
    m_cancellable(g_cancellable_new()),
    m_ccad_path(ccad_path)
  {
    id = next_unique_id();
    time_started = time(nullptr);
    get_ccad_properties();
  }

  ~DMTransfer()
  {
    if (m_changed_tag)
      g_source_remove(m_changed_tag);

    g_cancellable_cancel(m_cancellable);
    g_clear_object(&m_cancellable);
    g_clear_object(&m_bus);
  }

  core::Signal<>& changed() { return m_changed; }

  void start()
  {
    g_return_if_fail(can_start());

    call_ccad_method_no_args_no_response("start");
  }

  void pause()
  {
    g_return_if_fail(can_pause());

    call_ccad_method_no_args_no_response("pause");
  }

  void resume()
  {
    g_return_if_fail(can_resume());

    call_ccad_method_no_args_no_response("resume");
  }

  void cancel()
  {
    call_ccad_method_no_args_no_response("cancel");
  }

  void open()
  {
      open_app();
  }

  void open_app()
  {
    // destination app has priority over app_id
    std::string app_id = download_app_id();

    if (app_id.empty() && !m_package_name.empty()) {
        app_id = std::string(ubuntu_app_launch_triplet_to_app_id(m_package_name.c_str(),
                                                                 "first-listed-app",
                                                                 "current-user-version"));
    }

    if (app_id.empty())
      {
        g_warning("Fail to discovery app-id");
      }
    else
      {
        g_debug("calling ubuntu_app_launch_start_application() for %s", app_id.c_str());
        ubuntu_app_launch_start_application(app_id.c_str(), nullptr);
      }
  }

  const std::string& ccad_path() const
  {
    return m_ccad_path;
  }

  void handle_ccad_signal(const gchar* signal_name, GVariant* parameters)
  {
    if (!g_strcmp0(signal_name, "started"))
      {
        if (get_signal_success_arg(parameters))
          set_state(RUNNING);
      }
    else if (!g_strcmp0(signal_name, "paused"))
      {
        if (get_signal_success_arg(parameters))
          set_state(PAUSED);
      }
    else if (!g_strcmp0(signal_name, "resumed"))
      {
        if (get_signal_success_arg(parameters))
          set_state(RUNNING);
      }
    else if (!g_strcmp0(signal_name, "canceled"))
      {
        if (get_signal_success_arg(parameters))
          set_state(CANCELED);
      }
    else if (!g_strcmp0(signal_name, "hashing"))
      {
        set_state(HASHING);
      }
    else if (!g_strcmp0(signal_name, "processing"))
      {
        set_state(PROCESSING);
      }
    else if (!g_strcmp0(signal_name, "finished"))
      {
        char* local_path = nullptr;
        g_variant_get_child(parameters, 0, "s", &local_path);
        set_local_path(local_path);
        g_free(local_path);

        set_state(FINISHED);
      }
    else if (!g_strcmp0(signal_name, "error"))
      {
        char* error_string = nullptr;
        g_variant_get_child(parameters, 0, "s", &error_string);
        set_error_string(error_string);
        set_state(ERROR);
        g_free(error_string);
      }
    else if (!g_strcmp0(signal_name, "progress"))
      {
        set_state(RUNNING);
        g_variant_get_child(parameters, 0, "t", &m_received);
        g_variant_get_child(parameters, 1, "t", &m_total_size);
        update_progress();
      }
    else if (!g_strcmp0(signal_name, "httpError") ||
             !g_strcmp0(signal_name, "networkError") ||
             !g_strcmp0(signal_name, "processsError"))
      {
        int32_t i;
        char* str = nullptr;
        g_variant_get_child(parameters, 0, "(is)", &i, &str);
        g_debug("%s setting error to '%s'", G_STRLOC, str);
        set_error_string(str);
        set_state(ERROR);
        g_free(str);
      }
    else
      {
        auto args = g_variant_print(parameters, true);
        g_warning("%s: unrecognized signal '%s': %s", G_STRLOC, signal_name, args);
        g_free(args);
      }
  }

private:

  const std::string& download_app_id() const
  {
       return m_app_id.empty() ? m_destination_app : m_app_id;
  }

  void emit_changed_soon()
  {
    if (m_changed_tag == 0)
        m_changed_tag = g_timeout_add_seconds(1, emit_changed_now, this);
  }

  static gboolean emit_changed_now(gpointer gself)
  {
    auto self = static_cast<DMTransfer*>(gself);
    self->m_changed_tag = 0;
    self->m_changed();
    return G_SOURCE_REMOVE;
  }

  /* The 'started', 'paused', 'resumed', and 'canceled' signals
     from com.canonical.applications.Download all have a single
     parameter, a boolean success flag. */
  bool get_signal_success_arg(GVariant* parameters)
  {
    gboolean success = false;
    g_return_val_if_fail(g_variant_is_container(parameters), false);
    g_return_val_if_fail(g_variant_n_children(parameters) == 1, false);
    g_variant_get_child(parameters, 0, "b", &success);
    return success;
  }

  uint64_t get_averaged_speed_Bps()
  {
    // limit the average to the last X samples
    static constexpr int max_slots = 50;
    if (m_history.size() > max_slots)
      m_history.erase(m_history.begin(), m_history.end()-max_slots);

    // limit the average to the last Y seconds
    static constexpr unsigned int max_age_seconds = 30;
    const auto oldest_allowed_usec = g_get_real_time() - (max_age_seconds * G_USEC_PER_SEC);
    const auto is_young = [oldest_allowed_usec](const DownloadProgress& p){return p.time_usec >= oldest_allowed_usec;};
    m_history.erase(std::begin(m_history), std::find_if(std::begin(m_history), std::end(m_history), is_young));

    uint64_t Bps;

    if (m_history.size() < 2)
      {
        Bps = 0;
      }
    else
      {
        const auto diff = m_history.back() - m_history.front();
        Bps = (diff.bytes * G_USEC_PER_SEC) / diff.time_usec;
      }

    return Bps;
  }

  void update_progress()
  {
    uint64_t tmp_total_size = 0;
    float tmp_progress = 0.0f;
    int tmp_seconds_left = 0;
    uint64_t tmp_speed_Bps = 0;

    if (m_total_size && m_received)
      {
        // update our speed tables
        m_history.push_back(DownloadProgress{g_get_real_time(),m_received});

        const auto Bps = get_averaged_speed_Bps();
        const int seconds = Bps ? (int)((m_total_size - m_received) / Bps) : -1;

        tmp_total_size = m_total_size;
        tmp_speed_Bps = Bps;
        tmp_progress = m_received / (float)m_total_size;
        tmp_seconds_left = seconds;
      }

    bool changed = false;

    if ((int)(progress*100) != (int)(tmp_progress*100))
      {
        progress = tmp_progress;
        changed = true;
      }

    if (seconds_left != tmp_seconds_left)
      {
        g_debug("changing '%s' seconds_left to '%d'", m_ccad_path.c_str(), (int)tmp_seconds_left);
        seconds_left = tmp_seconds_left;
        changed = true;
      }

    if (speed_Bps != tmp_speed_Bps)
      {
        speed_Bps = tmp_speed_Bps;
        changed = true;
      }

    if (total_size != tmp_total_size)
      {
        total_size = tmp_total_size;
        changed = true;
      }

    if (changed)
      emit_changed_soon();
  }

  void set_state(State state_in)
  {
    if (state != state_in)
      {
        state = state_in;

        if (!can_pause())
          {
            speed_Bps = 0;
            m_history.clear();
          }

        emit_changed_soon();
      }
  }

  void set_error_string(const char* str)
  {
    const std::string tmp = str ? str : "";
    if (error_string != tmp)
    {
      g_debug("changing '%s' error to '%s'", m_ccad_path.c_str(), tmp.c_str());
      error_string = tmp;
      emit_changed_soon();
    }
  }

  void set_local_path(const char* str)
  {
    const std::string tmp = str ? str : "";
    if (local_path != tmp)
      {
        g_debug("changing '%s' path to '%s'", m_ccad_path.c_str(), tmp.c_str());
        local_path = tmp;
        emit_changed_soon();
      }

    // If we don't already have a title,
    // use the file's basename as the title
    if (title.empty())
      {
        auto bname = g_path_get_basename(str);
        set_title(bname);
        g_free(bname);
      }
  }

  void set_title(const char* title_in)
  {
    const std::string tmp = title_in ? title_in : "";
    if (title != tmp)
      {
        g_debug("changing '%s' title to '%s'", m_ccad_path.c_str(), tmp.c_str());
        title = tmp;
        emit_changed_soon();
      }
  }

  void set_icon(const char* filename)
  {
    const std::string tmp = filename ? filename : "";
    if (app_icon != tmp)
      {
        g_debug("changing '%s' icon to '%s'", m_ccad_path.c_str(), tmp.c_str());
        app_icon = tmp;
        emit_changed_soon();
      }
  }

  /***
  ****  DownloadManager
  ***/

  void get_ccad_properties()
  {
    const auto bus_name = DM_BUS_NAME;
    const auto object_path = m_ccad_path.c_str();
    const auto interface_name = DM_DOWNLOAD_IFACE_NAME;

    g_dbus_connection_call(m_bus, bus_name, object_path, interface_name,
                           "totalSize", nullptr, G_VARIANT_TYPE("(t)"),
                           G_DBUS_CALL_FLAGS_NONE, -1,
                           m_cancellable, on_ccad_total_size, this);

    g_dbus_connection_call(m_bus, bus_name, object_path, interface_name,
                           "progress", nullptr, G_VARIANT_TYPE("(t)"),
                           G_DBUS_CALL_FLAGS_NONE, -1,
                           m_cancellable, on_ccad_progress, this);

    g_dbus_connection_call(m_bus, bus_name, object_path, interface_name,
                           "metadata", nullptr, G_VARIANT_TYPE("(a{sv})"),
                           G_DBUS_CALL_FLAGS_NONE, -1,
                           m_cancellable, on_ccad_metadata, this);

    g_dbus_connection_call(m_bus, bus_name, object_path,
                           "org.freedesktop.DBus.Properties",
                           "Get", g_variant_new ("(ss)", interface_name, "DestinationApp"),
                           G_VARIANT_TYPE ("(v)"),
                           G_DBUS_CALL_FLAGS_NONE, -1,
                           m_cancellable, on_ccad_destination_app, this);

    g_dbus_connection_call(m_bus, bus_name, object_path,
                           "org.freedesktop.DBus.Properties",
                           "Get", g_variant_new ("(ss)", interface_name, "Title"),
                           G_VARIANT_TYPE ("(v)"),
                           G_DBUS_CALL_FLAGS_NONE, -1,
                           m_cancellable, on_ccad_title, this);
  }

  static void on_ccad_total_size(GObject      * source,
                                 GAsyncResult * res,
                                 gpointer       gself)
  {
    auto v = connection_call_finish(source, res, "Error calling totalSize()");
    if (v != nullptr)
      {
        guint64 n = 0;
        g_variant_get_child(v, 0, "t", &n);
        g_variant_unref(v);

        auto self = static_cast<DMTransfer*>(gself);
        self->m_total_size = n;
        self->update_progress();
      }
  }

  static void on_ccad_progress(GObject      * source,
                               GAsyncResult * res,
                               gpointer       gself)
  {
    auto v = connection_call_finish(source, res, "Error calling progress()");
    if (v != nullptr)
      {
        guint64 n = 0;
        g_variant_get_child(v, 0, "t", &n);
        g_variant_unref(v);

        auto self = static_cast<DMTransfer*>(gself);
        self->m_received = n;
        self->update_progress();
      }
  }

  static void on_ccad_metadata(GObject      * source,
                               GAsyncResult * res,
                               gpointer       gself)
  {
    auto v = connection_call_finish(source, res, "Error calling metadata()");
    if (v != nullptr)
      {
        auto self = static_cast<DMTransfer*>(gself);
        GVariant *dict;
        GVariantIter iter;
        GVariant *value;
        gchar *key;

        dict = g_variant_get_child_value(v, 0);
        g_variant_iter_init (&iter, dict);

        self->m_app_id = "";
        self->m_package_name = "";

        while (g_variant_iter_next(&iter, "{sv}", &key, &value))
          {
            if (g_strcmp0(key, "app-id") == 0)
              {
                self->m_app_id = std::string(g_variant_get_string(value, nullptr));
              }

            // update-manager uses package-name
            if (g_strcmp0(key, "package-name") == 0)
              {
                self->m_package_name = std::string(g_variant_get_string(value, nullptr));
              }

             // must free data for ourselves
             g_variant_unref(value);
             g_free (key);
          }

        g_variant_unref(dict);
        g_debug("App id: %s", self->m_app_id.c_str());
        g_debug("Package name: %s", self->m_package_name.c_str());
        self->update_app_info();
      }
  }

  static void on_ccad_destination_app(GObject      * source,
                                      GAsyncResult * res,
                                      gpointer       gself)
  {
    auto v = connection_call_finish(source, res, "Error getting destinationApp property");
    if (v != nullptr)
      {
        GVariant *value, *item;
        item = g_variant_get_child_value(v, 0);
        value = g_variant_get_variant(item);
        g_variant_unref(item);

        auto self = static_cast<DMTransfer*>(gself);
        self->m_destination_app = std::string(g_variant_get_string(value, nullptr));
        g_debug("Destination app: %s", self->m_destination_app.c_str());
        self->update_app_info();
        g_variant_unref(v);
      }
  }

  static void on_ccad_title(GObject      * source,
                                      GAsyncResult * res,
                                      gpointer       gself)
  {
    auto v = connection_call_finish(source, res, "Error getting Title property");
    if (v != nullptr)
      {
        GVariant *value, *item;
        item = g_variant_get_child_value(v, 0);
        value = g_variant_get_variant(item);
        g_variant_unref(item);

        auto self = static_cast<DMTransfer*>(gself);
        auto title = g_variant_get_string(value, nullptr);
        g_debug("Download title: %s", title);
        if (title && strlen(title))
          self->set_title(title);
        g_variant_unref(v);
      }
  }

  void call_ccad_method_no_args_no_response(const char* method_name)
  {
    const auto bus_name = DM_BUS_NAME;
    const auto object_path = m_ccad_path.c_str();
    const auto interface_name = DM_DOWNLOAD_IFACE_NAME;

    g_debug("%s transfer %s calling '%s'", G_STRLOC, id.c_str(), method_name);

    g_dbus_connection_call(m_bus, bus_name, object_path, interface_name,
                           method_name, nullptr, nullptr,
                           G_DBUS_CALL_FLAGS_NONE, -1,
                           m_cancellable, nullptr, nullptr);
  }

  void update_app_info()
  {
    // destination app has priority over app_id
    std::string app_id = download_app_id();

    if (!app_id.empty())
      update_app_info_from_app_id(app_id);
    else if (!m_package_name.empty())
      update_app_info_from_package_name(m_package_name);
    else
      g_warning("Download without app-id or package-name");
  }

  void update_app_info_from_package_name(const std::string &package_name)
  {
    std::string app_id = std::string(ubuntu_app_launch_triplet_to_app_id(package_name.c_str(),
                                                                         "first-listed-app",
                                                                         "current-user-version"));
    if (!app_id.empty())
      update_app_info_from_app_id(app_id);
    else
      g_warning("fail to retrive app-id from package: %s", package_name.c_str());
  }

  void update_app_info_from_app_id(const std::string &app_id)
  {
    gchar *app_dir;
    gchar *app_desktop_file;

    if (!ubuntu_app_launch_application_info(app_id.c_str(), &app_dir, &app_desktop_file))
      {
        g_warning("Fail to get app info: %s", app_id.c_str());
        return;
      }

    g_debug("App data: %s : %s", app_dir, app_desktop_file);
    gchar *full_app_desktop_file = g_build_filename(app_dir, app_desktop_file, nullptr);
    GKeyFile *app_info = g_key_file_new();
    GError *error = nullptr;

    g_debug("Open desktop file: %s", full_app_desktop_file);
    g_key_file_load_from_file(app_info, full_app_desktop_file, G_KEY_FILE_NONE, &error);
    if (error)
      {
        g_warning("Fail to open desktop info: %s:%s", full_app_desktop_file, error->message);
        g_free(full_app_desktop_file);
        g_key_file_free(app_info);
        g_error_free(error);
      }
    else
      {
        gchar *icon_name = g_key_file_get_string(app_info, "Desktop Entry", "Icon", &error);
        if (error == nullptr)
          {

            gchar *full_icon_name = g_build_filename(app_dir, icon_name, nullptr);
            g_debug("App icon: %s", icon_name);
            g_debug("App full icon name: %s", full_icon_name);
            // check if it is full path icon or a themed one
            if (g_file_test(full_icon_name, G_FILE_TEST_EXISTS))
              set_icon(full_icon_name);
            else
              set_icon(icon_name);
            g_free(full_icon_name);
          }
        else
          {
            g_warning("Fail to retrive icon:", error->message);
            g_error_free(error);
          }
        g_free(icon_name);
      }

    g_key_file_free(app_info);
    g_free(full_app_desktop_file);
    g_free(app_dir);
    g_free(app_desktop_file);
  }

  /***
  ****
  ***/

  static GVariant* connection_call_finish(GObject      * connection,
                                          GAsyncResult * res,
                                          const char   * warning)
  {
    GError* error = nullptr;

    auto v = g_dbus_connection_call_finish(G_DBUS_CONNECTION(connection),
                                           res,
                                           &error);

    if (v == nullptr)
      {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
          g_warning("%s: %s", warning, error->message);

        g_error_free(error);
      }

    return v;
  }

  core::Signal<> m_changed;

  uint32_t m_changed_tag = 0;
  uint64_t m_received = 0;
  uint64_t m_total_size = 0;
  struct DownloadProgress {
    int64_t time_usec; // microseconds since epoch
    uint64_t bytes;
    DownloadProgress operator-(const DownloadProgress& that) {
      return DownloadProgress{time_usec-that.time_usec, bytes-that.bytes};
    }
  };
  std::vector<DownloadProgress> m_history;

  GDBusConnection* m_bus = nullptr;
  GCancellable* m_cancellable = nullptr;
  std::string m_app_id;
  std::string m_destination_app;
  std::string m_package_name;
  const std::string m_ccad_path;
};

} // anonymous namespace

/***
****
***/

class DMSource::Impl
{
public:

  Impl():
    m_cancellable(g_cancellable_new()),
    m_model(std::make_shared<MutableModel>())
  {
    g_bus_get(G_BUS_TYPE_SESSION, m_cancellable, on_bus_ready, this);
  }

  ~Impl()
  {
    g_cancellable_cancel(m_cancellable);
    g_clear_object(&m_cancellable);
    set_bus(nullptr);
    g_clear_object(&m_bus);
  }

  void start(const Transfer::Id& id)
  {
    auto transfer = find_transfer_by_id(id);
    g_return_if_fail(transfer);
    transfer->start();
  }

  void pause(const Transfer::Id& id)
  {
    auto transfer = find_transfer_by_id(id);
    g_return_if_fail(transfer);
    transfer->pause();
  }

  void resume(const Transfer::Id& id)
  {
    auto transfer = find_transfer_by_id(id);
    g_return_if_fail(transfer);
    transfer->resume();
  }

  void cancel(const Transfer::Id& id)
  {
    auto transfer = find_transfer_by_id(id);
    g_return_if_fail(transfer);
    transfer->cancel();
  }

  void clear(const Transfer::Id& id)
  {
    auto transfer = find_transfer_by_id(id);
    if (transfer)
      {
        m_removed_ccad.insert(transfer->ccad_path());
        m_model->remove(id);
      }
  }

  void open(const Transfer::Id& id)
  {
    auto transfer = find_transfer_by_id(id);
    g_return_if_fail(transfer);
    transfer->open();
    transfer->open_app();
  }

  void open_app(const Transfer::Id& id)
  {
    auto transfer = find_transfer_by_id(id);
    g_return_if_fail(transfer);
    transfer->open_app();
  }

  std::shared_ptr<MutableModel> get_model()
  {
    return m_model;
  }

private:

  static void on_bus_ready(GObject        * /*source_object*/,
                           GAsyncResult   * res,
                           gpointer       gself)
  {
    GError* error = nullptr;
    auto bus = g_bus_get_finish(res, &error);

    if (bus == nullptr)
      {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
          g_warning("Could not get session bus: %s", error->message);

        g_error_free(error);
      }
    else
      {
        static_cast<Impl*>(gself)->set_bus(bus);
        g_object_unref(bus);
      }
  }

  void set_bus(GDBusConnection* bus)
  {
    if (m_bus != nullptr)
      {
        for (const auto& tag : m_signal_subscriptions)
          g_dbus_connection_signal_unsubscribe(m_bus, tag);

        m_signal_subscriptions.clear();
        g_clear_object(&m_bus);
      }

    if (bus != nullptr)
      {
        g_debug("%s: %s", G_STRFUNC, g_dbus_connection_get_unique_name(bus));
        m_bus = G_DBUS_CONNECTION(g_object_ref(bus));

        guint tag;

        tag = g_dbus_connection_signal_subscribe(bus,
                                                 DM_BUS_NAME,
                                                 DM_DOWNLOAD_IFACE_NAME,
                                                 nullptr,
                                                 nullptr,
                                                 nullptr,
                                                 G_DBUS_SIGNAL_FLAGS_NONE,
                                                 on_download_signal,
                                                 this,
                                                 nullptr);
        m_signal_subscriptions.insert(tag);
    }
  }


  static void on_download_signal(GDBusConnection* /*connection*/,
                                 const gchar*     /*sender_name*/,
                                 const gchar*       ccad_path,
                                 const gchar*     /*interface_name*/,
                                 const gchar*       signal_name,
                                 GVariant*          parameters,
                                 gpointer           gself)
  {
    gchar* variant_str = g_variant_print(parameters, TRUE);
    g_debug("download signal: %s %s %s", ccad_path, signal_name, variant_str);
    g_free(variant_str);

    // Route this signal to the DMTransfer for processing
    auto self = static_cast<Impl*>(gself);
    auto transfer = self->find_transfer_by_ccad_path(ccad_path);
    if (transfer)
      transfer->handle_ccad_signal(signal_name, parameters);
    else
      self->create_new_transfer(ccad_path);
  }

  /***
  ****
  ***/

  std::shared_ptr<DMTransfer> find_transfer_by_ccad_path(const std::string& path)
  {
    for (const auto& transfer : m_model->get_all())
      {
        const auto tmp = std::static_pointer_cast<DMTransfer>(transfer);

        if (tmp && (path == tmp->ccad_path()))
          return tmp;
      }

    return nullptr;
  }

  void create_new_transfer(const std::string& ccad_path)
  {
    // don't let transfers reappear after they've been cleared by the user
    if (m_removed_ccad.count(ccad_path))
      return;

    // check if the download should appear on indicator
    GError *error = nullptr;
    auto show = g_dbus_connection_call_sync(m_bus, DM_BUS_NAME, ccad_path.c_str(),
                                            "org.freedesktop.DBus.Properties",
                                            "Get", g_variant_new ("(ss)", DM_DOWNLOAD_IFACE_NAME, "ShowInIndicator"),
                                            G_VARIANT_TYPE ("(v)"),
                                            G_DBUS_CALL_FLAGS_NONE, -1,
                                            m_cancellable, &error);
    if (show != nullptr)
      {
        GVariant *value, *item;
        item = g_variant_get_child_value(show, 0);
        value = g_variant_get_variant(item);
        bool show_in_idicator = g_variant_get_boolean(value);

        g_variant_unref(value);
        g_variant_unref(item);
        g_variant_unref(show);

        if (!show_in_idicator)
          {
            m_removed_ccad.insert(ccad_path);
            return;
          }
      }
    else if (error != nullptr)
      {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
          g_warning("Fail to retrieve 'ShowInIndicator' property: %s", error->message);
        g_error_free(error);
      }

    auto new_transfer = std::make_shared<DMTransfer>(m_bus, ccad_path);

    m_model->add(new_transfer);

    // when one of the DMTransfer's properties changes,
    // emit a change signal for the model
    const auto id = new_transfer->id;
    new_transfer->changed().connect([this,id]{
      if (m_model->get(id))
        m_model->emit_changed(id);
    });
  }

  std::shared_ptr<DMTransfer> find_transfer_by_id(const Transfer::Id& id)
  {
    auto transfer = m_model->get(id);
    g_return_val_if_fail(transfer, std::shared_ptr<DMTransfer>());
    return std::static_pointer_cast<DMTransfer>(transfer);
  }

  GDBusConnection* m_bus = nullptr;
  GCancellable* m_cancellable = nullptr;
  std::set<guint> m_signal_subscriptions;
  std::shared_ptr<MutableModel> m_model;
  std::set<std::string> m_removed_ccad;
};

/***
****
***/

DMSource::DMSource():
  impl(new Impl{})
{
}

DMSource::~DMSource()
{
}

void
DMSource::open(const Transfer::Id& id)
{
  impl->open(id);
}

void
DMSource::start(const Transfer::Id& id)
{
  impl->start(id);
}

void
DMSource::pause(const Transfer::Id& id)
{
  impl->pause(id);
}

void
DMSource::resume(const Transfer::Id& id)
{
  impl->resume(id);
}

void
DMSource::cancel(const Transfer::Id& id)
{
  impl->cancel(id);
}

void
DMSource::clear(const Transfer::Id& id)
{
  impl->clear(id);
}

void
DMSource::open_app(const Transfer::Id& id)
{
    impl->open_app(id);
}

const std::shared_ptr<const MutableModel>
DMSource::get_model()
{
  return impl->get_model();
}

/***
****
***/

} // namespace transfer
} // namespace indicator
} // namespace unity

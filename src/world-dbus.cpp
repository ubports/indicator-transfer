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

#include <transfer/world-dbus.h>

#include <click.h>
#include <ubuntu-app-launch.h>

#include <json-glib/json-glib.h>

#include <algorithm>
#include <iostream>

namespace unity {
namespace indicator {
namespace transfer {

namespace {

static constexpr char const * DM_BUS_NAME {"com.canonical.applications.Downloader"};
static constexpr char const * DM_MANAGER_IFACE_NAME {"com.canonical.applications.DownloadManager"};
static constexpr char const * DM_DOWNLOAD_IFACE_NAME {"com.canonical.applications.Download"};

static constexpr char const * CH_BUS_NAME {"com.ubuntu.content.dbus.Service"};
static constexpr char const * CH_TRANSFER_IFACE_NAME {"com.ubuntu.content.dbus.Transfer"};


/**
 * A Transfer whose state comes from content-hub and ubuntu-download-manager.
 * 
 * Each DBusTransfer tracks a com.canonical.applications.Download (ccad) object
 * from ubuntu-download-manager. The ccad is used for pause/resume/cancel,
 * state change / download progress signals, etc.
 *
 * Each DBusTransfer also tracks a com.ubuntu.content.dbus.Transfer (cucdt)
 * object from content-hub. The cucdt is used for learning the download's peer
 * and for calling Charge() to launch the peer's app.
 */
class DBusTransfer: public Transfer
{
public:

  DBusTransfer(GDBusConnection* connection,
               const std::string& ccad_path,
               const std::string& cucdt_path):
    m_bus(G_DBUS_CONNECTION(g_object_ref(connection))),
    m_cancellable(g_cancellable_new()),
    m_ccad_path(ccad_path),
    m_cucdt_path(cucdt_path)
  {
    id = next_unique_id();
    time_started = time(nullptr);
    get_ccad_properties();
    get_cucdt_properties();
  }

  ~DBusTransfer()
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
    /* Once a transfer is complete, an app can be launched to process
       it by calling Charge() with an empty variant list argument */
    g_return_if_fail(!m_cucdt_path.empty());
    g_dbus_connection_call(m_bus,
                           CH_BUS_NAME,
                           m_cucdt_path.c_str(),
                           CH_TRANSFER_IFACE_NAME,
                           "Charge",
                           g_variant_new("(av)", nullptr),
                           nullptr,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1, // default timeout
                           m_cancellable,
                           on_cucdt_charge, // callback
                           nullptr); // callback user_data
  }

  void open_app()
  {
    g_return_if_fail(!m_peer_name.empty());

    gchar* app_id = ubuntu_app_launch_triplet_to_app_id(m_peer_name.c_str(), nullptr, nullptr);
    g_debug("calling ubuntu_app_launch_start_application() for %s", app_id);
    ubuntu_app_launch_start_application(app_id, nullptr);
    g_free(app_id);
  }

  const std::string& cucdt_path() const
  {
    return m_cucdt_path;
  }

  const std::string& ccad_path() const
  {
    return m_ccad_path;
  }

  void handle_cucdt_signal(const gchar* signal_name, GVariant* parameters)
  {
    if (!g_strcmp0(signal_name, "StoreChanged"))
      {
        const gchar* uri = nullptr;
        g_variant_get_child(parameters, 0, "&s", &uri);
        if (uri != nullptr)
          set_store(uri);
      }
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

  static void on_cucdt_charge(GObject      * source,
                              GAsyncResult * res,
                              gpointer       /*unused*/)
  {
    auto v = connection_call_finish(source, res, "Error calling Charge()");
    g_clear_pointer(&v, g_variant_unref);
  }

  void emit_changed_soon()
  {
    if (m_changed_tag == 0)
        m_changed_tag = g_timeout_add_seconds(1, emit_changed_now, this);
  }

  static gboolean emit_changed_now(gpointer gself)
  {
    auto self = static_cast<DBusTransfer*>(gself);
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
    auto tmp_total_size = total_size;
    auto tmp_progress = progress;
    auto tmp_seconds_left = seconds_left;
    auto tmp_speed_Bps = speed_Bps;

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
    else
      {
        tmp_total_size = 0;
        tmp_speed_Bps = 0;
        tmp_progress = 0.0;
        tmp_seconds_left = -1;
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

  void set_store(const char* store)
  {
    /**
     * This is a workaround until content-hub exposes a peer getter.
     * As an interim step, this code sniffs the peer by looking at the store.
     * the peer's listed in the directory component before HubIncoming, e.g.:
     * "/home/phablet/.cache/com.ubuntu.gallery/HubIncoming/4"
     */
    char** strv = g_strsplit(store, "/", -1);
    int i=0;
    for ( ; strv && strv[i]; ++i)
      if (!g_strcmp0(strv[i], "HubIncoming") && (i>0))
        set_peer_name(strv[i-1]);
    g_strfreev(strv);
  }

  void set_peer_name(const char* peer_name)
  {
    g_return_if_fail(peer_name && *peer_name);

    g_debug("changing '%s' peer_name to '%s'", m_ccad_path.c_str(), peer_name);
    m_peer_name = peer_name;

    /* If we can find a click icon for the peer,
       Use it as the transfer's icon */

    GError* error = nullptr;
    auto user = click_user_new_for_user(nullptr, nullptr, &error);
    if (user != nullptr)
      {
        gchar* path = click_user_get_path(user, peer_name, &error);
        if (path != nullptr)
          {
            auto manifest = click_user_get_manifest(user, peer_name, &error);
            if (manifest != nullptr)
              {
                const auto icon_name = json_object_get_string_member(manifest, "icon");
                if (icon_name != nullptr)
                  {
                    auto filename = g_build_filename(path, icon_name, nullptr);
                    set_icon(filename);
                    g_free(filename);
                  }
              }
            g_free(path);
          }
      }

    if (error != nullptr)
      g_warning("Unable to get manifest for '%s' package: %s", peer_name, error->message);

    g_clear_object(&user);
    g_clear_error(&error);
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
  ****  Content Hub
  ***/

  void get_cucdt_properties()
  {
    const auto bus_name = CH_BUS_NAME;
    const auto object_path = m_cucdt_path.c_str();
    const auto interface_name = CH_TRANSFER_IFACE_NAME;

    g_dbus_connection_call(m_bus, bus_name, object_path, interface_name,
                           "Store", nullptr, G_VARIANT_TYPE("(s)"),
                           G_DBUS_CALL_FLAGS_NONE, -1,
                           m_cancellable, on_cucdt_store, this);
  }

  static void on_cucdt_store(GObject* source, GAsyncResult* res, gpointer gself)
  {
    auto v = connection_call_finish(source, res, "Unable to get store");
    if (v != nullptr)
      {
        const gchar* store = nullptr;
        g_variant_get_child(v, 0, "&s", &store);
        if (store != nullptr)
          static_cast<DBusTransfer*>(gself)->set_store(store);
        g_variant_unref(v);
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

        auto self = static_cast<DBusTransfer*>(gself);
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

        auto self = static_cast<DBusTransfer*>(gself);
        self->m_received = n;
        self->update_progress();
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
  const std::string m_ccad_path;
  const std::string m_cucdt_path;
  std::string m_peer_name;
};

} // anonymous namespace

/***
****
***/

class DBusWorld::Impl
{
public:

  Impl(const std::shared_ptr<MutableModel>& model):
    m_cancellable(g_cancellable_new()),
    m_model(model)
  {
    g_bus_get(G_BUS_TYPE_SESSION, m_cancellable, on_bus_ready, this);

    m_model->removed().connect([this](const Transfer::Id& id){
      auto transfer = find_transfer_by_id(id);
      if (transfer)
        m_removed_ccad.insert(transfer->ccad_path());
    });
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

        tag = g_dbus_connection_signal_subscribe(bus,
                                                 CH_BUS_NAME,
                                                 CH_TRANSFER_IFACE_NAME,
                                                 nullptr,
                                                 nullptr,
                                                 nullptr,
                                                 G_DBUS_SIGNAL_FLAGS_NONE,
                                                 on_transfer_signal_static,
                                                 this,
                                                 nullptr);
        m_signal_subscriptions.insert(tag);
      }
  }

  static void on_transfer_signal_static(GDBusConnection* /*connection*/,
                                        const gchar*     /*sender_name*/,
                                        const gchar*       object_path,
                                        const gchar*     /*interface_name*/,
                                        const gchar*       signal_name,
                                        GVariant*          parameters,
                                        gpointer           gself)
  {
    static_cast<Impl*>(gself)->on_transfer_signal(object_path, signal_name, parameters);
  }

  void on_transfer_signal(const gchar* cucdt_path,
                          const gchar* signal_name,
                          GVariant* parameters)
  {
    gchar* variant_str = g_variant_print(parameters, TRUE);
    g_debug("transfer signal: %s %s %s", cucdt_path, signal_name, variant_str);
    g_free(variant_str);

    if (!g_strcmp0(signal_name, "DownloadIdChanged"))
      {
        const char* ccad_path = nullptr;
        g_variant_get_child(parameters, 0, "&s", &ccad_path);
        g_return_if_fail(cucdt_path != nullptr);

        // ensure this ccad/cucdt pair is tracked
        if (!find_transfer_by_ccad_path(ccad_path))
          create_new_transfer(ccad_path, cucdt_path);
      }
    else
      {
        // Route this signal to the DBusTransfer for processing
        auto transfer = find_transfer_by_cucdt_path(cucdt_path);
        if (transfer)
          transfer->handle_cucdt_signal(signal_name, parameters);
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

    // Route this signal to the DBusTransfer for processing 
    auto self = static_cast<Impl*>(gself);
    auto transfer = self->find_transfer_by_ccad_path(ccad_path);
    if (transfer)
      transfer->handle_ccad_signal(signal_name, parameters);
  }

  /***
  ****
  ***/

  std::shared_ptr<DBusTransfer> find_transfer_by_ccad_path(const std::string& path)
  {
    for (const auto& transfer : m_model->get_all())
      {
        const auto tmp = std::static_pointer_cast<DBusTransfer>(transfer);

        if (tmp && (path == tmp->ccad_path()))
          return tmp;
      }

    return nullptr;
  }

  std::shared_ptr<DBusTransfer> find_transfer_by_cucdt_path(const std::string& path)
  {
    for (const auto& transfer : m_model->get_all())
      {
        const auto tmp = std::static_pointer_cast<DBusTransfer>(transfer);

        if (tmp && (path == tmp->cucdt_path()))
          return tmp;
      }

    return nullptr;
  }

  void create_new_transfer(const std::string& ccad_path,
                           const std::string& cucdt_path)
  {
    // don't let transfers reappear after they've been cleared by the user
    if (m_removed_ccad.count(ccad_path))
      return;

    auto new_transfer = std::make_shared<DBusTransfer>(m_bus, ccad_path, cucdt_path);

    m_model->add(new_transfer);

    // when one of the DBusTransfer's properties changes,
    // emit a change signal for the model
    const auto id = new_transfer->id;
    new_transfer->changed().connect([this,id]{
      if (m_model->get(id))
        m_model->emit_changed(id);
    });
  }

  std::shared_ptr<DBusTransfer> find_transfer_by_id(const Transfer::Id& id)
  {
    auto transfer = m_model->get(id);
    g_return_val_if_fail(transfer, std::shared_ptr<DBusTransfer>());
    return std::static_pointer_cast<DBusTransfer>(transfer);
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

DBusWorld::DBusWorld(const std::shared_ptr<MutableModel>& model):
  impl(new Impl(model))
{
}

DBusWorld::~DBusWorld()
{
}

void
DBusWorld::open(const Transfer::Id& id)
{
  impl->open(id);
}

void
DBusWorld::start(const Transfer::Id& id)
{
  impl->start(id);
}

void
DBusWorld::pause(const Transfer::Id& id)
{
  impl->pause(id);
}

void
DBusWorld::resume(const Transfer::Id& id)
{
  impl->resume(id);
}

void
DBusWorld::cancel(const Transfer::Id& id)
{
  impl->cancel(id);
}

void
DBusWorld::open_app(const Transfer::Id& id)
{
  impl->open_app(id);
}

/***
****
***/


} // namespace transfer
} // namespace indicator
} // namespace unity

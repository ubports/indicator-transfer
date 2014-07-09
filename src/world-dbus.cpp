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

#include <iostream>

namespace unity {
namespace indicator {
namespace transfer {

namespace {

#define BUS_NAME "com.canonical.applications.Downloader"
#define DOWNLOAD_MANAGER_IFACE_NAME "com.canonical.applications.DownloadManager"
#define DOWNLOAD_IFACE_NAME "com.canonical.applications.Download"

class DBusTransfer: public Transfer
{
public:

  DBusTransfer(GDBusConnection* connection, const char* object_path):
    m_bus(G_DBUS_CONNECTION(g_object_ref(connection))),
    m_cancellable(g_cancellable_new()),
    m_object_path(object_path)
  {
    g_debug("creating a new DBusTransfer for '%s'", object_path);

    id = next_unique_id();
    time_started = time(nullptr);

    get_properties_from_bus();
  }

  ~DBusTransfer()
  {
    g_cancellable_cancel(m_cancellable);
    g_clear_object(&m_cancellable);
    g_clear_object(&m_bus);
  }

  core::Signal<>& changed() { return m_changed; }

  void start()
  {
    g_return_if_fail (can_start());
    
    call_method_no_args_no_response ("start");
  }

  void pause()
  {
    g_return_if_fail (can_pause());
    
    call_method_no_args_no_response("pause");
  }

  void resume()
  {
    g_return_if_fail(can_resume());

    call_method_no_args_no_response("resume");
  }

  void cancel()
  {
    call_method_no_args_no_response("cancel");
  }

  const std::string& object_path() const
  {
    return m_object_path;
  }

  void handle_signal(const gchar* signal_name, GVariant* parameters)
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
        set_error(error_string);
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
        char* error_string = nullptr;
        g_variant_get_child(parameters, 1, "s", &error_string);
        set_error(error_string);
        g_free(error_string);
      }
    else
      {
        auto args = g_variant_print(parameters, true);
        g_warning("%s: unrecognized signal '%s': %s", G_STRLOC, signal_name, args);
        g_free(args);
      }
  }

private:

  // the 'started', 'paused', 'resumed', and 'canceled' signals
  // from com.canonical.applications.Download all have a single
  // parameter, a boolean success flag.
  bool get_signal_success_arg(GVariant* parameters)
  {
    gboolean success = false;
    g_return_val_if_fail(g_variant_is_container(parameters), false);
    g_return_val_if_fail(g_variant_n_children(parameters) == 1, false);
    g_variant_get_child(parameters, 0, "b", &success);
    return success;
  }

  void call_method_no_args_no_response(const char* method_name)
  {
    g_dbus_connection_call(m_bus,                  // connection
                           BUS_NAME,               // bus_name
                           m_object_path.c_str(),  // object path
                           DOWNLOAD_IFACE_NAME,    // interface name
                           method_name,            // method name
                           nullptr,                // parameters
                           nullptr,                // reply type
                           G_DBUS_CALL_FLAGS_NONE, // flags
                           -1,                     // default timeout
                           m_cancellable,          // cancellable
                           nullptr,                // callback
                           nullptr                 /*user data*/);
  }


  core::Signal<> m_changed;
  void emit_changed() { changed()(); }

  uint64_t m_received = 0;
  uint64_t m_total_size = 0;
  std::vector<std::pair<GTimeVal,uint64_t>> m_history;

  static double get_time_from_timeval(const GTimeVal& t)
  {
    return t.tv_sec + (t.tv_usec / (double)G_USEC_PER_SEC);
  }

  // gets an averaged speed based on information saved from 
  // recent calls to set_progress()
  uint64_t get_average_speed_bps()
  {
    unsigned int n_samples = 0;
    uint64_t sum = 0;

    // prune out samples by size
    static constexpr int max_slots = 50;
    if (m_history.size() > max_slots)
      m_history.erase(m_history.begin(), m_history.end()-max_slots);

    static constexpr time_t max_age_seconds = 30;
    GTimeVal now_tv;
    g_get_current_time(&now_tv);
    const double now = get_time_from_timeval(now_tv);
    for(unsigned i=1; i<m_history.size(); i++)
      {
        const double begin_time = get_time_from_timeval(m_history[i-1].first);
        if (now - begin_time > max_age_seconds)
          continue;

        const double end_time = get_time_from_timeval(m_history[i].first);
        const double time_diff = end_time - begin_time;
        const uint64_t received_diff = m_history[i].second -
                                       m_history[i-1].second;
        const uint64_t bps = uint64_t(received_diff / time_diff);

        sum += bps;
        ++n_samples;
      }

    return n_samples ? sum / n_samples : 0;
  }

  void update_progress()
  {
    auto tmp_total_size = total_size;
    auto tmp_progress = progress;
    auto tmp_seconds_left = seconds_left;
    auto tmp_speed_bps = speed_bps;

    if (m_total_size && m_received)
      {
        // update our speed tables
        GTimeVal now;
        g_get_current_time(&now);
        m_history.push_back(std::pair<GTimeVal,uint64_t>(now, m_received));

        const auto bps = get_average_speed_bps();
        const int seconds = bps ? (int)((m_total_size - m_received) / bps) : -1;

        tmp_total_size = m_total_size;
        tmp_speed_bps = bps;
        tmp_progress = m_received / (float)m_total_size;
        tmp_seconds_left = seconds;
      }
    else
      {
        tmp_total_size = 0;
        tmp_speed_bps = 0;
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
        seconds_left = tmp_seconds_left;
        changed = true;
      }

    if (speed_bps != tmp_speed_bps)
      {
        speed_bps = tmp_speed_bps;
        changed = true;
      }

    if (total_size != tmp_total_size)
      {
        total_size = tmp_total_size;
        changed = true;
      }

    if (changed)
      emit_changed();
  }

  void set_state(State state_in)
  {
    if (state != state_in)
      {
        state = state_in;

        if (!can_pause())
          {
            speed_bps = 0;
            m_history.clear();
          }
 
        emit_changed();
      }
  }

  void set_error(const char* str)
  {
    const std::string tmp = str ? str : "";
    if (error_string != tmp)
    {
      error_string = tmp;
      emit_changed();
    }
  }

  void set_local_path(const char* str)
  {
    const std::string tmp = str ? str : "";
    if (local_path != tmp)
    {
      local_path = tmp;
      emit_changed();
    }
  }

  /***
  ****  DBUS
  ***/

  GDBusConnection* m_bus = nullptr;
  GCancellable* m_cancellable = nullptr;
  const std::string m_object_path;

  void get_properties_from_bus()
  {
    const auto bus_name = BUS_NAME;
    const auto object_path = m_object_path.c_str();
    const auto interface_name = DOWNLOAD_IFACE_NAME;

    g_dbus_connection_call(m_bus, bus_name, object_path, interface_name,
                           "totalSize", nullptr, G_VARIANT_TYPE("(t)"),
                           G_DBUS_CALL_FLAGS_NONE, -1,
                           m_cancellable, on_total_size, this);

    g_dbus_connection_call(m_bus, bus_name, object_path, interface_name,
                           "progress", nullptr, G_VARIANT_TYPE("(t)"),
                           G_DBUS_CALL_FLAGS_NONE, -1,
                           m_cancellable, on_progress, this);

    g_dbus_connection_call(m_bus, bus_name, object_path, interface_name,
                           "metadata", nullptr, G_VARIANT_TYPE("(a{sv})"),
                           G_DBUS_CALL_FLAGS_NONE, -1,
                           m_cancellable, on_metadata, this);
  }

  static void on_total_size(GObject* connection,
                            GAsyncResult* res,
                            gpointer gself)
  {
    GError* error = nullptr;

    auto v = g_dbus_connection_call_finish(G_DBUS_CONNECTION(connection),
                                           res,
                                           &error);

    if (error != nullptr)
      {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
          g_warning("Couldn't get download total size: %s", error->message);

        g_error_free(error);
      }
    else
      {
        guint64 n = 0;
        g_variant_get_child(v, 0, "t", &n);
        g_variant_unref(v);

        auto self = static_cast<DBusTransfer*>(gself);
        self->m_total_size = n;
        self->update_progress();
      }
  }

  static void on_progress(GObject* connection,
                          GAsyncResult* res,
                          gpointer gself)
  {
    GError* error = nullptr;
    auto v = g_dbus_connection_call_finish(G_DBUS_CONNECTION(connection),
                                           res,
                                           &error);

    if (error != nullptr)
      {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
          g_warning("Couldn't get download progress: %s", error->message);

        g_error_free(error);
      }
    else
      {
        guint64 n = 0;
        g_variant_get_child(v, 0, "t", &n);
        g_variant_unref(v);

        auto self = static_cast<DBusTransfer*>(gself);
        self->m_received = n;
        self->update_progress();
      }
  }

  static void on_metadata(GObject* connection,
                          GAsyncResult* res,
                          gpointer)// gself)
  {
    GError* error = nullptr;
    auto v = g_dbus_connection_call_finish(G_DBUS_CONNECTION(connection),
                                           res,
                                           &error);

    if (error != nullptr)
      {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
          g_warning("Couldn't get download metadata: %s", error->message);

        g_error_free(error);
      }
    else
      {
        char * variant_str = g_variant_print(v, true);
        // FIXME: pull appname (and icon) and title from metadata?
        g_warning("%s unhandled metadata: %s", G_STRFUNC, variant_str);
        g_free(variant_str);
        g_variant_unref(v);
      }
  }
};

} // anonymous namespace

/***
****
***/

DBusWorld::DBusWorld(const std::shared_ptr<MutableModel>& model):
    m_cancellable(g_cancellable_new()),
    m_model(model)
{
    g_bus_get(G_BUS_TYPE_SESSION, m_cancellable, on_bus_ready, this);
}

DBusWorld::~DBusWorld()
{
    g_cancellable_cancel(m_cancellable);
    g_clear_object(&m_cancellable);
    set_bus(nullptr);
    g_clear_object(&m_bus);
}

void DBusWorld::on_bus_ready(GObject* /*source_object*/,
                            GAsyncResult* res,
                            gpointer gself)
{
  GError* error = nullptr;
  auto bus = g_bus_get_finish(res, &error);

  if (error != nullptr)
    {
      if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning("Could not get session bus: %s", error->message);

      g_error_free(error);
    }
  else
    {
      static_cast<DBusWorld*>(gself)->set_bus(bus);
      g_object_unref(bus);
    }
}

void DBusWorld::set_bus(GDBusConnection* bus)
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
                                               BUS_NAME,
                                               DOWNLOAD_MANAGER_IFACE_NAME,
                                               "downloadCreated",
                                               "/",
                                               nullptr,
                                               G_DBUS_SIGNAL_FLAGS_NONE,
                                               on_download_created,
                                               this,
                                               nullptr);
      m_signal_subscriptions.insert(tag);

      tag = g_dbus_connection_signal_subscribe (bus,
                                                BUS_NAME,
                                                DOWNLOAD_IFACE_NAME,
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

namespace
{
  std::shared_ptr<DBusTransfer>
  find_dbus_transfer_for_object_path(const std::shared_ptr<Model>& model,
                                     const std::string& object_path)
  {
    std::shared_ptr<DBusTransfer> dbus_transfer;

    for (const auto& transfer : model->get_all())
      {
        const auto tmp = std::static_pointer_cast<DBusTransfer>(transfer);

        if (tmp && (tmp->object_path()==object_path))
          {
             dbus_transfer = tmp;
             break;
          }
      }

    return dbus_transfer;
  }
}

void DBusWorld::on_download_signal(GDBusConnection*, //connection,
                                  const gchar*,      //sender_name,
                                  const gchar*         object_path,
                                  const gchar*,      //interface_name,
                                  const gchar*         signal_name,
                                  GVariant*            parameters,
                                  gpointer             gself)
{
  auto self = static_cast<DBusWorld*>(gself);

  auto dbus_transfer = find_dbus_transfer_for_object_path(self->m_model, object_path);

  if (!dbus_transfer)
    {
      g_message("A %s that we didn't know about just emitted signal '%s' -- "
                "might be a transfer that was here before us?",
                DOWNLOAD_IFACE_NAME, signal_name);
      self->add_transfer(object_path);
      dbus_transfer = find_dbus_transfer_for_object_path(self->m_model, object_path);
      g_return_if_fail (dbus_transfer);
    }

  dbus_transfer->handle_signal(signal_name, parameters);
}

void DBusWorld::on_download_created(GDBusConnection*, //connection,
                                   const gchar*,      //sender_name,
                                   const gchar*,      //object_path,
                                   const gchar*,      //interface_name,
                                   const gchar*,      //signal_name,
                                   GVariant*            parameters,
                                   gpointer             gself)
{
  gchar* download_path = nullptr;
  g_variant_get_child(parameters, 0, "o", &download_path);

  if (download_path != nullptr)
    {
      if (g_variant_is_object_path(download_path))
        static_cast<DBusWorld*>(gself)->add_transfer(download_path);

      g_free(download_path);
    }
}

void DBusWorld::add_transfer(const char* object_path)
{
  // create a new Transfer and pass it to the model
  auto dbus_transfer = new DBusTransfer(m_bus, object_path);
  std::shared_ptr<Transfer> transfer(dbus_transfer);
  m_model->add(transfer);

  // notify the model whenever the Transfer changes
  const auto id = dbus_transfer->id;
  dbus_transfer->changed().connect([this,id]{
    if (m_model->get(id))
      m_model->emit_changed(id);
  });
}

namespace
{
  std::shared_ptr<DBusTransfer>
  get_dbus_transfer(const std::shared_ptr<Model>& model, const Transfer::Id& id)
  {
    auto transfer = model->get(id);
    g_return_val_if_fail(transfer, std::shared_ptr<DBusTransfer>());
    return std::static_pointer_cast<DBusTransfer>(transfer);
  }
}

void DBusWorld::start(const Transfer::Id& id)
{
  auto dbus_transfer = get_dbus_transfer (m_model, id);
  g_return_if_fail(dbus_transfer);
  dbus_transfer->start();
}

void DBusWorld::pause(const Transfer::Id& id)
{
  auto dbus_transfer = get_dbus_transfer (m_model, id);
  g_return_if_fail(dbus_transfer);
  dbus_transfer->pause();
}

void DBusWorld::resume(const Transfer::Id& id)
{
  auto dbus_transfer = get_dbus_transfer (m_model, id);
  g_return_if_fail(dbus_transfer);
  dbus_transfer->resume();
}

void DBusWorld::cancel(const Transfer::Id& id)
{
  auto dbus_transfer = get_dbus_transfer (m_model, id);
  g_return_if_fail(dbus_transfer);
  dbus_transfer->cancel();
}

void DBusWorld::open(const Transfer::Id& id)
{
  std::cerr << G_STRFUNC << " FIXME " << id << std::endl;
}

void DBusWorld::open_app(const Transfer::Id& id)
{
  std::cerr << G_STRFUNC << " FIXME " << id << std::endl;
}

/***
****
***/


} // namespace transfer
} // namespace indicator
} // namespace unity

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

#ifndef INDICATOR_TRANSFER_WORLD_DBUS_H
#define INDICATOR_TRANSFER_WORLD_DBUS_H

#include <transfer/world.h>

#include <gio/gio.h>

#include <set>

namespace unity {
namespace indicator {
namespace transfer {

/**
 * \brief a World that gets its updates & events from DBus
 */
class DBusWorld: public World
{
public:
    DBusWorld(const std::shared_ptr<MutableModel>& model);
    ~DBusWorld();

    void open(const Transfer::Id& id);
    void start(const Transfer::Id& id);
    void pause(const Transfer::Id& id);
    void resume(const Transfer::Id& id);
    void cancel(const Transfer::Id& id);
    void open_app(const Transfer::Id& id);

private:
    GDBusConnection* m_bus = nullptr;
    GCancellable* m_cancellable = nullptr;
    std::set<guint> m_signal_subscriptions;
    std::shared_ptr<MutableModel> m_model;

    void set_bus(GDBusConnection*);
    void add_transfer(const char* object_path);

    static void on_bus_ready(GObject*, GAsyncResult*, gpointer);
    static void on_progress(GObject*, GAsyncResult*, gpointer);
    static void on_total_size(GObject*, GAsyncResult*, gpointer);
    static void on_download_signal(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar*, GVariant*, gpointer);
    static void on_download_created(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar*, GVariant*, gpointer);
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_WORLD_H

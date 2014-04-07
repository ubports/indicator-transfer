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

#include "actions-mock.h"
#include "glib-fixture.h"

#include <transfer/dbus-shared.h>
#include <transfer/exporter.h>

#include <set>
#include <string>

using namespace unity::indicator::transfer;

class ExporterFixture: public GlibFixture
{
private:

    typedef GlibFixture super;

protected:

    GTestDBus* bus = nullptr;

    void SetUp()
    {
        super::SetUp();

        // bring up the test bus
        bus = g_test_dbus_new(G_TEST_DBUS_NONE);
        g_test_dbus_up(bus);
        const auto address = g_test_dbus_get_bus_address(bus);
        g_setenv("DBUS_SYSTEM_BUS_ADDRESS", address, true);
        g_setenv("DBUS_SESSION_BUS_ADDRESS", address, true);
    }

    void TearDown()
    {
        GError * error = nullptr;
        GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
        if(!g_dbus_connection_is_closed(connection))
            g_dbus_connection_close_sync(connection, nullptr, &error);
        g_assert_no_error(error);
        g_clear_object(&connection);
        g_test_dbus_down(bus);
        g_clear_object(&bus);

        super::TearDown();
    }
};

TEST_F(ExporterFixture, HelloWorld)
{
    // confirms that the Test DBus SetUp() and TearDown() works
}

TEST_F(ExporterFixture, Publish)
{
    // create the MockActions and GActions bridge
    std::shared_ptr<MockActions> mock_actions{new MockActions};
    std::shared_ptr<Actions> actions {std::dynamic_pointer_cast<Actions>(mock_actions)};
    std::shared_ptr<GActions> gactions{new GActions{actions}};

    // create the menus
    std::shared_ptr<Transfers> transfers {new Transfers{}};
    std::vector<std::shared_ptr<Menu>> menus;
    MenuFactory menu_factory (transfers, gactions);
    for(int i=0; i<Menu::NUM_PROFILES; i++)
      menus.push_back(menu_factory.buildMenu(Menu::Profile(i)));

    // export 'em
    Exporter exporter;
    exporter.publish(gactions, menus);
    wait_msec();

    auto connection = g_bus_get_sync (G_BUS_TYPE_SESSION, nullptr, nullptr);
    auto exported = g_dbus_action_group_get (connection, BUS_NAME, BUS_PATH);
    auto names_strv = g_action_group_list_actions(G_ACTION_GROUP(exported));

    // wait for the exported ActionGroup to be populated
    if (g_strv_length(names_strv) == 0)
    {
        g_strfreev(names_strv);
        wait_for_signal(exported, "action-added");
        names_strv = g_action_group_list_actions(G_ACTION_GROUP(exported));
    }

    // convert it to a std::set for easy prodding
    std::set<std::string> names;
    for(int i=0; names_strv && names_strv[i]; i++)
      names.insert(names_strv[i]);

    // confirm the actions that we expect
    EXPECT_EQ(1, names.count("activate-transfer"));
    EXPECT_EQ(1, names.count("cancel-transfer"));
    EXPECT_EQ(1, names.count("pause-transfer"));
    EXPECT_EQ(1, names.count("resume-transfer"));
    EXPECT_EQ(1, names.count("pause-all"));
    EXPECT_EQ(1, names.count("resume-all"));
    EXPECT_EQ(1, names.count("clear-all"));

    // try closing the connection prematurely
    // to test Exporter's name-lost signal
    bool name_lost {false};
    exporter.name_lost.connect([this,&name_lost](){
        name_lost = true;
        g_main_loop_quit(loop);
    });
    g_dbus_connection_close_sync(connection, nullptr, nullptr);
    g_main_loop_run(loop);
    EXPECT_TRUE(name_lost);

    // cleanup
    g_strfreev(names_strv);
    g_clear_object(&exported);
    g_clear_object(&connection);
}

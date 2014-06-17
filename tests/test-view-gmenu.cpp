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

#include "glib-fixture.h"
#include "controller-mock.h"
#include "world-mock.h"

#include <transfer/dbus-shared.h>
#include <transfer/view-gmenu.h>


using namespace unity::indicator::transfer;

class GMenuViewFixture: public GlibFixture
{
private:
  typedef GlibFixture super;

protected:

  GTestDBus* bus = nullptr;
  std::shared_ptr<MockWorld> m_world;
  std::shared_ptr<MutableModel> m_model;
  std::shared_ptr<MockController> m_controller;
  std::shared_ptr<GMenuView> m_view;

  void SetUp()
  {
    super::SetUp();

    // bring up the test bus
    bus = g_test_dbus_new(G_TEST_DBUS_NONE);
    g_test_dbus_up(bus);
    const auto address = g_test_dbus_get_bus_address(bus);
    g_setenv("DBUS_SYSTEM_BUS_ADDRESS", address, true);
    g_setenv("DBUS_SESSION_BUS_ADDRESS", address, true);

    // bring up the world
    m_world.reset(new MockWorld);
    m_model.reset(new MutableModel);
    std::shared_ptr<Transfer> t;
    t.reset(new Transfer);
    t->id = "a";
    t->state = Transfer::RUNNING;
    m_model->add(t);
    t.reset(new Transfer);
    t->id = "b";
    t->state = Transfer::PAUSED;
    m_model->add(t);
    t.reset(new Transfer);
    t->id = "c";
    t->state = Transfer::FINISHED;
    m_model->add(t);
    m_controller.reset(new MockController(m_model, m_world));
    m_view.reset(new GMenuView(m_model, m_controller));
  }

  void TearDown()
  {
    // empty the world
    m_view.reset();
    m_controller.reset();
    m_model.reset();
    m_world.reset();

    // bring down the bus
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

/***
****
***/

/**
 * These two are more about testing the scaffolding.
 * If the objects listening to the bus don't all get
 * torn down correctly, the second time we setup the
 * fixture we'll see glib errors
 */

TEST_F(GMenuViewFixture, CanFixtureSetupOnce)
{
  g_timeout_add_seconds(1, [](gpointer g){
    g_main_loop_quit(static_cast<GMainLoop*>(g));
    return G_SOURCE_REMOVE;
  }, loop);
  g_main_loop_run(loop);
}

TEST_F(GMenuViewFixture, CanFixtureSetupTwice)
{
  g_timeout_add_seconds(1, [](gpointer g){
    g_main_loop_quit(static_cast<GMainLoop*>(g));
    return G_SOURCE_REMOVE;
  }, loop);
  g_main_loop_run(loop);
}

/***
****
****  GActions
****
***/

/* Make sure all the actions we expect are there */
TEST_F(GMenuViewFixture, ExportedActions)
{
  wait_msec();

  // these are the actions we expect to find
  const std::set<std::string> expected_actions {
    "activate-transfer",
    "cancel-transfer",
    "clear-all",
    "open-app-transfer",
    "open-transfer",
    "pause-transfer",
    "pause-all",
    "phone-header",
    "resume-all",
    "resume-transfer",
    "transfer-states"
  };

  auto connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
  auto exported = g_dbus_action_group_get(connection, BUS_NAME, BUS_PATH);
  auto names_strv = g_action_group_list_actions(G_ACTION_GROUP(exported));

  // wait for the exported ActionGroup to be populated
  if (g_strv_length(names_strv) == 0)
    {
      g_strfreev(names_strv);
      wait_for_signal(exported, "action-added");
      names_strv = g_action_group_list_actions(G_ACTION_GROUP(exported));
    }

  // convert it to a std::set for easy prodding
  std::set<std::string> actions;
  for(int i=0; names_strv && names_strv[i]; i++)
    actions.insert(names_strv[i]);

  EXPECT_EQ(expected_actions, actions);

  // try closing the connection prematurely
  // to test Exporter's name-lost signal
  bool name_lost = false;
  m_view->name_lost().connect([this,&name_lost](){
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

TEST_F(GMenuViewFixture, InvokedGActionsCallTheController)
{
  wait_msec();
  auto connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
  auto dbus_action_group = g_dbus_action_group_get(connection, BUS_NAME, BUS_PATH);
  auto action_group = G_ACTION_GROUP(dbus_action_group);

  // wait for the exported ActionGroup to be populated
  auto names_strv = g_action_group_list_actions(action_group);
  if (g_strv_length(names_strv) == 0)
    {
      g_strfreev(names_strv);
      wait_for_signal(dbus_action_group, "action-added");
      names_strv = g_action_group_list_actions(action_group);
    }
  g_strfreev(names_strv);

  // try tapping a transfer that can be resumed
  const char* id = "b";
  EXPECT_TRUE(m_model->get(id)->can_resume());
  EXPECT_CALL(*m_controller, tap(id)).Times(1);
  g_action_group_activate_action(action_group, "activate-transfer", g_variant_new_string(id));
  wait_msec();

  // try tapping a transfer that CAN'T be resumed
  id = "c";
  EXPECT_TRUE(!m_model->get(id)->can_resume());
  EXPECT_CALL(*m_controller, tap(id)).Times(1);
  g_action_group_activate_action(action_group, "activate-transfer", g_variant_new_string(id));
  wait_msec();

  // try cancelling a transfer
  id = "a";
  EXPECT_CALL(*m_controller, cancel(id)).Times(1);
  g_action_group_activate_action(action_group, "cancel-transfer", g_variant_new_string(id));
  wait_msec();

  // try opening a transfer
  id = "b";
  EXPECT_CALL(*m_controller, open(id)).Times(1);
  g_action_group_activate_action(action_group, "open-transfer", g_variant_new_string(id));
  wait_msec();

  // try opening a transfer's recipient's app
  id = "c";
  EXPECT_CALL(*m_controller, open_app(id)).Times(1);
  g_action_group_activate_action(action_group, "open-app-transfer", g_variant_new_string(id));
  wait_msec();

  // try calling clear-all
  EXPECT_CALL(*m_controller, clear_all()).Times(1);
  g_action_group_activate_action(action_group, "clear-all", nullptr);
  wait_msec();

  // try pausing a transfer
  id = "a";
  EXPECT_CALL(*m_controller, pause(id)).Times(1);
  g_action_group_activate_action(action_group, "pause-transfer", g_variant_new_string(id));
  wait_msec();

  // try calling pause-all
  EXPECT_CALL(*m_controller, pause_all()).Times(1);
  g_action_group_activate_action(action_group, "pause-all", nullptr);
  wait_msec();

  // try calling resume-all
  EXPECT_CALL(*m_controller, resume_all()).Times(1);
  g_action_group_activate_action(action_group, "resume-all", nullptr);
  wait_msec();

  // try resuming a transfer
  id = "a";
  EXPECT_CALL(*m_controller, resume(id)).Times(1);
  g_action_group_activate_action(action_group, "resume-transfer", g_variant_new_string(id));
  wait_msec();

  // cleanup
  g_clear_object(&dbus_action_group);
  g_clear_object(&connection);
}

/***
****
****  GMenu
****
***/

TEST_F(GMenuViewFixture, DoesExportMenu)
{
  wait_msec();

  auto connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
  auto dbus_menu_model = g_dbus_menu_model_get(connection, BUS_NAME, BUS_PATH"/phone");
  auto menu_model = G_MENU_MODEL(dbus_menu_model);

  // query the GDBusMenuModel for information to activate it
  int n = g_menu_model_get_n_items(menu_model);
  if (!n)
    {
      // give the model a moment to populate its info
      wait_msec(100);
      n = g_menu_model_get_n_items(menu_model);
    }

  EXPECT_TRUE(menu_model != nullptr);
  EXPECT_NE(0, n);

  // cleanup
  g_clear_object(&dbus_menu_model);
  g_clear_object(&connection);
}


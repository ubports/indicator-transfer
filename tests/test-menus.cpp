/*
 * Copyright 2013 Canonical Ltd.
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

#include <transfer/gactions.h>
#include <transfer/menu.h>
#include <transfer/transfer-mock.h>

#include <gio/gio.h>

#include <memory>

using namespace unity::indicator::transfer;

/***
****
***/

class MenuFixture: public GlibFixture
{
private:
    typedef GlibFixture super;

protected:
    std::shared_ptr<MockActions> m_mock_actions;
    std::shared_ptr<Actions> m_actions;
    std::shared_ptr<GActions> m_gactions;
    std::shared_ptr<Transfers> m_transfers;
    std::shared_ptr<MenuFactory> m_menu_factory;
    std::vector<std::shared_ptr<Menu>> m_menus;

    virtual void SetUp()
    {
        super::SetUp();

        // create the MockActions and GActions bridge
        m_mock_actions.reset(new MockActions{});
        m_actions = std::dynamic_pointer_cast<Actions>(m_mock_actions);
        m_gactions.reset(new GActions{m_actions});

        // create the menus
        m_transfers.reset(new Transfers{});
        m_menus.clear();
        m_menu_factory.reset(new MenuFactory{m_transfers, m_gactions});
        for(int i=0; i<Menu::NUM_PROFILES; ++i)
          m_menus.push_back(m_menu_factory->buildMenu(Menu::Profile(i)));
    }

    virtual void TearDown()
    {
        m_menus.clear();
        m_menu_factory.reset();
        m_transfers.reset();
        m_gactions.reset();
        m_actions.reset();
        m_mock_actions.reset();

        super::TearDown();
    }

    void InspectHeader(GMenuModel* menu_model, const char* name)
    {
        // check that there's a header menuitem
        EXPECT_EQ(1,g_menu_model_get_n_items(menu_model));
        gchar* str = nullptr;
        g_menu_model_get_item_attribute(menu_model, 0, "x-canonical-type", "s", &str);
        EXPECT_STREQ("com.canonical.indicator.root", str);
        g_clear_pointer(&str, g_free);
        g_menu_model_get_item_attribute(menu_model, 0, G_MENU_ATTRIBUTE_ACTION, "s", &str);
        auto action_name = g_strdup_printf("%s-header", name);
        EXPECT_EQ(std::string("indicator.")+action_name, str);
        g_clear_pointer(&str, g_free);

        // check the header
        auto dict = g_action_group_get_action_state(m_gactions->action_group(), action_name);
        EXPECT_TRUE(dict != nullptr);
        EXPECT_TRUE(g_variant_is_of_type(dict, G_VARIANT_TYPE_VARDICT));
        auto v = g_variant_lookup_value(dict, "accessible-desc", G_VARIANT_TYPE_STRING);
        EXPECT_TRUE(v != nullptr);
        g_variant_unref(v);
        v = g_variant_lookup_value(dict, "label", G_VARIANT_TYPE_STRING);
        EXPECT_TRUE(v != nullptr);
        g_variant_unref(v);
        v = g_variant_lookup_value(dict, "title", G_VARIANT_TYPE_STRING);
        EXPECT_TRUE(v != nullptr);
        g_variant_unref(v);
        v = g_variant_lookup_value(dict, "visible", G_VARIANT_TYPE_BOOLEAN);
        EXPECT_TRUE(v != nullptr);
        g_variant_unref(v);
        g_variant_unref(dict);

        g_free(action_name);
    }

    void InspectVisible(GMenuModel*, const char* name, bool expected_visibility)
    {
        auto action_name = g_strdup_printf("%s-header", name);
        auto dict = g_action_group_get_action_state(m_gactions->action_group(), action_name);
        EXPECT_TRUE(dict != nullptr);
        EXPECT_TRUE(g_variant_is_of_type(dict, G_VARIANT_TYPE_VARDICT));
        auto v = g_variant_lookup_value(dict, "visible", G_VARIANT_TYPE_BOOLEAN);
        EXPECT_TRUE(v != nullptr);
        EXPECT_EQ(expected_visibility, g_variant_get_boolean(v));
        g_variant_unref(v);
        g_variant_unref(dict);
        g_free(action_name);
    }
};


TEST_F(MenuFixture, HelloWorld)
{
    EXPECT_EQ(Menu::NUM_PROFILES, m_menus.size());
    for (int i=0; i<Menu::NUM_PROFILES; i++)
    {
        EXPECT_TRUE(m_menus[i] != false);
        EXPECT_TRUE(m_menus[i]->menu_model() != nullptr);
        EXPECT_EQ(i, m_menus[i]->profile());
    }
    EXPECT_STREQ(m_menus[Menu::PHONE]->name(), "phone");
}

TEST_F(MenuFixture, Header)
{
    for(auto& menu : m_menus)
      InspectHeader(menu->menu_model(), menu->name());
}

TEST_F(MenuFixture, Visible)
{
    // since there are initally no transfers,
    // the indicator should be hidden
    bool expected_visible = false;
    for(auto& menu : m_menus)
      InspectVisible(menu->menu_model(), menu->name(), expected_visible);

    // add a transfer to the list...
    EXPECT_EQ(0, m_transfers->get().size());
    std::vector<std::shared_ptr<Transfer>> transfers;
    const Transfer::Id id = "some-id";
    const std::string icon_filename = "/usr/share/icons/ubuntu-mobile/status/scalable/battery_charged.svg";
    std::shared_ptr<Transfer> transfer(new MockTransfer{id, icon_filename});
    transfers.push_back(transfer);
    m_transfers->set(transfers);
    EXPECT_EQ(1, m_transfers->get().size());

    // now the menu should be visible
    wait_msec();
    expected_visible = true;
    for(auto& menu : m_menus)
      InspectVisible(menu->menu_model(), menu->name(), expected_visible);
}


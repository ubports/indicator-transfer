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

#include <transfer/menu.h>

#include <transfer/gactions.h>
#include <transfer/transfer.h>

#include <glib/gi18n.h>
#include <gio/gio.h>

#include <memory> // shared_ptr
#include <vector>

namespace unity {
namespace indicator {
namespace transfer {

/****
*****
****/

Menu::Menu (Profile profile_in, const char* name_in):
    m_profile(profile_in),
    m_name(name_in)
{
}

const char* Menu::name() const
{
    return m_name;
}

Menu::Profile Menu::profile() const
{
    return m_profile;
}

GMenuModel* Menu::menu_model()
{
    Transfers tmp;

    return G_MENU_MODEL(m_menu);
}

/****
*****
****/

class MenuImpl: public Menu
{
protected:
    MenuImpl(const Menu::Profile profile_in,
             const char* name_in,
             const std::shared_ptr<Transfers>& transfers,
             const std::shared_ptr<GActions>& gactions):
        Menu(profile_in, name_in),
        m_transfers(transfers),
        m_gactions(gactions)
    {
        // initialize the menu
        create_gmenu();
        for (int i=0; i<NUM_SECTIONS; i++)
            update_section(Section(i));

        // listen for state changes so we can update the menu accordingly
        m_transfers->changed().connect([this](const Transfers::ValueType&){
            update_transfer_connections();
            update_header();
            update_section(TRANSFERS);
            update_section(BULK);
        });

        update_transfer_connections();
    }

    virtual ~MenuImpl()
    {
        g_clear_object(&m_menu);
    }

    virtual GVariant* create_header_state() =0;

    void update_header()
    {
        auto action_group = m_gactions->action_group();
        auto action_name = g_strdup_printf("%s-header", name());
        auto state = create_header_state();
        g_action_group_change_action_state(action_group, action_name, state);
        g_free(action_name);
    }

    std::shared_ptr<Transfers> m_transfers;
    std::shared_ptr<GActions> m_gactions;
    GMenu* m_submenu = nullptr;

private:

    void create_gmenu()
    {
        g_assert(m_submenu == nullptr);

        m_submenu = g_menu_new();

        // build placeholders for the sections
        for(int i=0; i<NUM_SECTIONS; i++)
        {
            GMenuItem * item = g_menu_item_new(nullptr, nullptr);
            g_menu_append_item(m_submenu, item);
            g_object_unref(item);
        }

        // add submenu to the header
        const auto detailed_action = (std::string("indicator.") + name()) + "-header";
        auto header = g_menu_item_new(nullptr, detailed_action.c_str());
        g_menu_item_set_attribute(header, "x-canonical-type", "s",
                                  "com.canonical.indicator.root");
        g_menu_item_set_attribute(header, "submenu-action", "s",
                                  "indicator.calendar-active");
        g_menu_item_set_submenu(header, G_MENU_MODEL(m_submenu));
        g_object_unref(m_submenu);

        // add header to the menu
        m_menu = g_menu_new();
        g_menu_append_item(m_menu, header);
        g_object_unref(header);
    }

    GMenuItem* create_transfer_menuitem(const std::shared_ptr<Transfer>& transfer)
    {
        const auto& id = transfer->id().c_str();
        auto menu_item = g_menu_item_new (transfer->id().c_str(), nullptr);
        //g_menu_item_set_attribute (menu_item, "x-canonical-type", "s", "com.canonical.indicator.transfer");
        g_menu_item_set_attribute (menu_item, "x-canonical-state", "i", transfer->state().get());
        g_menu_item_set_attribute (menu_item, "x-canonical-uid", "s", id);
        g_menu_item_set_attribute (menu_item, "x-canonical-time", "x", transfer->last_active().get());
        g_menu_item_set_action_and_target_value (menu_item, "indicator.activate-transfer", g_variant_new_string(id));
        return G_MENU_ITEM(menu_item);
    }

    void update_transfer(const std::shared_ptr<Transfer>& transfer)
    {
        g_message ("FIXME update menu for transfer '%s'", transfer->id().c_str());
    }

    GMenuModel* create_transfers_section()
    {
        auto menu = g_menu_new();

        for (const auto& transfer : m_transfers->get())
        {
            auto menu_item = create_transfer_menuitem(transfer);
            g_menu_append_item (menu, menu_item);
            g_object_unref (menu_item);
        }

        return G_MENU_MODEL(menu);
    }

    GMenuModel* create_bulk_actions_section()
    {
        auto menu = g_menu_new();

        unsigned int n_running = 0;
        unsigned int n_stopped = 0;
        unsigned int n_done = 0;
        for (const auto& transfer : m_transfers->get()) {
            switch (transfer->state()) {
                case Transfer::STARTING:
                case Transfer::RUNNING: ++n_running; break;
                case Transfer::FAILED:
                case Transfer::PAUSED: ++n_stopped; break;
                case Transfer::CANCELING:
                case Transfer::DONE: ++n_done; break;
            }
        }

        if (n_stopped)
            g_menu_append (menu, _("Resume All"), "indicator.resume-all");
        else if (n_running) 
            g_menu_append (menu, _("Pause All"), "indicator.pause-all");
        else if (n_done)
            g_menu_append (menu, _("Clear All"), "indicator.clear-all");

        return G_MENU_MODEL(menu);
    }

    void update_section(Section section)
    {
        GMenuModel * model;

        switch (section)
        {
            case BULK:      model = create_bulk_actions_section(); break;
            case TRANSFERS: model = create_transfers_section();    break;
            default:        model = nullptr; g_warn_if_reached();
        }

        if (model)
        {
            g_menu_remove(m_submenu, section);
            g_menu_insert_section(m_submenu, section, nullptr, model);
            g_object_unref(model);
        }
    }

    void update_transfer_connections()
    {
        // out with the old
        for(auto& connection : m_transfer_connections)
            connection.disconnect();
        m_transfer_connections.clear();

        // in with the new
        for (auto& transfer : m_transfers->get()) {
            auto connection = transfer->last_active().changed().connect([this,transfer](const time_t&) {
                update_header();
                update_transfer(transfer);
                update_section(BULK);
            });
            m_transfer_connections.push_back(connection);
        }
    }

    std::vector<core::Connection> m_transfer_connections;

}; // class MenuImpl

/***
****
***/

class PhoneBaseMenu: public MenuImpl
{
protected:
    PhoneBaseMenu(Menu::Profile profile_,
                  const char* name_,
                  const std::shared_ptr<Transfers>& transfers_,
                  const std::shared_ptr<GActions>& gactions_):
        MenuImpl(profile_, name_, transfers_, gactions_)
    {
        update_header();
    }

    GVariant* create_header_state()
    {
        auto title_v = g_variant_new_string(_("Transfers"));
        const auto visible = !m_transfers->get().empty();

        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&b, "{sv}", "title", title_v);
        g_variant_builder_add(&b, "{sv}", "label", title_v);
        g_variant_builder_add(&b, "{sv}", "accessible-desc", title_v);
        g_variant_builder_add(&b, "{sv}", "visible", g_variant_new_boolean(visible));
        return g_variant_builder_end (&b);
    }
};

class PhoneMenu: public PhoneBaseMenu
{
public:
    PhoneMenu(const std::shared_ptr<Transfers>& transfers_,
              const std::shared_ptr<GActions>& actions_):
        PhoneBaseMenu(PHONE, profile_names[PHONE], transfers_, actions_) {}
};

/****
*****
****/

MenuFactory::MenuFactory(const std::shared_ptr<Transfers>& transfers_,
                         const std::shared_ptr<GActions>& gactions_):
    m_transfers(transfers_),
    m_gactions(gactions_)
{
}

std::shared_ptr<Menu>
MenuFactory::buildMenu(Menu::Profile profile)
{
    std::shared_ptr<Menu> menu;

    switch (profile)
    {
    case Menu::PHONE:
        menu.reset(new PhoneMenu(m_transfers, m_gactions));
        break;

    default:
        g_warn_if_reached();
        break;
    }
    
    return menu;
}

/****
*****
****/

} // namespace transfer
} // namespace indicator
} // namespace unity

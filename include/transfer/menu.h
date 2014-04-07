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

#ifndef INDICATOR_TRANFER_MENU_H
#define INDICATOR_TRANFER_MENU_H

#include <transfer/gactions.h>
#include <transfer/transfer.h>

#include <memory>

#include <gio/gio.h> // GMenuModel

namespace unity {
namespace indicator {
namespace transfer {

/**
 * \brief A menu for a specific profile; eg, Desktop or Phone.
 *
 * @see MenuFactory
 * @see Exporter
 */
class Menu
{
public:
    enum Profile { PHONE, NUM_PROFILES };
    enum Section { TRANSFERS, BULK, NUM_SECTIONS };
    constexpr static const char* profile_names[] = { "phone" };

    const char* name() const;
    Profile profile() const;
    GMenuModel* menu_model();

protected:
    Menu (Profile profile_in, const char* name_in);
    virtual ~Menu() =default;
    GMenu* m_menu = nullptr;

private:
    const Profile m_profile;
    const char* const m_name;

    // we've got raw pointers in here, so disable copying
    Menu(const Menu&) =delete;
    Menu& operator=(const Menu&) =delete;
};

/**
 * \brief Builds a Menu for a given state and profile
 *
 * @see Menu
 * @see Exporter
 */
class MenuFactory
{
public:
    MenuFactory (const std::shared_ptr<Transfers>& transfers,
                 const std::shared_ptr<GActions>& gactions);

    std::shared_ptr<Menu> buildMenu(Menu::Profile profile);

private:
    std::shared_ptr<Transfers> m_transfers;
    std::shared_ptr<GActions> m_gactions;
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_MENU_H

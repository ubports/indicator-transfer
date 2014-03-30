/*
 * Copyright 2013 Canonical Ltd.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
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
 */

#include <transfer/actions-live.h> 
#include <transfer/exporter.h>
#include <transfer/menu.h>
#include <transfer/transfer-mock.h>
#include <transfer/transfer-source-mock.h>

#include <glib/gi18n.h> // bindtextdomain()
#include <gio/gio.h>

#include <locale.h>

using namespace unity::indicator::transfer;

int
main(int /*argc*/, char** /*argv*/)
{
    // Work around a deadlock in glib's type initialization.
    // It can be removed when https://bugzilla.gnome.org/show_bug.cgi?id=674885 is fixed.
    g_type_ensure(G_TYPE_DBUS_CONNECTION);

    // boilerplate i18n
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
    textdomain(GETTEXT_PACKAGE);

    // create the TransfersSource
    // FIXME: mock transfers, for now
    std::shared_ptr<Transfers> transfers (new Transfers);
    std::shared_ptr<MockTransferSource> mock_transfer_source (new MockTransferSource(transfers));
    std::shared_ptr<MockTransfer> mock_transfer(new MockTransfer ("aaa", "/usr/share/icons/ubuntu-mobile/status/scalable/battery_charged.svg")); 
    mock_transfer_source->add (std::dynamic_pointer_cast<Transfer>(mock_transfer));

    // create the Actions and the GActions bridge
    std::shared_ptr<Actions> actions (new LiveActions(transfers));
    std::shared_ptr<GActions> gactions(new GActions(actions));

    // create the Menus
    std::vector<std::shared_ptr<Menu>> menus;
    MenuFactory menu_factory (transfers, gactions);
    for(int i=0; i<Menu::NUM_PROFILES; i++)
      menus.push_back(menu_factory.buildMenu(Menu::Profile(i)));

    // export 'em and run until we lose the busname
    auto loop = g_main_loop_new(nullptr, false);
    Exporter exporter;
    exporter.name_lost.connect([loop](){
        g_message("%s exiting; failed/lost bus ownership", GETTEXT_PACKAGE);
        g_main_loop_quit(loop);
    });
    exporter.publish(gactions, menus);
    g_main_loop_run(loop);

    // cleanup
    g_main_loop_unref(loop);
    return 0;
}

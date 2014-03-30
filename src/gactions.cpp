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

#include <transfer/gactions.h>

#include <glib.h>
#include <gio/gio.h>

namespace unity {
namespace indicator {
namespace transfer {

/***
****
***/

namespace
{

void on_transfer_activate(GSimpleAction*, GVariant* vuid, gpointer gself)
{
    const auto uid = g_variant_get_string(vuid, nullptr);
    static_cast<GActions*>(gself)->actions()->activate(uid);
}

void on_transfer_cancel(GSimpleAction*, GVariant* vuid, gpointer gself)
{
    const auto uid = g_variant_get_string(vuid, nullptr);
    static_cast<GActions*>(gself)->actions()->cancel(uid);
}

void on_transfer_pause(GSimpleAction*, GVariant* vuid, gpointer gself)
{
    const auto uid = g_variant_get_string(vuid, nullptr);
    static_cast<GActions*>(gself)->actions()->pause(uid);
}

void on_transfer_resume(GSimpleAction*, GVariant* vuid, gpointer gself)
{
    const auto uid = g_variant_get_string(vuid, nullptr);
    static_cast<GActions*>(gself)->actions()->resume(uid);
}

void on_resume_all(GSimpleAction*, GVariant*, gpointer gself)
{
    static_cast<GActions*>(gself)->actions()->resume_all();
}

void on_clear_all(GSimpleAction*, GVariant*, gpointer gself)
{
    static_cast<GActions*>(gself)->actions()->clear_all();
}

void on_pause_all(GSimpleAction*, GVariant*, gpointer gself)
{
    static_cast<GActions*>(gself)->actions()->pause_all();
}

GVariant* create_default_header_state()
{
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&b, "{sv}", "accessible-desc", g_variant_new_string("accessible-desc"));
    g_variant_builder_add(&b, "{sv}", "label", g_variant_new_string("label"));
    g_variant_builder_add(&b, "{sv}", "title", g_variant_new_string("title"));
    g_variant_builder_add(&b, "{sv}", "visible", g_variant_new_boolean(true));
    return g_variant_builder_end(&b);
}


} // unnamed namespace

/***
****
***/

GActions::GActions(const std::shared_ptr<Actions>& actions):
    m_action_group(g_simple_action_group_new()),
    m_actions(actions)
{
    GActionEntry entries[] = {
        { "activate-transfer", on_transfer_activate, "s", nullptr },
        { "cancel-transfer", on_transfer_cancel, "s", nullptr },
        { "pause-transfer", on_transfer_pause, "s", nullptr },
        { "resume-transfer", on_transfer_resume, "s", nullptr },
        { "resume-all", on_resume_all },
        { "pause-all", on_pause_all },
        { "clear-all", on_clear_all }
    };

    auto gam = G_ACTION_MAP(m_action_group);
    g_action_map_add_action_entries(gam,
                                    entries,
                                    G_N_ELEMENTS(entries),
                                    this);

    // add the header actions
    auto v = create_default_header_state();
    auto a = g_simple_action_new_stateful("phone-header", nullptr, v);
    g_action_map_add_action(gam, G_ACTION(a));
}

GActions::~GActions()
{
    g_clear_object(&m_action_group);
}

GActionGroup*
GActions::action_group() const
{
    return G_ACTION_GROUP(m_action_group);
}

std::shared_ptr<Actions>&
GActions::actions()
{
    return m_actions;
}

} // namespace transfer
} // namespace indicator
} // namespace unity

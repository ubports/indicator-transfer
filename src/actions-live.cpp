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

#include <transfer/actions-live.h>

namespace unity {
namespace indicator {
namespace transfer {

/****
*****
****/

LiveActions::LiveActions(std::shared_ptr<Transfers>& transfers):
    m_transfers(transfers)
{
}

LiveActions::~LiveActions()
{
}

void
LiveActions::pause_all()
{
    g_message("FIXME: %s", G_STRFUNC);
}

void
LiveActions::resume_all()
{
    g_message("FIXME: %s", G_STRFUNC);
}

void
LiveActions::clear_all()
{
    g_message("FIXME: %s", G_STRFUNC);
}

void
LiveActions::activate(const Transfer::Id& id)
{
    g_message("FIXME: %s, id is \"%s\"", G_STRFUNC, id.c_str());
}

void
LiveActions::pause(const Transfer::Id& id)
{
    g_message("FIXME: %s, id is \"%s\"", G_STRFUNC, id.c_str());
}

void
LiveActions::cancel(const Transfer::Id& id)
{
    g_message("FIXME: %s, id is \"%s\"", G_STRFUNC, id.c_str());
}

void
LiveActions::resume(const Transfer::Id& id)
{
    g_message("FIXME: %s, id is \"%s\"", G_STRFUNC, id.c_str());
}

/****
*****
****/

} // namespace transfer
} // namespace indicator
} // namespace unity


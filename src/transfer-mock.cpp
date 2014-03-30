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

#include <transfer/transfer-mock.h>

#include <gio/gio.h>

namespace unity {
namespace indicator {
namespace transfer {

/***
****
***/

MockTransfer::MockTransfer(const Id& id, const std::string& icon_filename):
    m_id(id),
    m_icon_filename(icon_filename)
{
}

MockTransfer::~MockTransfer()
{
}

void
MockTransfer::pause()
{
    m_state.set(PAUSED);
    m_history.push_back(Pause);
}

void
MockTransfer::resume()
{
    m_state.set(RUNNING);
    m_history.push_back(Resume);
}

void
MockTransfer::cancel()
{
    m_state.set(CANCELING);
    m_history.push_back(Cancel);
}

void
MockTransfer::clear()
{
    m_history.push_back(Clear);
}

void
MockTransfer::open()
{
    g_warn_if_fail(state().get() != DONE);
    m_history.push_back(Open);
}

GIcon*
MockTransfer::icon() const
{
    auto file = g_file_new_for_path(m_icon_filename.c_str());
    auto icon = g_file_icon_new (file);
    g_object_unref(file);
    return icon;
}

/***
****
***/

} // namespace transfer
} // namespace indicator
} // namespace unity

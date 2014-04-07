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

std::shared_ptr<Transfer> LiveActions::find_transfer_by_id(const Transfer::Id& id) const
{
    for (const auto& transfer : m_transfers->get())
        if (transfer->id() == id)
            return transfer;

    return std::shared_ptr<Transfer>();
}

void
LiveActions::pause_all()
{
    for (const auto& transfer : m_transfers->get())
        transfer->pause();
}

void
LiveActions::resume_all()
{
    for (const auto& transfer : m_transfers->get())
        transfer->resume();
}

void
LiveActions::clear_all()
{
    for (const auto& transfer : m_transfers->get())
        transfer->clear();
}

void
LiveActions::activate(const Transfer::Id& id)
{
    auto transfer = find_transfer_by_id(id);
    g_return_if_fail(transfer);

    switch(transfer->state().get())
    {
        case Transfer::STARTING:
        case Transfer::RUNNING:
            transfer->pause();
            break;

        case Transfer::CANCELING:
            transfer->clear();
            break;

        case Transfer::PAUSED:
        case Transfer::FAILED:
            transfer->resume();
            break;

        case Transfer::DONE:
            transfer->open();
            break;
    }
}

void
LiveActions::pause(const Transfer::Id& id)
{
    auto transfer = find_transfer_by_id(id);
    g_return_if_fail(transfer);
    transfer->pause();
}

void
LiveActions::cancel(const Transfer::Id& id)
{
    auto transfer = find_transfer_by_id(id);
    g_return_if_fail(transfer);
    transfer->cancel();
}

void
LiveActions::resume(const Transfer::Id& id)
{
    auto transfer = find_transfer_by_id(id);
    g_return_if_fail(transfer);
    transfer->resume();
}

/****
*****
****/

} // namespace transfer
} // namespace indicator
} // namespace unity


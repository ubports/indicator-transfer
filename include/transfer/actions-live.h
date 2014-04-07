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

#ifndef INDICATOR_TRANSFER_ACTIONS_LIVE_H
#define INDICATOR_TRANSFER_ACTIONS_LIVE_H

#include <transfer/actions.h>

#include <transfer/transfer.h>

#include <memory>

namespace unity {
namespace indicator {
namespace transfer {

/**
 * \brief Interface for all the actions that can be activated by users.
 */
class LiveActions: public Actions
{
public:
    LiveActions(std::shared_ptr<Transfers>& transfers);
    ~LiveActions();

    void pause_all();
    void resume_all();
    void clear_all();
    void activate(const Transfer::Id&);
    void pause(const Transfer::Id&);
    void cancel(const Transfer::Id&);
    void resume(const Transfer::Id&);

private:
    std::shared_ptr<Transfer> find_transfer_by_id(const Transfer::Id&) const;
    std::shared_ptr<Transfers> m_transfers;
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_ACTIONS_LIVE_H

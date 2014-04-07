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

#ifndef INDICATOR_TRANSFER_ACTIONS_H
#define INDICATOR_TRANSFER_ACTIONS_H

#include <transfer/transfer.h>

namespace unity {
namespace indicator {
namespace transfer {

/**
 * \brief Interface for all the actions that can be activated by users.
 */
class Actions
{
public:
    virtual void pause_all() =0;
    virtual void resume_all() =0;
    virtual void clear_all() =0;
    virtual void activate(const Transfer::Id&) =0;
    virtual void pause(const Transfer::Id&) =0;
    virtual void cancel(const Transfer::Id&) =0;
    virtual void resume(const Transfer::Id&) =0;
    virtual ~Actions() =default;
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_ACTIONS_H

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

#ifndef INDICATOR_TRANSFER_TRANSFER_H
#define INDICATOR_TRANSFER_TRANSFER_H

#include <core/property.h> // parent class

#include <gio/gio.h> // GIcon

#include <ctime> // time_t
#include <memory>
#include <string>
#include <vector>

namespace unity {
namespace indicator {
namespace transfer {

struct Transfer
{
    virtual ~Transfer() =default;

    typedef std::string Id;
    virtual Id id() const =0;
    virtual GIcon* icon() const =0;
    virtual core::Property<time_t>& last_active() =0;

    typedef enum { STARTING, RUNNING, CANCELING, PAUSED, DONE, FAILED } State;
    virtual core::Property<State>& state() =0;

    virtual void cancel() =0;
    virtual void pause() =0;
    virtual void resume() =0;
};

typedef core::Property<std::vector<std::shared_ptr<Transfer>>> Transfers;

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_TRANSFER_H

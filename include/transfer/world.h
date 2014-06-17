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

#ifndef INDICATOR_TRANSFER_WORLD_H
#define INDICATOR_TRANSFER_WORLD_H

#include <transfer/model.h>
#include <transfer/transfer.h> // Id

#include <memory> // std::shared_ptr

namespace unity {
namespace indicator {
namespace transfer {

/**
 * \brief Facade for everything outside of indicator-transfer
 */
class World
{
public:
    virtual ~World();

    virtual void open(const Transfer::Id& id) =0;
    virtual void start(const Transfer::Id& id) =0;
    virtual void pause(const Transfer::Id& id) =0;
    virtual void resume(const Transfer::Id& id) =0;
    virtual void cancel(const Transfer::Id& id) =0;
    virtual void open_app(const Transfer::Id& id) =0;
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_WORLD_H

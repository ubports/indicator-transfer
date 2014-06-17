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

#ifndef INDICATOR_TRANSFER_CONTROLLER_H
#define INDICATOR_TRANSFER_CONTROLLER_H

#include <transfer/model.h>
#include <transfer/transfer.h>
#include <transfer/world.h>

#include <memory> // std::shared_ptr

namespace unity {
namespace indicator {
namespace transfer {

/**
 * \brief Process actions triggered by views
 */
class Controller
{
public:
    Controller(const std::shared_ptr<MutableModel>& model,
               const std::shared_ptr<World>& world);
    virtual ~Controller();

    virtual void pause_all();
    virtual void resume_all();
    virtual void clear_all();
    virtual void tap(const Transfer::Id&);
    virtual void start(const Transfer::Id&);
    virtual void pause(const Transfer::Id&);
    virtual void cancel(const Transfer::Id&);
    virtual void resume(const Transfer::Id&);
    virtual void open(const Transfer::Id&);
    virtual void open_app(const Transfer::Id&);

private:
    std::shared_ptr<MutableModel> m_model;
    std::shared_ptr<World> m_world;
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_CONTROLLER_H

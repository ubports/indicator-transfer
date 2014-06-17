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

#ifndef INDICATOR_TRANSFER_VIEW_CONSOLE_H
#define INDICATOR_TRANSFER_VIEW_CONSOLE_H

#include <transfer/view.h>

#include <core/connection.h>

#include <memory> // shared_ptr
#include <set>

namespace unity {
namespace indicator {
namespace transfer {

/**
 * \brief a debugging view that dumps output to the console
 */
class ConsoleView: public View
{
public:
    ConsoleView(const std::shared_ptr<Model>&, const std::shared_ptr<Controller>&);
    ~ConsoleView();
    void set_controller(const std::shared_ptr<Controller>&);
    void set_model(const std::shared_ptr<Model>&);

private:
    std::shared_ptr<Model> m_model;
    std::shared_ptr<Controller> m_controller;
    std::set<core::ScopedConnection> m_connections;
    static gboolean on_timer (gpointer);
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_VIEW_H

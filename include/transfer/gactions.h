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

#ifndef INDICATOR_TRANSFER_GACTIONS_H
#define INDICATOR_TRANSFER_GACTIONS_H

#include <transfer/actions.h>

#include <memory> // shared_ptr

namespace unity {
namespace indicator {
namespace transfer {

/**
 * \brief Mediator to couple our Actions class with a GActionGroup exported on the bus.
 */
class GActions
{
public:
    GActions(const std::shared_ptr<Actions>& actions);
    ~GActions();

    GActionGroup* action_group() const;
    std::shared_ptr<Actions>& actions();

private:
    GSimpleActionGroup* m_action_group = nullptr;
    std::shared_ptr<Actions> m_actions;

    // we've got raw pointers in here, so disable copying
    GActions(const Actions&) =delete;
    GActions& operator=(const Actions&) =delete;
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_GACTIONS_H

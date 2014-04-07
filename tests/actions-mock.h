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

#ifndef INDICATOR_TRANSFER_ACTIONS_MOCK_H
#define INDICATOR_TRANSFER_ACTIONS_MOCK_H

#include <transfer/actions.h>
#include <transfer/transfer.h>

namespace unity {
namespace indicator {
namespace transfer {

/**
 * \brief Mock implentation of Actions for tests
 */
class MockActions: public Actions
{
public:
    ~MockActions() =default;

    enum Action { PauseAll, ResumeAll, Activate, ClearAll, Pause, Cancel, Resume };
    const std::string& id() const { return m_id; }
    const std::vector<Action>& history() const { return m_history; }

    void pause_all() { m_history.push_back(PauseAll); }
    void resume_all() { m_history.push_back(ResumeAll); }
    void clear_all() { m_history.push_back(ClearAll); }
    void pause (const Transfer::Id& id) { m_history.push_back(Pause); m_id=id; }
    void cancel (const Transfer::Id& id) { m_history.push_back(Cancel); m_id=id; }
    void resume (const Transfer::Id& id) { m_history.push_back(Resume); m_id=id; }
    void activate (const Transfer::Id& id) { m_history.push_back(Activate); m_id=id; }

    void reset() { m_id.clear(); m_history.clear(); }

private:
    std::vector<Action> m_history;
    Transfer::Id m_id;
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_ACTIONS_MOCK_H

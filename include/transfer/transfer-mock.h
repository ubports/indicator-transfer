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

#ifndef INDICATOR_TRANSFER_MOCK_H
#define INDICATOR_TRANSFER_MOCK_H

#include <transfer/transfer.h>

namespace unity {
namespace indicator {
namespace transfer {

struct MockTransfer: public Transfer
{
public:
    MockTransfer(const Id& id, const std::string& icon_filename);
    ~MockTransfer();

    Id id() const { return m_id; }
    core::Property<State>& state() { return m_state; }
    GIcon* icon() const;

    core::Property<time_t>& last_active() { return m_last_active; }

    void pause();
    void resume();
    void cancel();
    void set_last_active(time_t t) { m_last_active.set(t); }

private:
    const Id m_id;
    const std::string m_icon_filename;
    core::Property<State> m_state;
    core::Property<time_t> m_last_active;
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_MOCK_H

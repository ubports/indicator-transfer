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

#include <transfer/transfer-controller-mock.h>

#include <algorithm>

namespace unity {
namespace indicator {
namespace transfer {

/***
****
***/

MockTransferController::MockTransferController(const std::shared_ptr<Transfers>& transfers):
    TransferController(transfers)
{
}

MockTransferController::~MockTransferController()
{
}

void
MockTransferController::add(const std::shared_ptr<Transfer>& transfer)
{
    auto tmp = m_transfers->get();
    tmp.push_back(transfer);
    m_transfers->set(tmp);
}

void
MockTransferController::remove(const Transfer::Id& id)
{
    auto predicate = [id](const std::shared_ptr<Transfer>& t){return t->id()==id;};
    auto tmp = m_transfers->get();
    auto new_end = std::remove_if (tmp.begin(), tmp.end(), predicate);
    tmp.erase(new_end, tmp.end());
    m_transfers->set(tmp);
}

/***
****
***/

} // namespace transfer
} // namespace indicator
} // namespace unity

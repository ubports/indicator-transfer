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

#ifndef INDICATOR_TRANSFER_TRANSFER_CONTROLLER_MOCK_H
#define INDICATOR_TRANSFER_TRANSFER_CONTROLLER_MOCK_H

#include <transfer/transfer-controller.h>

namespace unity {
namespace indicator {
namespace transfer {

struct MockTransferController: public TransferController
{
public:
    MockTransferController(const std::shared_ptr<Transfers>& transfers);
    ~MockTransferController();

    void add(const std::shared_ptr<Transfer>& transfer);
    void remove(const Transfer::Id&);
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_TRANSFER_CONTROLLER_MOCK_H

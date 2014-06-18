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

#ifndef INDICATOR_TRANSFER_CONTROLLER_MOCK_H
#define INDICATOR_TRANSFER_CONTROLLER_MOCK_H

#include <transfer/controller.h> // parent class

#include "gmock/gmock.h"

namespace unity {
namespace indicator {
namespace transfer {

class MockController: public Controller
{
public:
  MockController(const std::shared_ptr<MutableModel>& model, 
                 const std::shared_ptr<World>& world):
    Controller(model, world) {}

  MOCK_METHOD0(pause_all, void());
  MOCK_METHOD0(resume_all, void());
  MOCK_METHOD0(clear_all, void());
  MOCK_METHOD1(tap, void(const Transfer::Id&));
  MOCK_METHOD1(start, void(const Transfer::Id&));
  MOCK_METHOD1(pause, void(const Transfer::Id&));
  MOCK_METHOD1(cancel, void(const Transfer::Id&));
  MOCK_METHOD1(resume, void(const Transfer::Id&));
  MOCK_METHOD1(open, void(const Transfer::Id&));
  MOCK_METHOD1(open_app, void(const Transfer::Id&));
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_CONTROLLER_MOCK_H

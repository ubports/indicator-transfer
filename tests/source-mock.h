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

#ifndef INDICATOR_TRANSFER_SOURCE_MOCK_H
#define INDICATOR_TRANSFER_SOURCE_MOCK_H

#include <transfer/source.h>

#include "gmock/gmock.h"

namespace unity {
namespace indicator {
namespace transfer {

/**
 * \brief a Source that gets its updates & events from DBus
 */
class MockSource: public Source
{
public:
  MockSource(): m_model(new MutableModel)
  {
    // make sure that the transfer get removed from model on clear call
    ON_CALL(*this, clear(::testing::_))
            .WillByDefault(::testing::Invoke(m_model.get(), &MutableModel::remove));
  }

  MOCK_METHOD1(open, void(const Transfer::Id&));
  MOCK_METHOD1(start, void(const Transfer::Id&));
  MOCK_METHOD1(pause, void(const Transfer::Id&));
  MOCK_METHOD1(resume, void(const Transfer::Id&));
  MOCK_METHOD1(cancel, void(const Transfer::Id&));
  MOCK_METHOD1(clear, void(const Transfer::Id&));
  MOCK_METHOD1(update, void(const Transfer::Id&));
  MOCK_METHOD1(open_app, void(const Transfer::Id&));

  const std::shared_ptr<const MutableModel> get_model() override {return m_model;}
  std::shared_ptr<MutableModel> m_model;
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_SOURCE_MOCK_H

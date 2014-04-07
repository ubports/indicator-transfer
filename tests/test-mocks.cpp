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

#include <transfer/transfer-mock.h>
#include <transfer/transfer-controller-mock.h>
#include "actions-mock.h"

#include <gtest/gtest.h>

// who watches the watchmen?

using namespace unity::indicator::transfer;

TEST(TestMocks, TestMockTransfer)
{
    const Transfer::Id id = "some-id";
    const std::string icon_filename = "/usr/share/icons/ubuntu-mobile/status/scalable/battery_charged.svg";
    std::shared_ptr<MockTransfer> mock_transfer(new MockTransfer(id, icon_filename));
    std::shared_ptr<Transfer> transfer = std::dynamic_pointer_cast<Transfer>(mock_transfer);

    EXPECT_EQ(transfer->id(), id);

    EXPECT_EQ(transfer->state().get(), Transfer::STARTING);
    transfer->pause();
    EXPECT_EQ(transfer->state().get(), Transfer::PAUSED);
    transfer->resume();
    EXPECT_EQ(transfer->state().get(), Transfer::RUNNING);
    transfer->cancel();
    EXPECT_EQ(transfer->state().get(), Transfer::CANCELING);

    auto file = g_file_new_for_path(icon_filename.c_str());
    auto expected_icon = g_file_icon_new(file);
    auto icon = transfer->icon();
    EXPECT_TRUE(g_icon_equal(expected_icon, icon));
    g_object_unref(icon);
    g_object_unref(expected_icon);

    const auto now = time(nullptr);
    EXPECT_EQ(0, transfer->last_active().get());
    mock_transfer->set_last_active(now);
    EXPECT_EQ(now, transfer->last_active().get());
}

TEST(TestMocks, TestMockTransferController)
{
    std::shared_ptr<Transfers> transfers (new Transfers);
    std::shared_ptr<MockTransferController> mock_transfer_controller (new MockTransferController(transfers));
    EXPECT_EQ(0, transfers->get().size());

    const Transfer::Id id = "some-id";
    const std::string icon_filename = "/usr/share/icons/ubuntu-mobile/status/scalable/battery_charged.svg";
    std::shared_ptr<MockTransfer> mock_transfer(new MockTransfer (id, icon_filename));
    mock_transfer_controller->add (std::dynamic_pointer_cast<Transfer>(mock_transfer));
    EXPECT_EQ(1, transfers->get().size());

    mock_transfer_controller->remove(id);
    EXPECT_EQ(0, transfers->get().size());
}

TEST(TestMocks, TestMockActions)
{
    MockActions actions;
    std::vector<MockActions::Action> expected_history;
    EXPECT_TRUE(actions.id().empty());

    actions.pause_all();
    expected_history.push_back(MockActions::PauseAll);
    EXPECT_EQ(expected_history, actions.history());

    actions.resume_all();
    expected_history.push_back(MockActions::ResumeAll);
    EXPECT_EQ(expected_history, actions.history());

    actions.clear_all();
    expected_history.push_back(MockActions::ClearAll);
    EXPECT_EQ(expected_history, actions.history());

    Transfer::Id id = "aaa";
    actions.pause(id);
    expected_history.push_back(MockActions::Pause);
    EXPECT_EQ(expected_history, actions.history());
    EXPECT_EQ(actions.id(), id);

    id = "bbb";
    actions.cancel(id);
    expected_history.push_back(MockActions::Cancel);
    EXPECT_EQ(expected_history, actions.history());
    EXPECT_EQ(actions.id(), id);

    id = "ccc";
    actions.resume(id);
    expected_history.push_back(MockActions::Resume);
    EXPECT_EQ(expected_history, actions.history());
    EXPECT_EQ(actions.id(), id);

    id = "ddd";
    actions.activate(id);
    expected_history.push_back(MockActions::Activate);
    EXPECT_EQ(expected_history, actions.history());
    EXPECT_EQ(actions.id(), id);
}

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

#include <transfer/actions-live.h>
#include <transfer/transfer-mock.h>

#include "glib-fixture.h"

#include <functional>

using namespace unity::indicator::transfer;
using namespace std::placeholders;

class ActionsFixture: public GlibFixture
{
    typedef GlibFixture super;

protected:

    const std::vector<MockTransfer::Action> empty_history;

    std::shared_ptr<MockTransfer> m_transfer_a;
    std::shared_ptr<MockTransfer> m_transfer_b;
    std::shared_ptr<Transfers> m_transfers;
    std::shared_ptr<LiveActions> m_live_actions;
    std::shared_ptr<Actions> m_actions;

    virtual void SetUp()
    {
        super::SetUp();

        m_transfer_a.reset(new MockTransfer("id-a", "/usr/share/icons/ubuntu-mobile/status/scalable/search.svg"));
        m_transfer_b.reset(new MockTransfer("id-b", "/usr/share/icons/ubuntu-mobile/status/scalable/syncing.svg"));
        m_transfers.reset(new Transfers({std::dynamic_pointer_cast<Transfer>(m_transfer_a),
                                         std::dynamic_pointer_cast<Transfer>(m_transfer_b)}));
        m_live_actions.reset(new LiveActions(m_transfers));
        m_actions = std::dynamic_pointer_cast<Actions>(m_live_actions);
    }

    virtual void TearDown()
    {
        m_actions.reset();
        m_live_actions.reset();
        m_transfers.reset();
        m_transfer_b.reset();
        m_transfer_a.reset();

        super::TearDown();
    }

    void TestIdFunc(std::function<void(const Transfer::Id&)> callme, MockTransfer::Action expected_action)
    {
        const std::vector<MockTransfer::Action> expected_history({expected_action});

        EXPECT_EQ(empty_history, m_transfer_a->history());
        EXPECT_EQ(empty_history, m_transfer_b->history());

        callme(m_transfer_a->id());
        EXPECT_EQ(expected_history, m_transfer_a->history());
        EXPECT_EQ(empty_history, m_transfer_b->history());

        callme(m_transfer_b->id());
        EXPECT_EQ(expected_history, m_transfer_a->history());
        EXPECT_EQ(expected_history, m_transfer_b->history());
    }

    void TestBatchFunc(std::function<void()> callme, MockTransfer::Action expected_action)
    {
        EXPECT_EQ(empty_history, m_transfer_a->history());
        EXPECT_EQ(empty_history, m_transfer_b->history());

        callme();

        const std::vector<MockTransfer::Action> expected_history({expected_action});
        EXPECT_EQ(expected_history, m_transfer_a->history());
        EXPECT_EQ(expected_history, m_transfer_b->history());
    }
};

/***
****
***/

TEST_F(ActionsFixture, Pause)
{
    TestIdFunc(std::bind(&LiveActions::pause, *m_live_actions, _1),
               MockTransfer::Pause);
}

TEST_F(ActionsFixture, Cancel)
{
    TestIdFunc(std::bind(&LiveActions::cancel, *m_live_actions, _1),
               MockTransfer::Cancel);
}

TEST_F(ActionsFixture, Resume)
{
    TestIdFunc(std::bind(&LiveActions::resume, *m_live_actions, _1),
               MockTransfer::Resume);
}

TEST_F(ActionsFixture, PauseAll)
{
    TestBatchFunc(std::bind(&LiveActions::pause_all, *m_live_actions),
                  MockTransfer::Pause);
}

TEST_F(ActionsFixture, ResumeAll)
{
    TestBatchFunc(std::bind(&LiveActions::resume_all, *m_live_actions),
                  MockTransfer::Resume);
}

TEST_F(ActionsFixture, ClearAll)
{
    TestBatchFunc(std::bind(&LiveActions::clear_all, *m_live_actions),
                  MockTransfer::Clear);
}

TEST_F(ActionsFixture, Activate)
{
    std::vector<MockTransfer::Action> expected_history;
    EXPECT_EQ(expected_history, m_transfer_a->history());

    m_live_actions->activate(m_transfer_a->id());
    expected_history.push_back(MockTransfer::Pause);
    EXPECT_EQ(expected_history, m_transfer_a->history());

    m_live_actions->activate(m_transfer_a->id());
    expected_history.push_back(MockTransfer::Resume);
    EXPECT_EQ(expected_history, m_transfer_a->history());

    m_transfer_a->state().set(Transfer::DONE);
    m_live_actions->activate(m_transfer_a->id());
    expected_history.push_back(MockTransfer::Open);
    EXPECT_EQ(expected_history, m_transfer_a->history());

    m_transfer_a->state().set(Transfer::CANCELING);
    m_live_actions->activate(m_transfer_a->id());
    expected_history.push_back(MockTransfer::Clear);
    EXPECT_EQ(expected_history, m_transfer_a->history());
}

TEST_F(ActionsFixture, InvalidId)
{
    EXPECT_EQ(empty_history, m_transfer_a->history());
    EXPECT_EQ(empty_history, m_transfer_b->history());

    m_live_actions->pause("unknown-id");
    increment_expected_errors(G_LOG_LEVEL_CRITICAL);
    EXPECT_EQ(empty_history, m_transfer_a->history());
    EXPECT_EQ(empty_history, m_transfer_b->history());

    m_live_actions->resume("unknown-id");
    increment_expected_errors(G_LOG_LEVEL_CRITICAL);
    EXPECT_EQ(empty_history, m_transfer_a->history());
    EXPECT_EQ(empty_history, m_transfer_b->history());

    m_live_actions->cancel("unknown-id");
    increment_expected_errors(G_LOG_LEVEL_CRITICAL);
    EXPECT_EQ(empty_history, m_transfer_a->history());
    EXPECT_EQ(empty_history, m_transfer_b->history());

    m_live_actions->resume("unknown-id");
    increment_expected_errors(G_LOG_LEVEL_CRITICAL);
    EXPECT_EQ(empty_history, m_transfer_a->history());
    EXPECT_EQ(empty_history, m_transfer_b->history());
}

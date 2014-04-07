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

#include <transfer/gactions.h>

#include "actions-mock.h"
#include "glib-fixture.h"

using namespace unity::indicator::transfer;

class GActionsFixture: public GlibFixture
{
    typedef GlibFixture super;

protected:

    std::shared_ptr<MockActions> m_mock_actions;
    std::shared_ptr<Actions> m_actions;
    std::shared_ptr<GActions> m_gactions;

    virtual void SetUp()
    {
        super::SetUp();

        m_mock_actions.reset(new MockActions{});
        m_actions = std::dynamic_pointer_cast<Actions>(m_mock_actions);
        m_gactions.reset(new GActions{m_actions});
    }

    virtual void TearDown()
    {
        m_gactions.reset();
        m_actions.reset();
        m_mock_actions.reset();

        super::TearDown();
    }

    void test_action_with_no_args(const char * action_name,
                                  MockActions::Action expected_action)
    {
        // preconditions
        EXPECT_TRUE(m_mock_actions->history().empty());
        auto action_group = m_gactions->action_group();
        EXPECT_TRUE(g_action_group_has_action(action_group, action_name));

        // run the test
        g_action_group_activate_action(action_group, action_name, nullptr);

        // test the results
        EXPECT_EQ(std::vector<MockActions::Action>{expected_action},
                  m_mock_actions->history());
    }

    void test_action_with_string_arg(const char * action_name,
                                     const std::string& param,
                                     MockActions::Action expected_action)
    {
        // preconditions
        EXPECT_TRUE(m_mock_actions->history().empty());
        auto action_group = m_gactions->action_group();
        EXPECT_TRUE(g_action_group_has_action(action_group, action_name));

        // activate the action
        auto v = g_variant_new_string(param.c_str());
        g_action_group_activate_action(action_group, action_name, v);

        // test the results
        EXPECT_EQ(std::vector<MockActions::Action>{expected_action},
                  m_mock_actions->history());
        EXPECT_EQ(param, m_mock_actions->id());
    }

};

/***
****
***/

TEST_F(GActionsFixture, ActionsExist)
{
    EXPECT_TRUE(m_actions != nullptr);

    const char* names[] = { "phone-header",
                            "activate-transfer",
                            "cancel-transfer",
                            "pause-transfer",
                            "resume-transfer",
                            "pause-all",
                            "resume-all",
                            "clear-all" };

    for(const auto& name: names)
    {
        EXPECT_TRUE(g_action_group_has_action(m_gactions->action_group(), name));
    }
}

/***
****
***/

TEST_F(GActionsFixture, PauseAll)
{
    test_action_with_no_args("pause-all", MockActions::PauseAll);
}

TEST_F(GActionsFixture, ResumeAll)
{
    test_action_with_no_args("resume-all", MockActions::ResumeAll);
}

TEST_F(GActionsFixture, ClearAll)
{
    test_action_with_no_args("clear-all", MockActions::ClearAll);
}

TEST_F(GActionsFixture, Activate)
{
    test_action_with_string_arg("activate-transfer", "aaa", MockActions::Activate);
}

TEST_F(GActionsFixture, Cancel)
{
    test_action_with_string_arg("cancel-transfer", "bbb", MockActions::Cancel);
}

TEST_F(GActionsFixture, Pause)
{
    test_action_with_string_arg("pause-transfer", "ccc", MockActions::Pause);
}

TEST_F(GActionsFixture, Resume)
{
    test_action_with_string_arg("resume-transfer", "ddd", MockActions::Resume);
}


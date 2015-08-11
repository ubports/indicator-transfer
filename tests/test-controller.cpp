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

#include "glib-fixture.h"
#include "source-mock.h"

#include <transfer/controller.h>


using namespace unity::indicator::transfer;

class ControllerFixture: public GlibFixture
{
private:

  typedef GlibFixture super;

protected:

  GTestDBus* bus = nullptr;

  std::shared_ptr<MockSource> m_source;
  std::shared_ptr<Controller> m_controller;

  void SetUp()
  {
    super::SetUp();

    m_source.reset(new MockSource);
    m_controller.reset(new Controller(m_source));
  }

  void TearDown()
  {
    m_controller.reset();
    m_source.reset();

    super::TearDown();
  }
};

namespace
{
  static constexpr Transfer::State all_states[] = {
    Transfer::QUEUED,
    Transfer::RUNNING,
    Transfer::PAUSED,
    Transfer::CANCELED,
    Transfer::HASHING,
    Transfer::PROCESSING,
    Transfer::FINISHED,
    Transfer::ERROR
  };
}

TEST_F(ControllerFixture, HelloSource)
{
  // confirms that the Test DBus SetUp() and TearDown() works
}

/**
 * Test which Transfers get cleared when 'clear_all' is called
 */
TEST_F(ControllerFixture, ClearAll)
{
  struct {
    Transfer::State state;
    bool can_clear;
    Transfer::Id id;
  } transfers[] = {
    { Transfer::QUEUED, false, "queued" },
    { Transfer::RUNNING, false, "running" },
    { Transfer::PAUSED, false, "paused" },
    { Transfer::CANCELED, false, "canceled" },
    { Transfer::HASHING, false, "hashing" },
    { Transfer::PROCESSING, false, "processing" },
    { Transfer::FINISHED, true, "finished" },
    { Transfer::ERROR, false, "error" }
  };

  for (const auto& transfer : transfers)
    {
      auto t = std::make_shared<Transfer>();
      t->state = transfer.state;
      t->id = transfer.id;
      m_source->add_transfer(t);
    }

  // make sure all the transfers made it into the model
  EXPECT_EQ(G_N_ELEMENTS(transfers), m_controller->size());
  for (const auto& transfer : transfers)
    {
      EXPECT_EQ(1, m_controller->count(transfer.id));
      EXPECT_CALL(*m_source, clear(transfer.id)).Times(transfer.can_clear?1:0);
    }

  // call clear-all
  m_controller->clear_all();

  // make sure all the clearable transfers are gone from controler and source
  auto source_ids = m_source->get_model()->get_ids();

  for (const auto& transfer : transfers)
    {
      int expect_count = (transfer.can_clear ? 0 : 1);
      EXPECT_EQ(source_ids.count(transfer.id), expect_count);
      EXPECT_EQ(m_controller->count(transfer.id), expect_count);
    }
}

/**
 * Test which Transfers get selected when 'pause_all' is called
 */
TEST_F(ControllerFixture, PauseAll)
{
  struct {
    Transfer::State state;
    bool can_pause;
    Transfer::Id id;
  } transfers[] = {
    { Transfer::QUEUED, true, "queued" },
    { Transfer::RUNNING, true, "running" },
    { Transfer::PAUSED, false, "paused" },
    { Transfer::CANCELED, false, "canceled" },
    { Transfer::HASHING, true, "hashing" },
    { Transfer::PROCESSING, true, "processing" },
    { Transfer::FINISHED, false, "finished" },
    { Transfer::ERROR, false, "error" }
  };

  for (const auto& transfer : transfers)
    {
      auto t = std::make_shared<Transfer>();
      t->state = transfer.state;
      t->id = transfer.id;
      m_source->add_transfer(t);
      EXPECT_EQ(transfer.can_pause, t->can_pause());
      EXPECT_CALL(*m_source, pause(transfer.id)).Times(transfer.can_pause?1:0);
    }

  m_controller->pause_all();
}

/**
 * Test which Transfers get selected when 'resume_all' is called
 */
TEST_F(ControllerFixture, ResumeAll)
{
  struct {
    Transfer::State state;
    bool can_resume;
    Transfer::Id id;
  } transfers[] = {
    { Transfer::QUEUED, false, "queued" },
    { Transfer::RUNNING, false, "running" },
    { Transfer::PAUSED, true, "paused" },
    { Transfer::CANCELED, true, "canceled" },
    { Transfer::HASHING, false, "hashing" },
    { Transfer::PROCESSING, false, "processing" },
    { Transfer::FINISHED, false, "finished" },
    { Transfer::ERROR, true, "error" }
  };

  for (const auto& transfer : transfers)
    {
      auto t = std::make_shared<Transfer>();
      t->state = transfer.state;
      t->id = transfer.id;
      m_source->add_transfer(t);
      EXPECT_EQ(transfer.can_resume, t->can_resume());
      EXPECT_CALL(*m_source, resume(transfer.id)).Times(transfer.can_resume?1:0);
    }

  m_controller->resume_all();
}

/**
 * Test what happens when you tap a Transfer
 */
TEST_F(ControllerFixture, Tap)
{
  const Transfer::Id id = "id";

  auto t = std::make_shared<Transfer>();
  t->state = Transfer::QUEUED;
  t->id = id;
  m_source->add_transfer(t);

  t->state = Transfer::QUEUED;
  EXPECT_CALL(*m_source, start(id)).Times(1);
  EXPECT_CALL(*m_source, pause(id)).Times(0);
  EXPECT_CALL(*m_source, resume(id)).Times(0);
  EXPECT_CALL(*m_source, cancel(id)).Times(0);
  EXPECT_CALL(*m_source, open(id)).Times(0);
  m_controller->tap(id);

  t->state = Transfer::RUNNING;
  EXPECT_CALL(*m_source, start(id)).Times(0);
  EXPECT_CALL(*m_source, pause(id)).Times(1);
  EXPECT_CALL(*m_source, resume(id)).Times(0);
  EXPECT_CALL(*m_source, cancel(id)).Times(0);
  EXPECT_CALL(*m_source, open(id)).Times(0);
  m_controller->tap(id);

  t->state = Transfer::PAUSED;
  EXPECT_CALL(*m_source, start(id)).Times(0);
  EXPECT_CALL(*m_source, pause(id)).Times(0);
  EXPECT_CALL(*m_source, resume(id)).Times(1);
  EXPECT_CALL(*m_source, cancel(id)).Times(0);
  EXPECT_CALL(*m_source, open(id)).Times(0);
  m_controller->tap(id);

  t->state = Transfer::CANCELED;
  EXPECT_CALL(*m_source, start(id)).Times(0);
  EXPECT_CALL(*m_source, pause(id)).Times(0);
  EXPECT_CALL(*m_source, resume(id)).Times(1);
  EXPECT_CALL(*m_source, cancel(id)).Times(0);
  EXPECT_CALL(*m_source, open(id)).Times(0);
  m_controller->tap(id);

  t->state = Transfer::ERROR;
  EXPECT_CALL(*m_source, start(id)).Times(0);
  EXPECT_CALL(*m_source, pause(id)).Times(0);
  EXPECT_CALL(*m_source, resume(id)).Times(1);
  EXPECT_CALL(*m_source, cancel(id)).Times(0);
  EXPECT_CALL(*m_source, open(id)).Times(0);
  m_controller->tap(id);

  t->state = Transfer::HASHING;
  EXPECT_CALL(*m_source, start(id)).Times(0);
  EXPECT_CALL(*m_source, pause(id)).Times(1);
  EXPECT_CALL(*m_source, resume(id)).Times(0);
  EXPECT_CALL(*m_source, cancel(id)).Times(0);
  EXPECT_CALL(*m_source, open(id)).Times(0);
  m_controller->tap(id);

  t->state = Transfer::PROCESSING;
  EXPECT_CALL(*m_source, start(id)).Times(0);
  EXPECT_CALL(*m_source, pause(id)).Times(1);
  EXPECT_CALL(*m_source, resume(id)).Times(0);
  EXPECT_CALL(*m_source, cancel(id)).Times(0);
  EXPECT_CALL(*m_source, open(id)).Times(0);
  m_controller->tap(id);

  t->state = Transfer::FINISHED;
  EXPECT_CALL(*m_source, start(id)).Times(0);
  EXPECT_CALL(*m_source, pause(id)).Times(0);
  EXPECT_CALL(*m_source, resume(id)).Times(0);
  EXPECT_CALL(*m_source, cancel(id)).Times(0);
  EXPECT_CALL(*m_source, open(id)).Times(1);
  m_controller->tap(id);
}

TEST_F(ControllerFixture, Start)
{
  const Transfer::Id id = "id";
  auto t = std::make_shared<Transfer>();
  t->id = id;
  t->state = Transfer::QUEUED;
  m_source->add_transfer(t);

  for (const auto& state : all_states)
    {
      t->state = state;
      EXPECT_CALL(*m_source, start(id)).Times(t->can_start()?1:0);
      EXPECT_CALL(*m_source, pause(id)).Times(0);
      EXPECT_CALL(*m_source, resume(id)).Times(0);
      EXPECT_CALL(*m_source, cancel(id)).Times(0);
      EXPECT_CALL(*m_source, open(id)).Times(0);
      m_controller->start(id);
    }
}

TEST_F(ControllerFixture, Pause)
{
  const Transfer::Id id = "id";
  auto t = std::make_shared<Transfer>();
  t->id = id;
  t->state = Transfer::QUEUED;
  m_source->add_transfer(t);

  for (const auto& state : all_states)
    {
      t->state = state;
      EXPECT_CALL(*m_source, start(id)).Times(0);
      EXPECT_CALL(*m_source, pause(id)).Times(t->can_pause()?1:0);
      EXPECT_CALL(*m_source, resume(id)).Times(0);
      EXPECT_CALL(*m_source, cancel(id)).Times(0);
      EXPECT_CALL(*m_source, open(id)).Times(0);
      m_controller->pause(id);
    }
}

TEST_F(ControllerFixture, Resume)
{
  const Transfer::Id id = "id";
  auto t = std::make_shared<Transfer>();
  t->id = id;
  t->state = Transfer::QUEUED;
  m_source->add_transfer(t);

  for (const auto& state : all_states)
    {
      t->state = state;
      EXPECT_CALL(*m_source, start(id)).Times(0);
      EXPECT_CALL(*m_source, pause(id)).Times(0);
      EXPECT_CALL(*m_source, resume(id)).Times(t->can_resume()?1:0);
      EXPECT_CALL(*m_source, cancel(id)).Times(0);
      EXPECT_CALL(*m_source, open(id)).Times(0);
      m_controller->resume(id);
    }
}

TEST_F(ControllerFixture, Cancel)
{
  const Transfer::Id id = "id";
  auto t = std::make_shared<Transfer>();
  t->id = id;
  t->state = Transfer::QUEUED;
  m_source->add_transfer(t);

  for (const auto& state : all_states)
    {
      t->state = state;
      EXPECT_CALL(*m_source, start(id)).Times(0);
      EXPECT_CALL(*m_source, pause(id)).Times(0);
      EXPECT_CALL(*m_source, resume(id)).Times(0);
      EXPECT_CALL(*m_source, cancel(id)).Times(t->can_cancel()?1:0);
      EXPECT_CALL(*m_source, open(id)).Times(0);
      m_controller->cancel(id);
    }
}

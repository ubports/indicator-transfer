/*
 * Copyright 2015 Canonical Ltd.
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

#include "source-mock.h"

#include <gtest/gtest.h>

#include <transfer/controller.h>
#include <transfer/multisource.h>

using ::testing::AtLeast;

using namespace unity::indicator::transfer;

class MultiSourceFixture: public ::testing::Test
{
protected:

  struct Event
  {
    typedef enum { ADDED, CHANGED, REMOVED } Type;
    Type type;
    Transfer::Id id;
    bool operator==(const Event& that) const { return type==that.type && id==that.id; }
  };

  bool model_consists_of(const std::shared_ptr<Model>& model, std::initializer_list<std::shared_ptr<Transfer>> list) const
  {
    // test get_all()
    std::vector<std::shared_ptr<Transfer>> transfers(list);
    std::sort(transfers.begin(), transfers.end());
    g_return_val_if_fail(transfers == model->get_all(), false);

    // test get()
    for(auto& transfer : transfers)
      g_return_val_if_fail(transfer == model->get(transfer->id), false);

    // test get_ids()
    std::set<Transfer::Id> ids;
    for(auto& transfer : transfers)
      ids.insert(transfer->id);
    g_return_val_if_fail(ids == model->get_ids(), false);

    return true;
  }
};

TEST_F(MultiSourceFixture,MultiplexesModels)
{
  // set up the tributary sources, 'a' and 'b'
  auto a = std::make_shared<MockSource>();
  auto b = std::make_shared<MockSource>();

  // set up the multisource and connect it to the tributaries
  MultiSource multisource;
  auto multimodel = multisource.get_model();
  std::vector<Event> events;
  std::vector<Event> expected_events;
  multisource.get_model()->added().connect([&events](const Transfer::Id& id){events.push_back(Event{Event::ADDED,id});});
  multisource.get_model()->changed().connect([&events](const Transfer::Id& id){events.push_back(Event{Event::CHANGED,id});});
  multisource.get_model()->removed().connect([&events](const Transfer::Id& id){events.push_back(Event{Event::REMOVED,id});});
  multisource.add_source(a);
  multisource.add_source(b);

  // add a transfer to the 'a' source...
  const Transfer::Id aid {"aid"};
  auto at = std::make_shared<Transfer>();
  at->id = aid;
  a->get_model()->add(at);
  expected_events.push_back(Event{Event::ADDED,aid});

  // confirm that the multimodel sees the new transfer
  EXPECT_EQ(expected_events, events);
  EXPECT_EQ(at, a->get_model()->get(aid));
  EXPECT_TRUE(model_consists_of(multimodel, {at}));

  // add a transfer to the 'b' source...
  const Transfer::Id bid {"bid"};
  auto bt = std::make_shared<Transfer>();
  bt->id = bid;
  b->get_model()->add(bt);
  expected_events.push_back(Event{Event::ADDED,bid});

  // confirm that the multimodel sees the new transfer
  EXPECT_EQ(expected_events, events);
  EXPECT_EQ(bt, b->get_model()->get(bid));
  EXPECT_TRUE(model_consists_of(multimodel, {at, bt}));

  // poke transfer 'at'...
  at->progress = 50.0;
  a->get_model()->emit_changed(aid);
  expected_events.push_back(Event{Event::CHANGED,aid});
  EXPECT_EQ(expected_events, events);
  EXPECT_TRUE(model_consists_of(multimodel, {at, bt}));

  // remove transfer 'at'...
  a->get_model()->remove(aid);
  expected_events.push_back(Event{Event::REMOVED,aid});
  EXPECT_EQ(expected_events, events);
  EXPECT_FALSE(a->get_model()->get(aid));
  EXPECT_TRUE(model_consists_of(multimodel, {bt}));

  // remove transfer 'bt'...
  b->get_model()->remove(bid);
  expected_events.push_back(Event{Event::REMOVED,bid});
  EXPECT_EQ(expected_events, events);
  EXPECT_FALSE(b->get_model()->get(aid));
  EXPECT_TRUE(model_consists_of(multimodel, {}));
}

TEST(Multisource,MethodDelegation)
{
  // set up the tributary sources, 'a' and 'b'
  auto a = std::make_shared<MockSource>();
  auto b = std::make_shared<MockSource>();

  // set up the multisource and connect it to the tributaries
  MultiSource multisource;
  multisource.add_source(a);
  multisource.add_source(b);

  // add a transfer to the 'a' source...
  const Transfer::Id aid {"aid"};
  auto at = std::make_shared<Transfer>();
  at->id = aid;
  a->get_model()->add(at);

  // add a transfer to the 'b' source...
  const Transfer::Id bid {"bid"};
  auto bt = std::make_shared<Transfer>();
  bt->id = bid;
  b->get_model()->add(bt);

  // confirm that multisource method calls are delegated to 'a'
  EXPECT_CALL(*a, open(aid)); multisource.open(aid);
  EXPECT_CALL(*a, start(aid)); multisource.start(aid);
  EXPECT_CALL(*a, pause(aid)); multisource.pause(aid);
  EXPECT_CALL(*a, resume(aid)); multisource.resume(aid);
  EXPECT_CALL(*a, cancel(aid)); multisource.cancel(aid);
  EXPECT_CALL(*a, open_app(aid)); multisource.open_app(aid);

  // confirm that multisource method calls are delegated to 'a'
  EXPECT_CALL(*b, open(bid)); multisource.open(bid);
  EXPECT_CALL(*b, start(bid)); multisource.start(bid);
  EXPECT_CALL(*b, pause(bid)); multisource.pause(bid);
  EXPECT_CALL(*b, resume(bid)); multisource.resume(bid);
  EXPECT_CALL(*b, cancel(bid)); multisource.cancel(bid);
  EXPECT_CALL(*b, open_app(bid)); multisource.open_app(bid);
}


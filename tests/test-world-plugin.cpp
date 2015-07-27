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

#include "glib-fixture.h"

#include <transfer/controller.h>
#include <transfer/world-plugin.h>


using namespace unity::indicator::transfer;

class PluginFixture: public GlibFixture
{
private:

  typedef GlibFixture super;

protected:

  GTestDBus* bus = nullptr;

  std::shared_ptr<MutableModel> m_model;
  std::shared_ptr<World> m_world;
  std::shared_ptr<Controller> m_controller;

  void SetUp()
  {
    super::SetUp();

    m_model.reset(new MutableModel);
    m_world.reset(new PluginWorld(m_model));
    m_controller.reset(new Controller(m_model, m_world));
  }

  void TearDown()
  {
    m_controller.reset();
    m_world.reset();
    m_model.reset();

    super::TearDown();
  }
};

TEST_F(PluginFixture, HelloWorld)
{
  // confirms that the Test DBus SetUp() and TearDown() works
}


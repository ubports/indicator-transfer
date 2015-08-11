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
#include <transfer/plugin-source.h>

using namespace unity::indicator::transfer;

class PluginFixture: public GlibFixture
{
private:

  typedef GlibFixture super;

protected:

  GTestDBus* bus = nullptr;

  std::shared_ptr<Source> m_source;
  std::shared_ptr<Controller> m_controller;

  void SetUp()
  {
    super::SetUp();

    auto plugin_dir = g_get_current_dir();
    m_source.reset(new PluginSource(plugin_dir));
    m_controller.reset(new Controller(m_source));
    g_clear_pointer(&plugin_dir, g_free);
  }

  void TearDown()
  {
    m_controller.reset();
    m_source.reset();

    super::TearDown();
  }
};

TEST_F(PluginFixture, MockSourcePluginLoads)
{
  // confirms that the fixture loads MockSourcePlugin
}


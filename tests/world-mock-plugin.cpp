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

#include "world-mock.h"

#include <transfer/model.h>
#include <transfer/world.h>

#include <glib.h>
#include <gmodule.h>

#include <memory>

using namespace unity::indicator::transfer;

extern "C"
{
G_MODULE_EXPORT std::shared_ptr<World> get_world(const std::shared_ptr<MutableModel>&);

G_MODULE_EXPORT std::shared_ptr<World> get_world(const std::shared_ptr<MutableModel>& /*model*/)
{
  return std::make_shared<MockWorld>();
}
}

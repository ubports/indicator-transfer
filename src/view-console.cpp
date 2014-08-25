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

#include <transfer/controller.h>
#include <transfer/model.h>
#include <transfer/view-console.h>

#include <iostream>

namespace unity {
namespace indicator {
namespace transfer {

/***
****
***/

ConsoleView::ConsoleView(const std::shared_ptr<Model>& model,
                         const std::shared_ptr<Controller>& controller)
{
  set_model(model);
  set_controller(controller);
}

ConsoleView::~ConsoleView()
{
}

void ConsoleView::set_controller(const std::shared_ptr<Controller>& controller)
{
  m_controller = controller;
}

static std::string dump_transfer(const std::shared_ptr<Transfer>& transfer)
{
  auto tmp = g_strdup_printf ("state [%d] id [%s] title[%s] app_icon[%s] time_started[%zu] seconds_left[%d] speed[%f KiB/s] progress[%f] error_string[%s] local_path[%s]",
                              (int)transfer->state,
                              transfer->id.c_str(),
                              transfer->title.c_str(),
                              transfer->app_icon.c_str(),
                              (size_t)transfer->time_started,
                              transfer->seconds_left,
                              transfer->speed_Bps/1024.0,
                              transfer->progress,
                              transfer->error_string.c_str(),
                              transfer->local_path.c_str());
  std::string ret = tmp;
  g_free(tmp);
  return ret;
}

void ConsoleView::set_model(const std::shared_ptr<Model>& model)
{
  m_connections.clear();

  if ((m_model = model))
    {
      m_connections.insert(m_model->added().connect([this](const Transfer::Id& id){
        std::cerr << "view added: " << dump_transfer(m_model->get(id)) << std::endl;
      }));

      m_connections.insert(m_model->changed().connect([this](const Transfer::Id& id){
        std::cerr << "view changed: " << dump_transfer(m_model->get(id)) << std::endl;
      }));

      m_connections.insert(m_model->removed().connect([this](const Transfer::Id& id){
        std::cerr << "view removing: " << dump_transfer(m_model->get(id)) << std::endl;
      }));
    }
}

/***
****
***/

} // namespace transfer
} // namespace indicator
} // namespace unity

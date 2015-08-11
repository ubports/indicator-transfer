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

namespace unity {
namespace indicator {
namespace transfer {

/***
****
***/

Controller::Controller(const std::shared_ptr<Source>& source):
  m_source(source)
{
}

Controller::~Controller()
{
}

void Controller::pause_all()
{
  for(const auto& id : get_ids())
    pause(id);
}

void Controller::resume_all()
{
  for(const auto& id : get_ids())
    resume(id);
}

void Controller::clear_all()
{
  for (const auto& id : get_ids())
    clear(id);
}

void Controller::tap(const Transfer::Id& id)
{
  const auto transfer = get(id);
  g_return_if_fail (transfer);

  if (transfer->can_start())
    start(id);
  else if (transfer->can_resume())
    resume(id);
  else if (transfer->can_pause())
    pause(id);
  else if (transfer->state == Transfer::FINISHED)
    open(id);
}


void Controller::pause(const Transfer::Id& id)
{
  const auto& transfer = get(id);
  if (transfer && transfer->can_pause())
    m_source->pause(id);
}

void Controller::cancel(const Transfer::Id& id)
{
  const auto& transfer = get(id);
  if (transfer && transfer->can_cancel())
    m_source->cancel(id);
}

void Controller::clear(const Transfer::Id& id)
{
  const auto& transfer = get(id);
  if (transfer && transfer->can_clear())
    m_source->clear(id);
}

void Controller::resume(const Transfer::Id& id)
{
  const auto& transfer = get(id);
  if (transfer && transfer->can_resume())
    m_source->resume(id);
}

void Controller::start(const Transfer::Id& id)
{
  const auto& transfer = get(id);
  if (transfer && transfer->can_start())
    m_source->start(id);
}

void Controller::open(const Transfer::Id& id)
{
  m_source->open(id);
}

void Controller::open_app(const Transfer::Id& id)
{
    m_source->open_app(id);
}

int Controller::size() const
{
    return m_source->get_model()->size();
}

int Controller::count(const Transfer::Id& id) const
{
    return m_source->get_model()->count(id);
}

std::set<Transfer::Id> Controller::get_ids() const
{
    return m_source->get_model()->get_ids();
}

std::shared_ptr<Transfer> Controller::get(const Transfer::Id& id) const
{
    return m_source->get_model()->get(id);
}

/***
****
***/

} // namespace transfer
} // namespace indicator
} // namespace unity

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

#include <transfer/model.h>

namespace unity {
namespace indicator {
namespace transfer {

Model::~Model()
{
}

std::set<Transfer::Id> Model::get_ids() const
{
  std::set<Transfer::Id> keys;

  for(const auto& it : m_transfers)
    keys.insert(it.first);

  return keys;
}

std::vector<std::shared_ptr<Transfer>> Model::get_all() const
{
  std::vector<std::shared_ptr<Transfer>> transfers;

  for(const auto& it : m_transfers)
    transfers.push_back(it.second);

  return transfers;
}

std::shared_ptr<Transfer> Model::get(const Transfer::Id& id) const
{
  std::shared_ptr<Transfer> ret;

  auto it = m_transfers.find(id);
  if (it != m_transfers.end())
    ret = it->second;

  return ret;
}

int Model::size() const
{
    return m_transfers.size();
}

int Model::count(const Transfer::Id& id) const
{
    return m_transfers.count(id);
}

const core::Signal<Transfer::Id>& Model::changed() const
{
  return m_changed;
}

const core::Signal<Transfer::Id>& Model::added() const
{
  return m_added;
}

const core::Signal<Transfer::Id>& Model::removed() const
{
  return m_removed;
}

/***
****
***/

MutableModel::~MutableModel()
{
}

void MutableModel::add(const std::shared_ptr<Transfer>& add_me)
{
  const auto& id = add_me->id;

  m_transfers[id] = add_me;
  m_added(id);
}

void MutableModel::remove(const Transfer::Id& id)
{
  auto it = m_transfers.find(id);
  g_return_if_fail (it != m_transfers.end());

  m_removed(id);
  m_transfers.erase(it);
}

void MutableModel::emit_changed(const Transfer::Id& id)
{
  m_changed(id);
}

/***
****
***/

} // namespace transfer
} // namespace indicator
} // namespace unity


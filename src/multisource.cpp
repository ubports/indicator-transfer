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

#include <transfer/multisource.h>

#include <set>
#include <string>
#include <vector>

namespace unity {
namespace indicator {
namespace transfer {

/***
****
***/

class MultiSource::Impl
{
public:

  Impl():
    m_model(std::make_shared<MutableModel>())
  {
  }

  std::shared_ptr<MutableModel> get_model()
  {
    return m_model;
  }

  void add_source(const std::shared_ptr<Source>& source)
  {
    g_return_if_fail(source);

    const auto idx = m_sources.size();
    m_sources.push_back(source);

    auto model = source->get_model();

    m_connections.insert(
      model->added().connect([this,idx](const Transfer::Id& id){
        auto s = m_sources[idx];
        m_id2source[id] = s;
        m_model->add(s->get_model()->get(id));
      })
    );

    m_connections.insert(
      model->removed().connect([this](const Transfer::Id& id){
        m_id2source.erase(id);
        m_model->remove(id);
      })
    );

    m_connections.insert(
      model->changed().connect([this](const Transfer::Id& id){
        m_model->emit_changed(id);
      })
    );
  }

  void start(const Transfer::Id& id)
  {
    auto source = lookup_source(id);
    g_return_if_fail(source);
    source->start(id);
  }

  void pause(const Transfer::Id& id)
  {
    auto source = lookup_source(id);
    g_return_if_fail(source);
    source->pause(id);
  }

  void resume(const Transfer::Id& id)
  {
    auto source = lookup_source(id);
    g_return_if_fail(source);
    source->resume(id);
  }

  void cancel(const Transfer::Id& id)
  {
    auto source = lookup_source(id);
    g_return_if_fail(source);
    source->cancel(id);
  }

  void clear(const Transfer::Id& id)
  {
    auto source = lookup_source(id);
    g_return_if_fail(source);
    source->clear(id);
  }

  void open(const Transfer::Id& id)
  {
    auto source = lookup_source(id);
    g_return_if_fail(source);
    source->open(id);
  }

  void open_app(const Transfer::Id& id)
  {
    auto source = lookup_source(id);
    g_return_if_fail(source);
    source->open_app(id);
  }

private:

  std::shared_ptr<Source> lookup_source(const Transfer::Id& id)
  {
    std::shared_ptr<Source> source;

    auto it = m_id2source.find(id);
    if (it != m_id2source.end())
      source = it->second;

    return source;
  }

  std::shared_ptr<MutableModel> m_model;
  std::vector<std::shared_ptr<Source>> m_sources;
  std::map<Transfer::Id,std::shared_ptr<Source>> m_id2source;
  std::set<core::ScopedConnection> m_connections;
};

/***
****
***/

MultiSource::MultiSource():
  impl(new Impl{})
{
}

MultiSource::~MultiSource()
{
}

void
MultiSource::open(const Transfer::Id& id)
{
  impl->open(id);
}

void
MultiSource::start(const Transfer::Id& id)
{
  impl->start(id);
}

void
MultiSource::pause(const Transfer::Id& id)
{
  impl->pause(id);
}

void
MultiSource::resume(const Transfer::Id& id)
{
  impl->resume(id);
}

void
MultiSource::cancel(const Transfer::Id& id)
{
  impl->cancel(id);
}

void MultiSource::clear(const Transfer::Id &id)
{
  impl->clear(id);
}

void
MultiSource::open_app(const Transfer::Id& id)
{
  impl->open_app(id);
}

const std::shared_ptr<const MutableModel>
MultiSource::get_model()
{
  return impl->get_model();
}

void
MultiSource::add_source(const std::shared_ptr<Source>& source)
{
  impl->add_source(source);
}

/***
****
***/

} // namespace transfer
} // namespace indicator
} // namespace unity

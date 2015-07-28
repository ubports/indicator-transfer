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

#include <transfer/source-plugin.h>

#include <gmodule.h>

namespace unity {
namespace indicator {
namespace transfer {

/***
****
***/

class PluginSource::Impl
{
public:

  explicit Impl(const std::shared_ptr<MutableModel>& model):
    m_model(model)
  {
    GError * error = nullptr;
    gchar * dirname = g_get_current_dir();
    g_message("plugin dir: %s", dirname);

    GDir * dir = g_dir_open(dirname, 0, &error);
    if (dir != nullptr)
    {
      const gchar * name;
      while ((name = g_dir_read_name(dir)))
      {
        if (g_str_has_suffix(name, ".so"))
        {
          gchar * filename = g_build_filename(dirname, name, nullptr);
          GModule * module = g_module_open(filename, (GModuleFlags)0);
          gpointer symbol {};
          if (module == nullptr)
          { 
            g_warning("Unable to load module '%s'", filename);
          }
          else if (!g_module_symbol(module, "get_source", &symbol))
          {
            g_warning("Unable to use module '%s'", filename);
          }
          else
          {
            using get_source_func = std::shared_ptr<Source>(const std::shared_ptr<MutableModel>&);
            auto model = std::make_shared<MutableModel>();
            auto source = reinterpret_cast<get_source_func*>(symbol)(model);
            if (source)
            {
              Sandbox sandbox { module, source, model };
              m_sandboxes.push_back(sandbox);
              g_debug("Loaded plugin '%s'", filename);

              // FIXME: gotta listen to signals from source, mux the ids, and re-emit them
            }
          }

          g_clear_pointer(&filename, g_free);
        }
      }

      g_clear_pointer(&dir, g_dir_close);
    }

    g_clear_pointer(&dirname, g_free);
  }

  ~Impl()
  {
    // FIXME: need to close / free the modules & clean out the sandboxes.
    // better yet, wrap them in smart pointers so they destruct automatically
  }

  void start(const Transfer::Id& id)
  {
    auto demux = lookup(id);
    if (demux.first)
        demux.first->start(id);
  }

  void pause(const Transfer::Id& id)
  {
    auto demux = lookup(id);
    if (demux.first)
        demux.first->pause(id);
  }

  void resume(const Transfer::Id& id)
  {
    auto demux = lookup(id);
    if (demux.first)
        demux.first->resume(id);
  }

  void cancel(const Transfer::Id& id)
  {
    auto demux = lookup(id);
    if (demux.first)
        demux.first->cancel(id);
  }

  void open(const Transfer::Id& id)
  {
    auto demux = lookup(id);
    if (demux.first)
        demux.first->open(id);
  }

  void open_app(const Transfer::Id& id)
  {
    auto demux = lookup(id);
    if (demux.first)
        demux.first->open_app(id);
  }

private:

  std::pair<std::shared_ptr<Source>,Transfer::Id> lookup(const Transfer::Id& id)
  {
    std::shared_ptr<Source> source;
    Transfer::Id demuxed_id;
    
    auto tokens = g_strsplit(id.c_str(), ":", 2);
    if (g_strv_length(tokens) == 2)
    {
        unsigned int i = atoi(tokens[0]);
        if (i<m_sandboxes.size())
        {
            source = m_sandboxes[i].source;
            demuxed_id = tokens[1];
        }
    }
    g_strfreev(tokens);

    return std::make_pair(source, demuxed_id);
  }

  std::shared_ptr<MutableModel> m_model;

  struct Sandbox
  {
    GModule * module;
    std::shared_ptr<Source> source;
    std::shared_ptr<MutableModel> model;
  };

  std::vector<Sandbox> m_sandboxes;
};

/***
****
***/

PluginSource::PluginSource(const std::shared_ptr<MutableModel>& model):
  impl(new Impl(model))
{
}

PluginSource::~PluginSource()
{
}

void
PluginSource::open(const Transfer::Id& id)
{
  impl->open(id);
}

void
PluginSource::start(const Transfer::Id& id)
{
  impl->start(id);
}

void
PluginSource::pause(const Transfer::Id& id)
{
  impl->pause(id);
}

void
PluginSource::resume(const Transfer::Id& id)
{
  impl->resume(id);
}

void
PluginSource::cancel(const Transfer::Id& id)
{
  impl->cancel(id);
}

void
PluginSource::open_app(const Transfer::Id& id)
{
  impl->open_app(id);
}

/***
****
***/

} // namespace transfer
} // namespace indicator
} // namespace unity

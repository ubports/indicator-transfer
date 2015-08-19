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

#include <transfer/plugin-source.h>

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

  Impl(PluginSource& owner, const std::string& plugin_dir):
    m_owner(owner)
  {
    GError * error = nullptr;
    g_debug("plugin_dir '%s'", plugin_dir.c_str());
    GDir * dir = g_dir_open(plugin_dir.c_str(), 0, &error);
    if (dir != nullptr)
    {
      const gchar * name;
      while ((name = g_dir_read_name(dir)))
      {
        if (g_str_has_suffix(name, G_MODULE_SUFFIX))
        {
          gchar * filename = g_build_filename(plugin_dir.c_str(), name, nullptr);
          GModule * mod = g_module_open(filename, G_MODULE_BIND_LOCAL);
          gpointer symbol {};

          if (mod == nullptr)
          { 
            g_warning("Unable to load module '%s'", filename);
          }
          else if (!g_module_symbol(mod, "get_source", &symbol))
          {
            g_warning("Unable to use module '%s'", filename);
          }
          else
          {
            using get_source_func = Source*();
            auto src = reinterpret_cast<get_source_func*>(symbol)();
            if (src)
            {
              auto deleter = [mod,src](Source *s){
                delete s;
                g_module_close(mod);
              };
              m_owner.add_source(std::shared_ptr<Source>(src, deleter));
              g_debug("Loaded plugin '%s'", filename);
              mod = nullptr;
            }
          }
          g_clear_pointer(&mod, g_module_close);
          g_clear_pointer(&filename, g_free);
        }
      }
      g_clear_pointer(&dir, g_dir_close);
    }
  }

private:
  PluginSource& m_owner;

};

/***
****
***/

PluginSource::PluginSource(const std::string& plugin_dir):
  impl(new Impl(*this, plugin_dir))
{
}

PluginSource::~PluginSource()
{
}

/***
****
***/

} // namespace transfer
} // namespace indicator
} // namespace unity

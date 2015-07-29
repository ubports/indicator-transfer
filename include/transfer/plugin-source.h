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

#ifndef INDICATOR_TRANSFER_SOURCE_PLUGIN_H
#define INDICATOR_TRANSFER_SOURCE_PLUGIN_H

#include <transfer/multisource.h>

#include <memory> // unique_ptr

namespace unity {
namespace indicator {
namespace transfer {

/**
 * \brief a MultiSource that gets its sources from plugins
 */
class PluginSource: public MultiSource
{
public:
    explicit PluginSource(const std::string& plugin_dir);
    ~PluginSource();

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_SOURCE_PLUGINSH

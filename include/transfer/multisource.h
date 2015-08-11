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

#ifndef INDICATOR_TRANSFER_MULTISOURCE_H
#define INDICATOR_TRANSFER_MULTISOURCE_H

#include <transfer/source.h>

namespace unity {
namespace indicator {
namespace transfer {

/**
 * \brief A multiplexer/demultiplexer for sources
 */
class MultiSource: public Source
{
public:
    MultiSource();
    virtual ~MultiSource();

    // Source
    void open(const Transfer::Id& id) override;
    void start(const Transfer::Id& id) override;
    void pause(const Transfer::Id& id) override;
    void resume(const Transfer::Id& id) override;
    void cancel(const Transfer::Id& id) override;
    void clear(const Transfer::Id& id) override;
    void open_app(const Transfer::Id& id) override;
    std::shared_ptr<MutableModel> get_model() override;

    void add_source(const std::shared_ptr<Source>& source);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_MULTISOURCE_H

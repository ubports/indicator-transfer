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

#ifndef INDICATOR_TRANSFER_MODEL_H
#define INDICATOR_TRANSFER_MODEL_H

#include <transfer/transfer.h>

#include <core/signal.h>

#include <map>
#include <memory> // std::shared_ptr

namespace unity {
namespace indicator {
namespace transfer {

/**
 * \brief A model of all the Transfers that we know about
 */
class Model
{
public:
    virtual ~Model();

    std::set<Transfer::Id> get_ids() const;
    std::vector<std::shared_ptr<Transfer>> get_all() const;
    std::shared_ptr<Transfer> get(const Transfer::Id&) const;

    const core::Signal<Transfer::Id>& changed() const;
    const core::Signal<Transfer::Id>& added() const;
    const core::Signal<Transfer::Id>& removed() const;

protected:
    std::map<Transfer::Id,std::shared_ptr<Transfer>> m_transfers;
    core::Signal<Transfer::Id> m_changed;
    core::Signal<Transfer::Id> m_added;
    core::Signal<Transfer::Id> m_removed;
};

/**
 * \brief A Transfer Model that has a public API for changing its contents
 */
class MutableModel: public Model
{
public:
    ~MutableModel();
    void add(const std::shared_ptr<Transfer>& add_me);
    void remove(const Transfer::Id&);
    void emit_changed(const Transfer::Id&);
};


} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_MODEL_H

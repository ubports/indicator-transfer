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

#include <transfer/transfer-source-mock.h>

#include <algorithm>

namespace unity {
namespace indicator {
namespace transfer {

/***
****
***/

MockTransferSource::MockTransferSource(const std::shared_ptr<Transfers>& transfers):
    TransferSource(transfers)
{
}

MockTransferSource::~MockTransferSource()
{
}

void
MockTransferSource::add(const std::shared_ptr<Transfer>& transfer)
{
    g_message("%s adding '%s'", G_STRFUNC, transfer->id().c_str());
    auto tmp = m_transfers->get();
    g_message("old size is %zu", (size_t)tmp.size());
    tmp.push_back(transfer);
    m_transfers->set(tmp);
    g_message("new size is %zu", (size_t)tmp.size());
}

void
MockTransferSource::remove(const Transfer::Id& id)
{
    g_message("%s removing '%s'", G_STRFUNC, id.c_str());
    auto tmp = m_transfers->get();
    g_message("old size is %zu", (size_t)tmp.size());
    auto new_end = remove_if (tmp.begin(), tmp.end(), [id](const std::shared_ptr<Transfer>& t){return t->id()==id;});
    tmp.erase(new_end, tmp.end());
    m_transfers->set(tmp);
    g_message("new size is %zu", (size_t)tmp.size());
}

/***
****
***/

} // namespace transfer
} // namespace indicator
} // namespace unity

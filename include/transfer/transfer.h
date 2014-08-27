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

#ifndef INDICATOR_TRANSFER_TRANSFER_H
#define INDICATOR_TRANSFER_TRANSFER_H

#include <core/property.h> // parent class

#include <gio/gio.h> // GIcon

#include <ctime> // time_t
#include <memory>
#include <string>
#include <vector>

namespace unity {
namespace indicator {
namespace transfer {

/**
 * \brief A simple struct representing a single Transfer
 */
struct Transfer
{
  typedef enum { QUEUED, RUNNING, PAUSED, CANCELED,
                 HASHING, PROCESSING, FINISHED,
                 ERROR } State;
  State state = QUEUED;
  bool can_start() const;
  bool can_resume() const;
  bool can_pause() const;
  bool can_cancel() const;
  bool can_clear() const;

  // -1 == unknown
  int seconds_left = -1;

  time_t time_started = 0;

  // [0...1]
  float progress = 0.0;

  // bytes per second
  uint64_t speed_Bps = 0;

  uint64_t total_size = 0;

  typedef std::string Id;
  Id id;
  std::string title;
  std::string app_icon;

  // meaningful iff state is ERROR
  std::string error_string;

  // meaningful iff state is FINISHED
  std::string local_path;

protected:
  static std::string next_unique_id();
};
    
} // namespace transfer
} // namespace indicator
} // namespace unity

#endif // INDICATOR_TRANSFER_TRANSFER_H

/** collector

A full notice with attributions is provided along with this source code.

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 2 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

* In addition, as a special exception, the copyright holders give
* permission to link the code of portions of this program with the
* OpenSSL library under certain conditions as described in each
* individual source file, and distribute linked combinations
* including the two.
* You must obey the GNU General Public License in all respects
* for all of the code used other than OpenSSL.  If you modify
* file(s) with this exception, you may extend this exception to your
* version of the file(s), but you are not obligated to do so.  If you
* do not wish to do so, delete this exception statement from your
* version.
*/

#ifndef COLLECTOR_CONNTRACKER_H
#define COLLECTOR_CONNTRACKER_H

#include <mutex>
#include <vector>

#include "Containers.h"
#include "Hash.h"
#include "NRadix.h"
#include "NetworkConnection.h"

namespace collector {

// ConnStatus encapsulates the status of a connection, comprised of the timestamp when the connection was last seen
// alive (in microseconds since epoch), and a flag indicating whether the connection is currently active.
class ConnStatus {
 private:
  static constexpr uint64_t kActiveFlag = 1UL << 63;

  static inline uint64_t MakeActive(uint64_t data, bool active) {
    return active ? (data | kActiveFlag) : (data & ~kActiveFlag);
  }

 public:
  ConnStatus() : data_(0UL) {}
  ConnStatus(int64_t microtimestamp, bool active) : data_(MakeActive(static_cast<uint64_t>(microtimestamp), active)) {}

  int64_t LastActiveTime() const { return static_cast<int64_t>(data_ & ~kActiveFlag); }
  bool IsActive() const { return (data_ & kActiveFlag) != 0; }

  void SetActive(bool active) {
    data_ = MakeActive(data_, active);
  }

  void MergeFrom(const ConnStatus& other) {
    data_ = std::max(data_, other.data_);
  }

  ConnStatus WithStatus(bool active) const {
    return ConnStatus(MakeActive(data_, active));
  }

  bool operator==(const ConnStatus& other) const {
    return data_ == other.data_;
  }

  bool operator!=(const ConnStatus& other) const {
    return !(*this == other);
  }

 private:
  explicit ConnStatus(uint64_t data) : data_(data) {}

  uint64_t data_;
};

using ConnMap = UnorderedMap<Connection, ConnStatus>;
using ContainerEndpointMap = UnorderedMap<ContainerEndpoint, ConnStatus>;

class CollectorStats;

class ConnectionTracker {
 public:
  void UpdateConnection(const Connection& conn, int64_t timestamp, bool added);
  void AddConnection(const Connection& conn, int64_t timestamp) {
    UpdateConnection(conn, timestamp, true);
  }
  void RemoveConnection(const Connection& conn, int64_t timestamp) {
    UpdateConnection(conn, timestamp, false);
  }

  void Update(const std::vector<Connection>& all_conns, const std::vector<ContainerEndpoint>& all_listen_endpoints, int64_t timestamp);

  // Atomically fetch a snapshot of the current state, removing all inactive connections if requested.
  ConnMap FetchConnState(bool normalize = false, bool clear_inactive = true);
  ContainerEndpointMap FetchEndpointState(bool normalize = false, bool clear_inactive = true);

  template <typename T>
  static void UpdateOldState(UnorderedMap<T, ConnStatus>* old_state, const UnorderedMap<T, ConnStatus>& new_state, int64_t now, int64_t afterglow_period_micros);
  static bool WasRecentlyActive(const ConnStatus& conn, int64_t now, int64_t afterglow_period_micros);
  static bool IsInAfterglowPeriod(const ConnStatus& conn, int64_t now, int64_t afterglow_period_micros);
  template <typename T>
  // ComputeDelta computes a diff between new_state and old_state
  static void ComputeDelta(const UnorderedMap<T, ConnStatus>& new_state, const UnorderedMap<T, ConnStatus>& old_state, UnorderedMap<T, ConnStatus>& delta, int64_t now, int64_t old_now, int64_t afterglow_period_micros);

  void UpdateKnownPublicIPs(UnorderedSet<Address>&& known_public_ips);
  void UpdateKnownIPNetworks(UnorderedMap<Address::Family, std::vector<IPNet>>&& known_ip_networks);
  void UpdateIgnoredL4ProtoPortPairs(UnorderedSet<L4ProtoPortPair>&& ignored_l4proto_port_pairs);

 private:
  // NormalizeConnection transforms a connection into a normalized form.
  Connection NormalizeConnectionNoLock(const Connection& conn) const;

  // Emplace a connection into the state ConnMap, or update its timestamp if the supplied timestamp is more recent
  // than the stored one.
  void EmplaceOrUpdateNoLock(const Connection& conn, ConnStatus status);

  // Emplace a listen endpoint into the state ContainerEndpointMap, or update its timestamp if the supplied timestamp is more
  // recent than the stored one.
  void EmplaceOrUpdateNoLock(const ContainerEndpoint& ep, ConnStatus status);

  IPNet NormalizeAddressNoLock(const Address& address) const;

  // Returns true if any connection filters are found.
  inline bool HasConnectionStateFilters() const {
    return !ignored_l4proto_port_pairs_.empty();
  }

  // Determine if a protocol port combination from a connection or endpoint should be ignored
  inline bool IsIgnoredL4ProtoPortPair(const L4ProtoPortPair& p) const {
    return Contains(ignored_l4proto_port_pairs_, p);
  }

  // NormalizeContainerEndpoint transforms a container endpoint into a normalized form.
  inline ContainerEndpoint NormalizeContainerEndpoint(const ContainerEndpoint& cep) const {
    const auto& ep = cep.endpoint();
    return ContainerEndpoint(cep.container(), Endpoint(Address(ep.address().family()), ep.port()), cep.l4proto());
  }

  // Determine if a connection should be ignored
  inline bool ShouldFetchConnection(const Connection& conn) const {
    return !IsIgnoredL4ProtoPortPair(L4ProtoPortPair(conn.l4proto(), conn.local().port())) &&
           !IsIgnoredL4ProtoPortPair(L4ProtoPortPair(conn.l4proto(), conn.remote().port()));
  }

  // Determine if a container endpoint should be ignored
  inline bool ShouldFetchContainerEndpoint(const ContainerEndpoint& cep) const {
    return !IsIgnoredL4ProtoPortPair(L4ProtoPortPair(cep.l4proto(), cep.endpoint().port()));
  }

  std::mutex mutex_;
  ConnMap conn_state_;
  ContainerEndpointMap endpoint_state_;

  UnorderedSet<Address> known_public_ips_;
  NRadixTree known_ip_networks_;
  UnorderedMap<Address::Family, bool> known_private_networks_exists_;
  UnorderedSet<L4ProtoPortPair> ignored_l4proto_port_pairs_;
};

/* static */
template <typename T>
void ConnectionTracker::UpdateOldState(UnorderedMap<T, ConnStatus>* old_state, const UnorderedMap<T, ConnStatus>& new_state, int64_t now, int64_t afterglow_period_micros) {
  //Remove inactive connections that are older than the afterglow period and add unexpired new connections to the old state
  for (auto it = old_state->begin(); it != old_state->end();) {
    auto& old_conn = *it;
    if (WasRecentlyActive(old_conn.second, now, afterglow_period_micros)) {
      ++it;
    } else {
      it = old_state->erase(it);
    }
  }
  for (const auto& conn : new_state) {
    auto insert_res = old_state->insert(conn);
    if (!insert_res.second) {
      auto& old_conn = *insert_res.first;
      old_conn.second = conn.second;
    }
  }
}

template <typename T>
void ConnectionTracker::ComputeDelta(const UnorderedMap<T, ConnStatus>& new_state, const UnorderedMap<T, ConnStatus>& old_state, UnorderedMap<T, ConnStatus>& delta, int64_t now, int64_t old_now, int64_t afterglow_period_micros) {
  // Insert all objects from the new state, if anything changed about them.
  for (const auto& conn : new_state) {
    auto old_conn = old_state.find(conn.first);
    if (old_conn != old_state.end()) {
      bool oldRecentlyActive = WasRecentlyActive(old_conn->second, old_now, afterglow_period_micros);  //Connections active within the afterglow period are considered to be active for the purpose of the delta.
      bool newRecentlyActive = WasRecentlyActive(conn.second, now, afterglow_period_micros);
      if (newRecentlyActive != oldRecentlyActive) {
        // Object was either resurrected or newly closed. Update in either case.
        delta.insert(conn);
      } else if (!newRecentlyActive) {
        // Both objects are inactive. Update the timestamp if applicable, otherwise omit from delta.
        if (old_conn->second.LastActiveTime() < conn.second.LastActiveTime()) {
          delta.insert(conn);
        }
      }
    } else {
      delta.insert(conn);
    }
  }

  //Add everything in the old state that was in the active state and is not in the new state
  for (auto conn : old_state) {
    if (new_state.find(conn.first) == new_state.end() && conn.second.IsActive() && !IsInAfterglowPeriod(conn.second, now, afterglow_period_micros)) {
      conn.second.SetActive(false);
      delta.insert(conn);
    }
  }
}

}  // namespace collector

#endif  //COLLECTOR_CONNTRACKER_H

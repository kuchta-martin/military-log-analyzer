#include "visitor_log.hpp"

#include <algorithm>
#include <cstddef>

// ── Constructor ─────────────────────────────────────────────────────────────

CVisitorLog::CVisitorLog(std::vector<LogEntry>             entries,
                         std::vector<std::vector<int>>      dist,
                         std::map<std::string, std::size_t> zoneIndex)
    : m_dist(std::move(dist))
    , m_zoneIndex(std::move(zoneIndex))
{
    for (auto& e : entries)
        m_byPerson[e.m_name].push_back(std::move(e));

    for (auto& [name, visits] : m_byPerson)
        std::sort(visits.begin(), visits.end(),
                  [](const LogEntry& a, const LogEntry& b) {
                      return a.m_time < b.m_time;
                  });
}

// ── search ──────────────────────────────────────────────────────────────────

std::set<std::string> CVisitorLog::search(const CAuditFilter& filter) const {
    std::set<std::string> result;

    const auto zoneIt = m_zoneIndex.find(filter.m_zoneName);
    if (zoneIt == m_zoneIndex.end()) return result; // unknown zone → no suspects
    const std::size_t targetIdx = zoneIt->second;

    for (const auto& [name, visits] : m_byPerson) {
        // Log entries are paired: enter[i], exit[i+1], enter[i+2], …
        for (size_t i = 0; i < visits.size(); i += 2) {
            const LogEntry& entryEv = visits[i];

            const int distIn = m_dist[entryEv.m_zoneIdx][targetIdx];
            if (distIn == -1) continue; // target unreachable from entry zone

            const long long earliest = entryEv.m_time + distIn;

            bool isSuspect = true;

            if (i + 1 < visits.size()) { // paired exit record exists
                const int distOut = m_dist[visits[i + 1].m_zoneIdx][targetIdx];
                if (distOut == -1) continue; // target unreachable from exit zone

                const long long latest = visits[i + 1].m_time - distOut;
                if (latest < earliest) continue; // impossible time window

                if (filter.m_from != -1 && latest  < filter.m_from) isSuspect = false;
            }

            if (filter.m_to != -1 && earliest > filter.m_to) isSuspect = false;
            if (isSuspect) result.insert(name);
        }
    }
    return result;
}

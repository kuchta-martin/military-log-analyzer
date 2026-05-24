#pragma once

#include "audit_filter.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

class CMilBase; // forward declaration – only needed for friend

/**
 * An immutable snapshot of processed log data.
 *
 * Instances are produced exclusively by CMilBase::processLog().
 * Use search() to query which people could have visited a given zone.
 */
class CVisitorLog {
    friend class CMilBase;

    std::map<std::string, std::vector<LogEntry>> m_byPerson; ///< All visits, sorted by time per person
    std::vector<std::vector<int>>                m_dist;     ///< BFS all-pairs distance matrix
    std::map<std::string, std::size_t>           m_zoneIndex;///< Zone name → matrix column/row index

    /// Private – only CMilBase may construct a CVisitorLog.
    CVisitorLog(std::vector<LogEntry>             entries,
                std::vector<std::vector<int>>      dist,
                std::map<std::string, std::size_t> zoneIndex);

public:
    /**
     * Returns the set of people who could have visited the zone described by @p filter
     * within the optional time window.
     *
     * A person is a suspect if there exists at least one entry/exit pair such that
     * they had enough time to travel from their entry zone to the target zone (and back
     * before exiting), and that window overlaps the requested interval.
     */
    std::set<std::string> search(const CAuditFilter& filter) const;
};

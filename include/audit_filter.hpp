#pragma once

#include "date_utils.hpp"

#include <string>
#include <utility>

class CVisitorLog; // forward declaration – only needed for friend

/**
 * Fluent builder that describes what zone to audit and an optional time window.
 *
 * Example:
 *   CAuditFilter("headquarters")
 *       .notBefore(2026, 1, 1, 0, 0)
 *       .notAfter (2026, 12, 31, 23, 59)
 */
class CAuditFilter {
    friend class CVisitorLog;

    std::string m_zoneName;
    long long   m_from = -1; ///< -1 = no lower bound
    long long   m_to   = -1; ///< -1 = no upper bound

public:
    explicit CAuditFilter(std::string zoneName)
        : m_zoneName(std::move(zoneName)) {}

    /// Restrict results to visits that could have occurred on or after this timestamp.
    CAuditFilter& notBefore(int y, int m, int d, int h, int mi) {
        m_from = toMinutes(y, m, d, h, mi);
        return *this;
    }

    /// Restrict results to visits that could have occurred on or before this timestamp.
    CAuditFilter& notAfter(int y, int m, int d, int h, int mi) {
        m_to = toMinutes(y, m, d, h, mi);
        return *this;
    }
};

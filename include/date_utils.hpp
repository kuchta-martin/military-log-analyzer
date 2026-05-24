#pragma once

#include <cstddef>
#include <string>

/// Returns true if @p y is a leap year.
inline bool isLeapYear(int y) noexcept {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

/// Returns the number of days in month @p m of year @p y.
inline int daysInMonth(int y, int m) noexcept {
    static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && isLeapYear(y)) return 29;
    return days[m];
}

/// Converts a date-time to an absolute minute count (enables fast integer comparisons).
inline long long toMinutes(int y, int m, int d, int h, int mi) noexcept {
    long long totalDays = static_cast<long long>(y) * 365 + y / 4 - y / 100 + y / 400;
    for (int i = 1; i < m; ++i)
        totalDays += daysInMonth(y, i);
    totalDays += d - 1;
    return totalDays * 24LL * 60 + h * 60 + mi;
}

/// A single access-log record: who entered/left which zone and when.
struct LogEntry {
    std::string m_name;    ///< Person's full name
    long long   m_time;    ///< Absolute minute timestamp (via toMinutes)
    std::size_t m_zoneIdx; ///< Index into the zone distance table
};

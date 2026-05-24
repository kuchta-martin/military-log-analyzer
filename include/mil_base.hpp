#pragma once

#include "visitor_log.hpp"

#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/**
 * Represents a military base: its zone-connectivity graph and log-file parser.
 *
 * Typical usage:
 *   CMilBase base;
 *   base.readBase("base.txt");
 *   CVisitorLog log = base.processLog("in1.log");
 *   auto suspects = log.search(CAuditFilter("vault").notBefore(2026,3,10,0,0));
 */
class CMilBase {
    std::map<std::string, std::size_t>                    m_zoneIndex; ///< Zone name → vertex index
    std::vector<std::vector<std::pair<std::size_t, int>>> m_graph;     ///< Adjacency list (neighbour idx, weight)
    std::vector<std::vector<int>>                 m_dist;      ///< All-pairs shortest-path matrix (-1 = unreachable)

    // ── Binary I/O helpers (defined in mil_base.cpp) ────────────────────────
    static unsigned short readU16LE(std::ifstream& f);
    static unsigned int   readU32LE(std::ifstream& f);
    static unsigned short readU16BE(std::ifstream& f);
    static unsigned int   readU32BE(std::ifstream& f);

    // ── Graph helpers (defined in mil_base.cpp) ─────────────────────────────
    std::size_t getOrAddZone(const std::string& name);
    void computeDistances();

    // ── Date helpers (defined in mil_base.cpp) ──────────────────────────────
    struct DateTime { int y, m, d, h, mi; };
    static DateTime unpackDT(unsigned int packed);
    static bool     isValidDate(const DateTime& dt);

    /**
     * Parses one binary block (LE or BE).
     *
     * Kept as a template so that the same parsing logic handles both byte orders:
     * the caller supplies the appropriate readU16 / readU32 overload.
     * Must live in the header because the compiler needs the full definition
     * to instantiate the template.
     */
    template <typename ReadU16, typename ReadU32>
    void processBinaryBlock(std::ifstream& file,
                            ReadU16 readU16, ReadU32 readU32,
                            std::vector<LogEntry>& entries) {
        const unsigned int zoneLen = readU16(file);
        std::string zoneName(zoneLen, '\0');
        if (!file.read(&zoneName[0], static_cast<std::streamsize>(zoneLen)))
            throw std::runtime_error("Read error");

        const unsigned int count = readU32(file);
        for (unsigned int i = 0; i < count; ++i) {
            const DateTime dt = unpackDT(readU32(file));

            const unsigned int nameLen = readU16(file);
            std::string name(nameLen, '\0');
            if (!file.read(&name[0], static_cast<std::streamsize>(nameLen)))
                throw std::runtime_error("Read error");

            if (!isValidDate(dt))
                throw std::runtime_error("Invalid date in binary block");

            entries.push_back({name,
                               toMinutes(dt.y, dt.m, dt.d, dt.h, dt.mi),
                               getOrAddZone(zoneName)});
        }
    }

public:
    CMilBase() = default;

    /// Reads the base topology (undirected weighted zone graph) from a text file.
    /// Each line: <zoneA> <zoneB> <minutes>
    void readBase(const std::string& baseFilename);

    /// Parses a log file containing TEXT / LE-binary / BE-binary blocks.
    /// Returns a queryable CVisitorLog snapshot.
    CVisitorLog processLog(const std::string& logFilename);
};

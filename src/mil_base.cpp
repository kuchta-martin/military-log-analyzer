#include "mil_base.hpp"

#include <queue>
#include <sstream>

// ── Binary I/O helpers ──────────────────────────────────────────────────────

unsigned short CMilBase::readU16LE(std::ifstream& f) {
    unsigned char b[2];
    if (!f.read(reinterpret_cast<char*>(b), 2))
        throw std::runtime_error("Read error");
    return static_cast<unsigned short>(b[0] | (b[1] << 8));
}

unsigned int CMilBase::readU32LE(std::ifstream& f) {
    unsigned char b[4];
    if (!f.read(reinterpret_cast<char*>(b), 4))
        throw std::runtime_error("Read error");
    return static_cast<unsigned int>(b[0])
         | (static_cast<unsigned int>(b[1]) <<  8)
         | (static_cast<unsigned int>(b[2]) << 16)
         | (static_cast<unsigned int>(b[3]) << 24);
}

unsigned short CMilBase::readU16BE(std::ifstream& f) {
    unsigned char b[2];
    if (!f.read(reinterpret_cast<char*>(b), 2))
        throw std::runtime_error("Read error");
    return static_cast<unsigned short>((b[0] << 8) | b[1]);
}

unsigned int CMilBase::readU32BE(std::ifstream& f) {
    unsigned char b[4];
    if (!f.read(reinterpret_cast<char*>(b), 4))
        throw std::runtime_error("Read error");
    return (static_cast<unsigned int>(b[0]) << 24)
         | (static_cast<unsigned int>(b[1]) << 16)
         | (static_cast<unsigned int>(b[2]) <<  8)
         |  static_cast<unsigned int>(b[3]);
}

// ── Zone / graph helpers ────────────────────────────────────────────────────

std::size_t CMilBase::getOrAddZone(const std::string& name) {
    const auto it = m_zoneIndex.find(name);
    if (it != m_zoneIndex.end()) return it->second;

    const std::size_t idx = m_graph.size();
    m_zoneIndex[name] = idx;
    m_graph.emplace_back();
    for (auto& row : m_dist) row.push_back(-1); // extend all existing rows
    m_dist.emplace_back(idx + 1, -1);           // new row, length = idx+1
    m_dist[idx][idx] = 0;
    return idx;
}

void CMilBase::computeDistances() {
    const size_t n = m_graph.size();
    m_dist.assign(n, std::vector<int>(n, -1));

    for (size_t start = 0; start < n; ++start) {
        std::queue<size_t> q;
        q.push(start);
        m_dist[start][start] = 0;

        while (!q.empty()) {
            const size_t cur = q.front(); q.pop();
            for (const auto& [next, weight] : m_graph[cur]) {
                if (m_dist[start][next] == -1) {
                    m_dist[start][next] = m_dist[start][cur] + weight;
                    q.push(static_cast<size_t>(next));
                }
            }
        }
    }
}

// ── Date helpers ────────────────────────────────────────────────────────────

CMilBase::DateTime CMilBase::unpackDT(unsigned int dt) {
    // Bit layout: YYYYYYYYYYYYMMMMDDDDDHHHHHIIIIII
    return {
        static_cast<int>((dt >> 20) & 0xFFF),
        static_cast<int>((dt >> 16) & 0x0F),
        static_cast<int>((dt >> 11) & 0x1F),
        static_cast<int>((dt >>  6) & 0x1F),
        static_cast<int>( dt        & 0x3F)
    };
}

bool CMilBase::isValidDate(const DateTime& dt) {
    if (dt.y < 1)                                    return false;
    if (dt.m < 1 || dt.m > 12)                       return false;
    if (dt.d < 1 || dt.d > daysInMonth(dt.y, dt.m)) return false;
    if (dt.h  < 0 || dt.h  > 23)                     return false;
    if (dt.mi < 0 || dt.mi > 59)                     return false;
    return true;
}

// ── Public interface ────────────────────────────────────────────────────────

void CMilBase::readBase(const std::string& baseFilename) {
    std::ifstream file(baseFilename);
    if (!file.is_open())
        throw std::runtime_error("Cannot open " + baseFilename);

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string zoneA, zoneB;
        int weight = 0;
        if (!(ss >> zoneA >> zoneB >> weight) || weight <= 0)
            throw std::runtime_error("Invalid base format in " + baseFilename);

        const std::size_t idxA = getOrAddZone(zoneA);
        const std::size_t idxB = getOrAddZone(zoneB);
        m_graph[idxA].emplace_back(idxB, weight);
        m_graph[idxB].emplace_back(idxA, weight);
    }
    if (file.bad())
        throw std::runtime_error("Read error in " + baseFilename);

    computeDistances();
}

CVisitorLog CMilBase::processLog(const std::string& logFilename) {
    std::ifstream file(logFilename, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Cannot open " + logFilename);

    std::vector<LogEntry> entries;
    char magic[4];

    while (file.read(magic, 4)) {
        if (magic[0] == 'T' && magic[1] == 'E' && magic[2] == 'X' && magic[3] == 'T') {
            // ── TEXT block ──────────────────────────────────────────────────
            std::string header;
            if (!std::getline(file, header))
                throw std::runtime_error("Read error in " + logFilename);

            std::istringstream hss(header);
            std::string zoneName;
            int count = 0;
            if (!(hss >> zoneName >> count) || count < 0)
                throw std::runtime_error("Invalid TEXT header in " + logFilename);

            for (int i = 0; i < count; ++i) {
                std::string recLine;
                if (!std::getline(file, recLine))
                    throw std::runtime_error("Read error in " + logFilename);

                std::istringstream rec(recLine);
                DateTime dt{};
                char dash1, dash2, colon;
                if (!(rec >> dt.y >> dash1 >> dt.m >> dash2 >> dt.d
                          >> dt.h >> colon >> dt.mi)
                    || dash1 != '-' || dash2 != '-' || colon != ':')
                    throw std::runtime_error("Invalid date format in " + logFilename);

                std::string name;
                std::getline(rec, name);
                if (!name.empty() && name.front() == ' ')
                    name.erase(name.begin());

                if (!isValidDate(dt))
                    throw std::runtime_error("Invalid date in " + logFilename);

                entries.push_back({name,
                                   toMinutes(dt.y, dt.m, dt.d, dt.h, dt.mi),
                                   getOrAddZone(zoneName)});
            }
        } else if (magic[0] == 0x49 && magic[1] == 0x49 &&
                   magic[2] == 0x49 && magic[3] == 0x49) {
            // ── Little-endian binary block (magic = "IIII") ─────────────────
            processBinaryBlock(file, readU16LE, readU32LE, entries);

        } else if (magic[0] == 0x4D && magic[1] == 0x4D &&
                   magic[2] == 0x4D && magic[3] == 0x4D) {
            // ── Big-endian binary block (magic = "MMMM") ────────────────────
            processBinaryBlock(file, readU16BE, readU32BE, entries);

        } else {
            throw std::runtime_error("Invalid magic bytes in " + logFilename);
        }
    }

    if (file.gcount() > 0)
        throw std::runtime_error("Unexpected end of file: " + logFilename);

    return {entries, m_dist, m_zoneIndex};
}

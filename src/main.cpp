#include "mil_base.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    const std::string baseFile = (argc > 1) ? argv[1] : "examples/base.txt";
    const std::string logFile  = (argc > 2) ? argv[2] : "examples/sample.log";

    CMilBase base;
    base.readBase(baseFile);
    CVisitorLog log = base.processLog(logFile);

    // Helper: print all suspects matching a filter, with a description header.
    auto printSuspects = [&](const CAuditFilter& filter, const std::string& desc) {
        std::cout << desc << "\n";
        const auto suspects = log.search(filter);
        if (suspects.empty()) {
            std::cout << "  (none)\n";
        } else {
            for (const auto& name : suspects)
                std::cout << "  \xC2\xB7 " << name << "\n"; // UTF-8 middle dot
        }
        std::cout << "\n";
    };

    printSuspects(
        CAuditFilter("armory"),
        "Who could have visited 'armory' (any time)?");

    printSuspects(
        CAuditFilter("armory").notBefore(2026, 3, 10, 8, 20).notAfter(2026, 3, 10, 8, 30),
        "Who could have visited 'armory' between 08:20-08:30 on 2026-03-10?");

    printSuspects(
        CAuditFilter("lab"),
        "Who could have visited 'lab' (any time)?");

    return EXIT_SUCCESS;
}

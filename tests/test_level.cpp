#include <cstdio>
#include <string>
#include <vector>

#include "Scene/Level.h"

using namespace vajra;

namespace {

void check(bool condition, const std::string& what, int& failures) {
    std::printf("  [%s] %s\n", condition ? " OK " : "FAIL", what.c_str());
    if (!condition) ++failures;
}

} // namespace

void runLevelTests(int& failures) {
    std::printf("\n=== Level Loading & Robustness Tests ===\n");

    // 1. Missing File Fallback
    {
        bool ok = true;
        Level lvl = Level::loadFromFile("non_existent_file_path.json", &ok);
        check(!ok, "Missing level sets outOk to false", failures);
        check(lvl.title == "FALLBACK BLOCKOUT", "Missing level loads fallback title", failures);
        check(!lvl.boxes.empty(), "Fallback level has static geometry", failures);
        check(!lvl.guards.empty(), "Fallback level has patrol guards", failures);
        check(!lvl.objectives.empty(), "Fallback level has mandatory objectives", failures);
    }

    // 2. Direct Fallback Creation
    {
        Level fb = Level::fallback();
        check(fb.groundSize > 0.0f, "Fallback ground size is positive", failures);
        check(fb.extractionRadius > 0.0f, "Fallback extraction radius is positive", failures);
        check(fb.playerStart.y >= 0.0f, "Fallback player starts above ground", failures);
    }
}

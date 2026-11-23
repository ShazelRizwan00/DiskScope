#include "test_framework.h"

int main() {
    int failedCases = 0;

    for (const dstest::TestCase& test : dstest::registry()) {
        const int failuresBefore = dstest::failures();
        std::printf("== %s\n", test.name);
        test.fn();
        if (dstest::failures() > failuresBefore) {
            ++failedCases;
        }
    }

    std::printf("\n---\n%d cases, %d checks, %d skipped, %d failed cases, %d failed checks\n",
                static_cast<int>(dstest::registry().size()), dstest::checks(), dstest::skipped(),
                failedCases, dstest::failures());

    return dstest::failures() == 0 ? 0 : 1;
}

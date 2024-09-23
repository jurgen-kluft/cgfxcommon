#include "cgfxcommon/c_offset_allocator.h"
#include "cgfxcommon/test_allocator.h"
#include "cunittest/cunittest.h"

using namespace ncore;

UNITTEST_SUITE_BEGIN(test_resource_pool)
{
    // Test the 'C' style resource pool
    UNITTEST_FIXTURE(main)
    {
        UNITTEST_ALLOCATOR;

        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}


    }
}
UNITTEST_SUITE_END

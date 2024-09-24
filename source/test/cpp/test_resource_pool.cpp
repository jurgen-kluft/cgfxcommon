#include "cgfxcommon/c_resource_pool.h"
#include "cgfxcommon/test_allocator.h"

#include "cunittest/cunittest.h"

using namespace ncore;

UNITTEST_SUITE_BEGIN(resource_pool)
{
    // Test the 'C' style resource pool
    UNITTEST_FIXTURE(main)
    {
        UNITTEST_ALLOCATOR;

        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}

        UNITTEST_TEST(test_init_shutdown)
        {
            ngfx::object_pool_t pool;
            pool.init(Allocator, 1024, 32);
            pool.shutdown(Allocator);
        }

        UNITTEST_TEST(obtain_release)
        {
            ngfx::object_pool_t pool;
            pool.init(Allocator, 1024, 32);

            u32 r = pool.obtain_resource();
            CHECK_EQUAL(0, r);
            pool.release_resource(r);

            pool.shutdown(Allocator);
        }
    }

    // Test the typed resource pool
    UNITTEST_FIXTURE(typed)
    {
        UNITTEST_ALLOCATOR;

        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}

        struct myresource_t
        {
            int   a;
            int   b;
            float c;
        };

        UNITTEST_TEST(test_init_shutdown)
        {
            ngfx::resource_pool_t<myresource_t> pool;
            pool.init(Allocator, 32);
            pool.shutdown();
        }

        UNITTEST_TEST(obtain_release)
        {
            ngfx::resource_pool_t<myresource_t> pool;
            pool.init(Allocator, 32);

            myresource_t* r1 = pool.obtain();
            r1->a          = 1;
            r1->b          = 2;
            r1->c          = 3.14f;
            myresource_t* r2 = pool.obtain();
            r2->a          = 4;
            r2->b          = 5;
            r2->c          = 6.28f;

            myresource_t* r11 = pool.get(0);
            CHECK_EQUAL(1, r11->a);
            CHECK_EQUAL(2, r11->b);
            CHECK_EQUAL(3.14f, r11->c);

            myresource_t* r22 = pool.get(1);
            CHECK_EQUAL(4, r22->a);
            CHECK_EQUAL(5, r22->b);
            CHECK_EQUAL(6.28f, r22->c);

            pool.release(r1);
            pool.release(r2);

            pool.shutdown();
        }
    }
}
UNITTEST_SUITE_END

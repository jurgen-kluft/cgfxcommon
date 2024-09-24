#include "cgfxcommon/c_resource_pool.h"
#include "cgfxcommon/test_allocator.h"

#include "cunittest/cunittest.h"

using namespace ncore;

namespace ncore
{
    namespace ngfx
    {
        struct resource_a_t
        {
            int   a;
            int   b;
            float c;
        };

        struct resource_b_t
        {
            int   a;
            int   b;
            float c;
        };

        struct resource_c_t
        {
            int   a;
            int   b;
            float c;
        };

        DEFINE_RESOURCE(resource_a_t);
        DEFINE_RESOURCE(resource_b_t);
        DEFINE_RESOURCE(resource_c_t);

        DECLARE_RESOURCE(resource_a_t);
        DECLARE_RESOURCE(resource_b_t);
        DECLARE_RESOURCE(resource_c_t);
    }
}

UNITTEST_SUITE_BEGIN(resource_pool)
{
    // Test the 'C' style resource pool
    UNITTEST_FIXTURE(objects)
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

            u32 r = pool.obtain();
            CHECK_EQUAL(0, r);
            pool.release(r);

            pool.shutdown(Allocator);
        }
    }

    // Test the typed resource pool
    UNITTEST_FIXTURE(types)
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

            myresource_t* r1 = pool.obtain_access();
            r1->a            = 1;
            r1->b            = 2;
            r1->c            = 3.14f;
            myresource_t* r2 = pool.obtain_access();
            r2->a            = 4;
            r2->b            = 5;
            r2->c            = 6.28f;

            myresource_t* r11 = pool.get_access(0);
            CHECK_EQUAL(1, r11->a);
            CHECK_EQUAL(2, r11->b);
            CHECK_EQUAL(3.14f, r11->c);

            myresource_t* r22 = pool.get_access(1);
            CHECK_EQUAL(4, r22->a);
            CHECK_EQUAL(5, r22->b);
            CHECK_EQUAL(6.28f, r22->c);

            pool.release(0);
            pool.release(1);

            pool.shutdown();
        }
    }


    // Test the resources pool
    UNITTEST_FIXTURE(resources)
    {
        UNITTEST_ALLOCATOR;

        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}

        UNITTEST_TEST(test_init_shutdown)
        {
            ngfx::resources_pool_t pool;
            pool.init(Allocator, 32, 4);
            pool.shutdown();
        }

        UNITTEST_TEST(register_resource_types)
        {
            ngfx::resources_pool_t pool;
            pool.init(Allocator, 32, 4);

            ngfx::handle_t h1 = pool.register_resource<ngfx::resource_a_t>();
            CHECK_EQUAL((0<<24) | 0, h1);
            ngfx::handle_t h2 = pool.register_resource<ngfx::resource_b_t>();
            CHECK_EQUAL((1<<24) | 0, h2);
            ngfx::handle_t h3 = pool.register_resource<ngfx::resource_c_t>();
            CHECK_EQUAL((2<<24) | 0, h3);

            CHECK_TRUE(pool.is_resource_type<ngfx::resource_a_t>(h1));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_b_t>(h1));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_c_t>(h1));

            CHECK_TRUE(pool.is_resource_type<ngfx::resource_b_t>(h2));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_a_t>(h2));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_c_t>(h2));

            CHECK_TRUE(pool.is_resource_type<ngfx::resource_c_t>(h3));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_a_t>(h3));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_b_t>(h3));

            pool.release_resource(h1);
            pool.release_resource(h2);
            pool.release_resource(h3);

            pool.shutdown();
        }
    }
}
UNITTEST_SUITE_END

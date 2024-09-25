#include "cgfxcommon/c_resource_pool.h"
#include "cgfxcommon/test_allocator.h"

#include "cunittest/cunittest.h"

using namespace ncore;

namespace ncore
{
    namespace ngfx
    {
        struct object_a_t
        {
            int   a;
            int   b;
            float c;
        };

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

        DEFINE_OBJECT_TYPE(object_a_t);
        DECLARE_OBJECT_TYPE(object_a_t);

        DEFINE_OBJECT_RESOURCE_TYPE(resource_a_t);
        DEFINE_OBJECT_RESOURCE_TYPE(resource_b_t);
        DEFINE_OBJECT_RESOURCE_TYPE(resource_c_t);

        DECLARE_OBJECT_RESOURCE_TYPE(resource_a_t);
        DECLARE_OBJECT_RESOURCE_TYPE(resource_b_t);
        DECLARE_OBJECT_RESOURCE_TYPE(resource_c_t);
    }  // namespace ngfx
}  // namespace ncore

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
            ngfx::nobject::pool_t pool;
            pool.init(Allocator, 1024, 32);
            pool.shutdown(Allocator);
        }

        UNITTEST_TEST(obtain_release)
        {
            ngfx::nobject::pool_t pool;
            pool.init(Allocator, 1024, 32);

            u32 r = pool.alloc();
            CHECK_EQUAL(0, r);
            pool.dealloc(r);

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
            ngfx::nobject::ntyped::pool_t<myresource_t> pool;
            pool.init(Allocator, 32);
            pool.shutdown();
        }

        UNITTEST_TEST(obtain_release)
        {
            ngfx::nobject::ntyped::pool_t<myresource_t> pool;
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

            pool.dealloc(0);
            pool.dealloc(1);

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
            ngfx::nresources::pool_t pool;
            pool.init(Allocator, 32, 4);
            pool.shutdown();
        }

        UNITTEST_TEST(register_resource_types)
        {
            ngfx::nresources::pool_t pool;
            pool.init(Allocator, 32, 4);

            CHECK_TRUE(pool.register_resource<ngfx::resource_a_t>());
            CHECK_TRUE(pool.register_resource<ngfx::resource_b_t>());
            CHECK_TRUE(pool.register_resource<ngfx::resource_c_t>());

            ngfx::handle_t h1 = pool.alloc_resource<ngfx::resource_a_t>();
            CHECK_EQUAL(0, h1.index);
            CHECK_EQUAL(0, h1.type);
            ngfx::handle_t h2 = pool.alloc_resource<ngfx::resource_b_t>();
            CHECK_EQUAL(0, h2.index);
            CHECK_EQUAL(1, h2.type);
            ngfx::handle_t h3 = pool.alloc_resource<ngfx::resource_c_t>();
            CHECK_EQUAL(0, h3.index);
            CHECK_EQUAL(2, h3.type);

            CHECK_TRUE(pool.is_resource_type<ngfx::resource_a_t>(h1));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_b_t>(h1));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_c_t>(h1));

            CHECK_TRUE(pool.is_resource_type<ngfx::resource_b_t>(h2));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_a_t>(h2));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_c_t>(h2));

            CHECK_TRUE(pool.is_resource_type<ngfx::resource_c_t>(h3));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_a_t>(h3));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_b_t>(h3));

            pool.dealloc_resource(h1);
            pool.dealloc_resource(h2);
            pool.dealloc_resource(h3);

            pool.shutdown();
        }
    }

    // Test the object resources pool
    UNITTEST_FIXTURE(object_resources)
    {
        UNITTEST_ALLOCATOR;

        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}

        UNITTEST_TEST(test_init_shutdown)
        {
            ngfx::nobject_resources::pool_t pool;
            pool.init(Allocator, 32, 4);
            pool.shutdown();
        }

        UNITTEST_TEST(register_object_and_resource_types)
        {
            ngfx::nobject_resources::pool_t pool;
            pool.init(Allocator, 32, 4);

            bool obj_a = pool.register_object_type<ngfx::object_a_t>(32);
            CHECK_TRUE(obj_a);

            bool obj_a_res_a = pool.register_resource_type<ngfx::object_a_t, ngfx::resource_a_t>(32);
            CHECK_TRUE(obj_a_res_a);

            bool obj_a_res_b = pool.register_resource_type<ngfx::object_a_t, ngfx::resource_b_t>(32);
            CHECK_TRUE(obj_a_res_b);

            bool obj_a_res_c = pool.register_resource_type<ngfx::object_a_t, ngfx::resource_c_t>(32);
            CHECK_TRUE(obj_a_res_c);

            ngfx::handle_t o1 = pool.alloc_object<ngfx::object_a_t>();
            CHECK_EQUAL(0, o1.index);
            CHECK_EQUAL(0, o1.type);

            ngfx::handle_t h1 = pool.add_resource<ngfx::resource_a_t>(o1);
            CHECK_EQUAL(0, h1.index);
            CHECK_EQUAL(0, h1.type);
            ngfx::handle_t h2 = pool.add_resource<ngfx::resource_b_t>(o1);
            CHECK_EQUAL(0, h2.index);
            CHECK_EQUAL(1, h2.type);
            ngfx::handle_t h3 = pool.add_resource<ngfx::resource_c_t>(o1);
            CHECK_EQUAL(0, h3.index);
            CHECK_EQUAL(2, h3.type);

            CHECK_TRUE(pool.is_resource_type<ngfx::resource_a_t>(h1));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_b_t>(h1));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_c_t>(h1));

            CHECK_TRUE(pool.is_resource_type<ngfx::resource_b_t>(h2));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_a_t>(h2));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_c_t>(h2));

            CHECK_TRUE(pool.is_resource_type<ngfx::resource_c_t>(h3));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_a_t>(h3));
            CHECK_FALSE(pool.is_resource_type<ngfx::resource_b_t>(h3));

            pool.dealloc_resource(h1);
            pool.dealloc_resource(h2);
            pool.dealloc_resource(h3);

            pool.shutdown();
        }
    }
}
UNITTEST_SUITE_END

#include "cgfxcommon/c_resource_pool.h"
#include "cgfxcommon/test_allocator.h"

#include "cunittest/cunittest.h"

using namespace ncore;

namespace ncore
{
    namespace ngfx
    {
        enum EObjectTypes
        {
            kObjectA = 0,
            kObjectB = 1,
        };

        enum EResourceTypes
        {
            kResourceA = 0,
            kResourceB = 1,
            kResourceC = 2,
        };

        struct object_a_t
        {
            DECLARE_OBJECT_TYPE(kObjectA);
            int   a;
            int   b;
            float c;
        };

        struct object_b_t
        {
            DECLARE_OBJECT_TYPE(kObjectB);
            int   a;
            int   b;
            float c;
        };

        struct resource_a_t
        {
            DECLARE_RESOURCE_TYPE(kResourceA);
            int   a;
            int   b;
            float c;
        };

        struct resource_b_t
        {
            DECLARE_RESOURCE_TYPE(kResourceB);
            int   a;
            int   b;
            float c;
        };

        struct resource_c_t
        {
            DECLARE_RESOURCE_TYPE(kResourceC);
            int   a;
            int   b;
            float c;
        };

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
            ngfx::nobject::array_t array;
            array.setup(Allocator, 1024, 32);
            ngfx::nobject::pool_t pool;
            pool.setup(&array, Allocator);

            pool.teardown(Allocator);
            array.teardown(Allocator);
        }

        UNITTEST_TEST(obtain_release)
        {
            ngfx::nobject::array_t array;
            array.setup(Allocator, 1024, 32);
            ngfx::nobject::pool_t pool;
            pool.setup(&array, Allocator);

            u32 r = pool.allocate();
            CHECK_EQUAL(0, r);
            pool.deallocate(r);

            pool.teardown(Allocator);
            array.teardown(Allocator);
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
            pool.setup(Allocator, 32);
            pool.teardown();
        }

        UNITTEST_TEST(obtain_release)
        {
            ngfx::nobject::ntyped::pool_t<myresource_t> pool;
            pool.setup(Allocator, 32);

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

            pool.deallocate(0);
            pool.deallocate(1);

            pool.teardown();
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
            pool.setup(Allocator, 32);
            pool.teardown();
        }

        UNITTEST_TEST(register_resource_types)
        {
            ngfx::nresources::pool_t pool;
            pool.setup(Allocator, 4);

            CHECK_TRUE(pool.register_resource<ngfx::resource_a_t>(32));
            CHECK_TRUE(pool.register_resource<ngfx::resource_b_t>(32));
            CHECK_TRUE(pool.register_resource<ngfx::resource_c_t>(32));

            ngfx::handle_t h1 = pool.allocate<ngfx::resource_a_t>();
            CHECK_EQUAL(0, h1.index);
            CHECK_EQUAL(0, h1.type);
            ngfx::handle_t h2 = pool.allocate<ngfx::resource_b_t>();
            CHECK_EQUAL(0, h2.index);
            CHECK_EQUAL(1, h2.type);
            ngfx::handle_t h3 = pool.construct<ngfx::resource_c_t>();
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

            pool.deallocate(h1);
            pool.deallocate(h2);
            pool.destruct<ngfx::resource_c_t>(h3);

            ngfx::handle_t h4 = pool.construct<ngfx::resource_a_t>();
            CHECK_EQUAL(0, h4.index);
            CHECK_EQUAL(0, h4.type);
            pool.destruct<ngfx::resource_a_t>(h4);

            pool.teardown();
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
            ngfx::nobjects_with_resources::pool_t pool;
            pool.setup(Allocator, 32, 4);
            pool.teardown();
        }

        UNITTEST_TEST(register_object_and_resource_types)
        {
            ngfx::nobjects_with_resources::pool_t pool;
            pool.setup(Allocator, 30, 30);

            bool obj_a = pool.register_object_type<ngfx::object_a_t>(40);
            CHECK_TRUE(obj_a);

            bool obj_b = pool.register_object_type<ngfx::object_b_t>(40);
            CHECK_TRUE(obj_b);

            bool obj_a_res_a = pool.register_resource_type<ngfx::object_a_t, ngfx::resource_a_t>();
            CHECK_TRUE(obj_a_res_a);

            bool obj_a_res_b = pool.register_resource_type<ngfx::object_a_t, ngfx::resource_b_t>();
            CHECK_TRUE(obj_a_res_b);

            bool obj_a_res_c = pool.register_resource_type<ngfx::object_a_t, ngfx::resource_c_t>();
            CHECK_TRUE(obj_a_res_c);

            ngfx::handle_t oa1 = pool.allocate_object<ngfx::object_a_t>();
            CHECK_EQUAL(0, oa1.index & 0x0FFFFFFF);
            CHECK_EQUAL(0xFFFF, oa1.type);

            ngfx::handle_t oa2 = pool.allocate_object<ngfx::object_a_t>();
            CHECK_EQUAL(1, oa2.index & 0x0FFFFFFF);
            CHECK_EQUAL(0xFFFF, oa2.type);

            ngfx::handle_t ob1 = pool.allocate_object<ngfx::object_b_t>();
            CHECK_EQUAL(0, ob1.index & 0x0FFFFFFF);
            CHECK_EQUAL(0x0001FFFF, ob1.type);

            ngfx::handle_t h1 = pool.allocate_resource<ngfx::resource_a_t>(oa1);
            CHECK_EQUAL(0, h1.index & 0x00FFFFFF);
            CHECK_EQUAL(0, h1.type);
            ngfx::handle_t h2 = pool.construct_resource<ngfx::resource_b_t>(oa1);
            CHECK_EQUAL(0, h2.index & 0x00FFFFFF);
            CHECK_EQUAL(1, h2.type);
            ngfx::handle_t h3 = pool.allocate_resource<ngfx::resource_c_t>(oa1);
            CHECK_EQUAL(0, h3.index & 0x00FFFFFF);
            CHECK_EQUAL(2, h3.type);

            CHECK_TRUE(pool.is_object<ngfx::object_a_t>(oa1));
            CHECK_FALSE(pool.is_object<ngfx::object_b_t>(oa1));
            CHECK_TRUE(pool.is_object<ngfx::object_b_t>(ob1));
            CHECK_FALSE(pool.is_object<ngfx::object_a_t>(ob1));

            CHECK_TRUE(pool.is_resource<ngfx::resource_a_t>(h1));
            CHECK_FALSE(pool.is_resource<ngfx::resource_b_t>(h1));
            CHECK_FALSE(pool.is_resource<ngfx::resource_c_t>(h1));

            CHECK_TRUE(pool.is_resource<ngfx::resource_b_t>(h2));
            CHECK_FALSE(pool.is_resource<ngfx::resource_a_t>(h2));
            CHECK_FALSE(pool.is_resource<ngfx::resource_c_t>(h2));

            CHECK_TRUE(pool.is_resource<ngfx::resource_c_t>(h3));
            CHECK_FALSE(pool.is_resource<ngfx::resource_a_t>(h3));
            CHECK_FALSE(pool.is_resource<ngfx::resource_b_t>(h3));

            pool.deallocate_resource(h1);
            pool.destruct_resource<ngfx::resource_b_t>(h2);
            pool.deallocate_resource(h3);

            pool.deallocate_object(oa1);
            pool.deallocate_object(oa2);
            pool.destruct_object<ngfx::object_b_t>(ob1);

            pool.teardown();
        }
    }
}
UNITTEST_SUITE_END

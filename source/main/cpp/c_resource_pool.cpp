#include "cbase/c_allocator.h"
#include "cbase/c_integer.h"
#include "cbase/c_memory.h"
#include "cgfxcommon/c_resource_pool.h"

namespace ncore
{
    namespace ngfx
    {
        namespace nobject
        {
            array_t::array_t()
                : m_memory(nullptr)
                , m_num_used(0)
                , m_num_max(0)
                , m_sizeof(0)
            {
            }

            void array_t::init(alloc_t* allocator, u32 max_num_resources, u32 sizeof_resource)
            {
                ASSERT(sizeof_resource >= sizeof(u32));  // Resource size must be at least the size of a u32 since we use it as a linked list.

                m_sizeof   = ncore::math::alignUp(sizeof_resource, (u32)sizeof(void*));
                m_memory   = (byte*)allocator->allocate(max_num_resources * m_sizeof);
                m_num_used = 0;
                m_num_max  = max_num_resources;
                m_sizeof   = sizeof_resource;
            }

            void array_t::shutdown(alloc_t* allocator)
            {
                ASSERT(m_num_used == 0);
                allocator->deallocate(m_memory);
            }

            void* array_t::get_access(u32 index) { return &m_memory[index * m_sizeof]; }

            const void* array_t::get_access(u32 index) const { return &m_memory[index * m_sizeof]; }

            // ------------------------------------------------------------------------------------------------

            inventory_t::inventory_t()
                : m_bitarray(nullptr)
                , m_array()
            {
            }

            void inventory_t::init(alloc_t* allocator, u32 max_num_resources, u32 sizeof_resource)
            {
                m_array.init(allocator, max_num_resources, sizeof_resource);
                m_bitarray = (u32*)g_allocate_and_clear(allocator, ((max_num_resources + 31) / 32) * sizeof(u32));
            }

            void inventory_t::shutdown(alloc_t* allocator)
            {
                m_array.shutdown(allocator);
                allocator->deallocate(m_bitarray);
            }

            void inventory_t::free_all()
            {
                m_array.m_num_used = 0;
                nmem::memset(m_bitarray, 0, ((m_array.m_num_max + 31) / 32) * sizeof(u32));
            }


            // ------------------------------------------------------------------------------------------------
            pool_t::pool_t()
                : m_object_array()
                , m_free_resource_index(0)
                , m_free_resource_map()
            {
            }

            void pool_t::init(alloc_t* allocator, u32 max_num_resources, u32 sizeof_resource)
            {
                m_object_array.init(allocator, max_num_resources, sizeof_resource);

                m_free_resource_map.init_all_used_lazy(max_num_resources, allocator);
                m_free_resource_index = 0;
            }

            void pool_t::shutdown(alloc_t* allocator)
            {
                m_object_array.shutdown(allocator);
                m_free_resource_map.release(allocator);
            }

            void pool_t::free_all()
            {
                m_free_resource_index     = 0;
                m_object_array.m_num_used = 0;
                m_free_resource_map.init_all_used_lazy();
            }

            u32 pool_t::allocate()
            {
                if (m_free_resource_index < m_object_array.m_num_max)
                {
                    const u32 free_index = m_free_resource_index++;
                    m_free_resource_map.tick_all_used_lazy(free_index);
                    m_object_array.m_num_used++;
                    return free_index;
                }
                s32 const free_index = m_free_resource_map.find_and_set();
                if (free_index >= 0)
                {
                    m_object_array.m_num_used++;
                    return free_index;
                }
                ASSERTS(false, "Error: no more resources left!");
                return c_invalid_handle;
            }

            void pool_t::deallocate(u32 index)
            {
                ASSERT(index < m_free_resource_index);

                m_object_array.m_num_used--;
                if (m_object_array.m_num_used == 0)
                {
                    m_free_resource_index = 0;
                    u32 l0len, l1len, l2len, l3len;
                    binmap_t::compute_levels(m_object_array.m_num_max, l0len, l1len, l2len, l3len);
                    u32* const l1 = m_free_resource_map.m_l[0];
                    u32* const l2 = m_free_resource_map.m_l[1];
                    u32* const l3 = m_free_resource_map.m_l[2];
                    m_free_resource_map.init_all_used_lazy(m_object_array.m_num_max, l0len, l1, l1len, l2, l2len, l3, l3len);
                }
                else
                {
                    m_free_resource_map.set_free(index);
                }
            }

            void* pool_t::get_access(u32 index)
            {
                ASSERT(index < m_free_resource_index);
                if (index != c_invalid_handle)
                {
                    ASSERTS(m_free_resource_map.is_used(index), "Error: resource is not marked as being in use!");
                    return &m_object_array.m_memory[index * m_object_array.m_sizeof];
                }
                return nullptr;
            }

            const void* pool_t::get_access(u32 index) const
            {
                ASSERT(index < m_free_resource_index);
                if (index != c_invalid_handle)
                {
                    ASSERTS(m_free_resource_map.is_used(index), "Error: resource is not marked as being in use!");
                    return &m_object_array.m_memory[index * m_object_array.m_sizeof];
                }
                return nullptr;
            }
        }  // namespace nobject

        namespace nresources
        {
            const handle_t pool_t::c_invalid_handle = {0xFFFFFFFF, 0xFFFFFFFF};

            void pool_t::init(alloc_t* allocator, u16 max_types)
            {
                m_allocator = allocator;
                m_num_types = 0;
                m_max_types = max_types;
                m_types     = (type_t*)allocator->allocate(max_types * sizeof(type_t));
            }

            void pool_t::shutdown()
            {
                for (u32 i = 0; i < m_num_types; i++)
                {
                    m_types[i].m_resource_pool->shutdown(m_allocator);
                    m_allocator->deallocate(m_types[i].m_resource_pool);
                }
                m_allocator->deallocate(m_types);
            }

            void* pool_t::get_access_raw(handle_t handle)
            {
                ASSERT(handle.type < m_num_types);
                return m_types[handle.type].m_resource_pool->get_access(handle.index);
            }

            const void* pool_t::get_access_raw(handle_t handle) const
            {
                ASSERT(handle.type < m_num_types);
                return m_types[handle.type].m_resource_pool->get_access(handle.index);
            }

            void pool_t::register_resource_pool(s16 type_index, u32 max_num_resources, u32 sizeof_resource)
            {
                ASSERT(type_index < m_max_types);
                m_types[type_index].m_resource_pool = m_allocator->construct<nobject::pool_t>();
                m_types[type_index].m_resource_pool->init(m_allocator, max_num_resources, sizeof_resource);
            }
        }  // namespace nresources

        namespace nobjects_with_resources
        {
            const handle_t pool_t::c_invalid_handle = {0xFFFFFFFF, 0xFFFFFFFF};

            void pool_t::init(alloc_t* allocator, u32 max_num_object_types, u32 max_num_resource_types)
            {
                m_allocator          = allocator;
                m_objects            = (object_t*)allocator->allocate(max_num_object_types * sizeof(object_t));
                m_num_object_types   = 0;
                m_max_object_types   = max_num_object_types;
                m_num_resource_types = 0;
                m_max_resource_types = max_num_resource_types;
            }

            void pool_t::shutdown()
            {
                for (u32 i = 0; i < m_num_object_types; i++)
                {
                    for (u32 j = 0; j < m_num_resource_types; j++)
                    {
                        if (m_objects[i].m_a_resources[j] != nullptr)
                        {
                            m_objects[i].m_a_resources[j]->shutdown(m_allocator);
                            m_allocator->deallocate(m_objects[i].m_a_resources[j]);
                        }
                    }
                    m_allocator->deallocate(m_objects[i].m_a_resources);

                    m_objects[i].m_object_pool->shutdown(m_allocator);
                    m_allocator->deallocate(m_objects[i].m_object_pool);
                }
                m_allocator->deallocate(m_objects);
            }

            void* pool_t::get_access(handle_t handle)
            {
                if (is_handle_an_object(handle))
                {
                    const u16 object_type_index = get_object_type_index(handle);
                    const u32 object_index      = get_object_index(handle);
                    return m_objects[object_type_index].m_object_pool->get_access(object_index);
                }
                else if (is_handle_a_resource(handle))
                {
                    const u16 object_type_index   = get_object_type_index(handle);
                    const u16 resource_type_index = get_resource_type_index(handle);
                    const u32 resource_index      = get_resource_index(handle);
                    return m_objects[object_type_index].m_a_resources[resource_type_index]->get_access(resource_index);
                }
                return nullptr;
            }

            const void* pool_t::get_access(handle_t handle) const
            {
                if (is_handle_an_object(handle))
                {
                    const u16 object_type_index = get_object_type_index(handle);
                    const u32 object_index      = get_object_index(handle);
                    return m_objects[object_type_index].m_object_pool->get_access(object_index);
                }
                else if (is_handle_a_resource(handle))
                {
                    const u16 object_type_index   = get_object_type_index(handle);
                    const u16 resource_type_index = get_resource_type_index(handle);
                    const u32 resource_index      = get_resource_index(handle);
                    return m_objects[object_type_index].m_a_resources[resource_type_index]->get_access(resource_index);
                }
                return nullptr;
            }

            void pool_t::register_object_type(u16 object_type_index, u32 max_num_objects, u32 sizeof_object, u32 max_num_resources)
            {
                ASSERT(object_type_index < m_max_object_types);
                m_objects[object_type_index].m_object_pool = m_allocator->construct<nobject::pool_t>();
                m_objects[object_type_index].m_object_pool->init(m_allocator, max_num_objects, sizeof_object);
                m_objects[object_type_index].m_a_resources = (nobject::inventory_t**)g_allocate_and_clear(m_allocator, max_num_resources * sizeof(nobject::inventory_t*));
            }

            void pool_t::register_resource_type(u16 object_type_index, u16 resource_type_index, u32 sizeof_resource)
            {
                ASSERT(object_type_index < m_max_object_types);
                ASSERT(resource_type_index < m_max_resource_types);
                const u32 max_num_resources                                     = m_objects[object_type_index].m_object_pool->m_object_array.m_num_max;
                m_objects[object_type_index].m_a_resources[resource_type_index] = m_allocator->construct<nobject::inventory_t>();
                m_objects[object_type_index].m_a_resources[resource_type_index]->init(m_allocator, max_num_resources, sizeof_resource);
            }

            handle_t pool_t::allocate_object(u16 object_type_index)
            {
                ASSERT(object_type_index < m_max_object_types);
                const u32 object_index = m_objects[object_type_index].m_object_pool->allocate();
                return make_object_handle(object_type_index, object_index);
            }

            handle_t pool_t::allocate_resource(handle_t object_handle, u16 resource_type_index)
            {
                const u32 object_index = get_object_index(object_handle);
                const u16 object_type_index = get_object_type_index(object_handle);
                ASSERT(object_type_index < m_max_object_types);
                ASSERT(resource_type_index < m_max_resource_types);
                m_objects[object_type_index].m_a_resources[resource_type_index]->allocate(object_index);
                return make_resource_handle(object_type_index, resource_type_index, object_index);
            }

        }  // namespace nobjects_with_resources
    }  // namespace ngfx
}  // namespace ncore

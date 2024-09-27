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
                , m_num_max(0)
                , m_sizeof(0)
            {
            }

            void array_t::setup(alloc_t* allocator, u32 max_num_resources, u32 sizeof_resource)
            {
                ASSERT(sizeof_resource >= sizeof(u32));  // Resource size must be at least the size of a u32 since we use it as a linked list.

                m_sizeof  = ncore::math::alignUp(sizeof_resource, (u32)sizeof(void*));
                m_memory  = (byte*)allocator->allocate(max_num_resources * m_sizeof);
                m_num_max = max_num_resources;
                m_sizeof  = sizeof_resource;
            }

            void array_t::teardown(alloc_t* allocator) { allocator->deallocate(m_memory); }

            // ------------------------------------------------------------------------------------------------

            inventory_t::inventory_t()
                : m_bitarray(nullptr)
                , m_array()
            {
            }

            void inventory_t::setup(alloc_t* allocator, u32 max_num_resources, u32 sizeof_resource)
            {
                m_array.setup(allocator, max_num_resources, sizeof_resource);
                m_bitarray = (u32*)g_allocate_and_clear(allocator, ((max_num_resources + 31) / 32) * sizeof(u32));
            }

            void inventory_t::teardown(alloc_t* allocator)
            {
                m_array.teardown(allocator);
                allocator->deallocate(m_bitarray);
            }

            void inventory_t::free_all() { nmem::memset(m_bitarray, 0, ((m_array.m_num_max + 31) / 32) * sizeof(u32)); }

            // ------------------------------------------------------------------------------------------------
            pool_t::pool_t()
                : m_object_array()
                , m_free_resource_map()
            {
            }

            void pool_t::setup(array_t* object_array, alloc_t* allocator)
            {
                m_object_array = object_array;
                m_free_resource_map.init_all_free(object_array->m_num_max, allocator);
            }

            void pool_t::teardown(alloc_t* allocator)
            {
                m_object_array = nullptr;
                m_free_resource_map.release(allocator);
            }

            void pool_t::free_all() { m_free_resource_map.init_all_free(); }

            u32 pool_t::allocate()
            {
                s32 const index = m_free_resource_map.find_and_set();
                ASSERTS(index >= 0, "Error: no more resources left!");
                return index;
            }

            void pool_t::deallocate(u32 index) { m_free_resource_map.set_free(index); }

            void* pool_t::get_access(u32 index)
            {
                ASSERT(index != c_invalid_handle);
                ASSERTS(m_free_resource_map.is_used(index), "Error: resource is not marked as being in use!");
                return &m_object_array->m_memory[index * m_object_array->m_sizeof];
            }

            const void* pool_t::get_access(u32 index) const
            {
                ASSERT(index != c_invalid_handle);
                ASSERTS(m_free_resource_map.is_used(index), "Error: resource is not marked as being in use!");
                return &m_object_array->m_memory[index * m_object_array->m_sizeof];
            }
        }  // namespace nobject

        namespace nresources
        {
            const handle_t pool_t::c_invalid_handle = {0xFFFFFFFF, 0xFFFFFFFF};

            void pool_t::setup(alloc_t* allocator, u16 max_types)
            {
                m_allocator = allocator;
                m_num_pools = max_types;
                m_pools     = (nobject::pool_t**)g_allocate_and_clear(allocator, max_types * sizeof(nobject::pool_t*));
            }

            void pool_t::teardown()
            {
                for (u32 i = 0; i < m_num_pools; i++)
                {
                    if (m_pools[i] != nullptr)
                    {
                        m_pools[i]->m_object_array->teardown(m_allocator);
                        m_allocator->deallocate(m_pools[i]->m_object_array);
                        m_pools[i]->teardown(m_allocator);
                        m_allocator->deallocate(m_pools[i]);
                    }
                }
                m_allocator->deallocate(m_pools);
            }

            void* pool_t::get_access_raw(handle_t handle)
            {
                ASSERT(handle.type < m_num_pools);
                return m_pools[handle.type]->get_access(handle.index);
            }

            const void* pool_t::get_access_raw(handle_t handle) const
            {
                ASSERT(handle.type < m_num_pools);
                return m_pools[handle.type]->get_access(handle.index);
            }

            bool pool_t::register_resource_pool(s16 type_index, u32 max_num_resources, u32 sizeof_resource)
            {
                if (m_pools[type_index] == nullptr)
                {
                    ASSERT(type_index < m_num_pools);
                    nobject::array_t* array = m_allocator->construct<nobject::array_t>();
                    array->setup(m_allocator, max_num_resources, sizeof_resource);
                    m_pools[type_index] = m_allocator->construct<nobject::pool_t>();
                    m_pools[type_index]->setup(array, m_allocator);
                    return true;
                }
                return false;
            }
        }  // namespace nresources

        namespace nobjects_with_resources
        {
            const handle_t pool_t::c_invalid_handle = {0xFFFFFFFF, 0xFFFFFFFF};

            void pool_t::setup(alloc_t* allocator, u32 max_num_object_types, u32 max_num_resource_types)
            {
                m_allocator          = allocator;
                m_objects            = (object_t*)g_allocate_and_clear(allocator, max_num_object_types * sizeof(object_t));
                m_max_object_types   = max_num_object_types;
                m_max_resource_types = max_num_resource_types + 1; // +1 for object
            }

            void pool_t::teardown()
            {
                for (u32 i = 0; i < m_max_object_types; i++)
                {
                    if (m_objects[i].m_object_map.m_count > 0)
                    {
                        for (u32 j = 0; j < m_max_resource_types; j++)
                        {
                            if (m_objects[i].m_a_resources[j] != nullptr)
                            {
                                m_objects[i].m_a_resources[j]->teardown(m_allocator);
                                m_allocator->deallocate(m_objects[i].m_a_resources[j]);
                            }
                        }
                        m_allocator->deallocate(m_objects[i].m_a_resources);
                        m_objects[i].m_object_map.release(m_allocator);
                    }
                }
                m_allocator->deallocate(m_objects);
            }

            bool pool_t::register_object_type(u16 object_type_index, u32 max_num_objects, u32 sizeof_object, u32 max_num_resources)
            {
                ASSERT(m_objects[object_type_index].m_object_map.m_count == 0);
                if (m_objects[object_type_index].m_object_map.m_count == 0)
                {
                    ASSERT(object_type_index < m_max_object_types);
                    m_objects[object_type_index].m_object_map.init_all_free(max_num_objects, m_allocator);
                    m_objects[object_type_index].m_a_resources = (nobject::inventory_t**)g_allocate_and_clear(m_allocator, max_num_resources * sizeof(nobject::inventory_t*));
                    m_objects[object_type_index].m_a_resources[0] = m_allocator->construct<nobject::inventory_t>();
                    m_objects[object_type_index].m_a_resources[0]->setup(m_allocator, max_num_objects, sizeof_object);
                    return true;
                }
                return false;
            }

            bool pool_t::register_resource_type(u16 object_type_index, u16 resource_type_index, u32 sizeof_resource)
            {
                ASSERT(m_objects[object_type_index].m_a_resources[resource_type_index + 1] == nullptr);
                if (m_objects[object_type_index].m_a_resources[resource_type_index + 1] == nullptr)
                {
                    ASSERT(object_type_index < m_max_object_types);
                    ASSERT(resource_type_index < m_max_resource_types);
                    const u32 max_num_resources                                         = m_objects[object_type_index].m_object_map.m_count;
                    m_objects[object_type_index].m_a_resources[resource_type_index + 1] = m_allocator->construct<nobject::inventory_t>();
                    m_objects[object_type_index].m_a_resources[resource_type_index + 1]->setup(m_allocator, max_num_resources, sizeof_resource);
                    return true;
                }
                return false;
            }

            handle_t pool_t::allocate_object(u16 object_type_index)
            {
                ASSERT(object_type_index < m_max_object_types);
                const u32 object_index = m_objects[object_type_index].m_object_map.find_and_set();
                return make_object_handle(object_type_index, object_index);
            }

            handle_t pool_t::allocate_resource(handle_t object_handle, u16 resource_type_index)
            {
                const u32 object_index      = get_object_index(object_handle);
                const u16 object_type_index = get_object_type_index(object_handle);
                ASSERT(object_type_index < m_max_object_types);
                ASSERT(resource_type_index < m_max_resource_types);
                m_objects[object_type_index].m_a_resources[resource_type_index + 1]->allocate(object_index);
                return make_resource_handle(object_type_index, resource_type_index, object_index);
            }

        }  // namespace nobjects_with_resources
    }  // namespace ngfx
}  // namespace ncore

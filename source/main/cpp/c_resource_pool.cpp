#include "cbase/c_allocator.h"
#include "cbase/c_integer.h"
#include "cgfxcommon/c_resource_pool.h"

namespace ncore
{
    namespace ngfx
    {
        // TODO Use hbb_t to identify free resources since it has a bias to allocate from the beginning of the pool.
        // TODO: add bits for checking if resource is alive and use bitmasks.

        void object_pool_t::init(alloc_t* allocator, u32 max_num_resources, u32 sizeof_resource)
        {
            ASSERT(sizeof_resource >= sizeof(u32));  // Resource size must be at least the size of a u32 since we use it as a linked list.

            m_free_resource_map.init_all_used_lazy(max_num_resources, allocator);

            m_resource_sizeof     = ncore::math::alignUp(sizeof_resource, (u32)sizeof(void*));
            m_resource_memory     = (byte*)allocator->allocate(max_num_resources * m_resource_sizeof);
            m_free_resource_index = 0;
            m_num_resource_used   = 0;
            m_num_resource_max    = max_num_resources;
            m_resource_sizeof     = sizeof_resource;
        }

        void object_pool_t::shutdown(alloc_t* allocator)
        {
            ASSERT(m_num_resource_used == 0);
            allocator->deallocate(m_resource_memory);
            m_free_resource_map.release(allocator);
        }

        void object_pool_t::free_all_resources()
        {
            m_free_resource_index = 0;
            m_num_resource_used   = 0;
            m_free_resource_map.init_all_used_lazy();
        }

        u32 object_pool_t::obtain_resource()
        {
            if (m_free_resource_index < m_num_resource_max)
            {
                const u32 free_index = m_free_resource_index++;
                m_free_resource_map.tick_all_used_lazy(free_index);
                m_num_resource_used++;
                return free_index;
            }
            s32 const free_index = m_free_resource_map.find_and_set();
            if (free_index >= 0)
            {
                m_num_resource_used++;
                return free_index;
            }
            ASSERTS(false, "Error: no more resources left!");
            return c_invalid_index;
        }

        void object_pool_t::release_resource(u32 handle)
        {
            ASSERT(handle < m_free_resource_index);

            m_num_resource_used--;
            if (m_num_resource_used == 0)
            {
                m_free_resource_index = 0;
                u32 l0len, l1len, l2len, l3len;
                binmap_t::compute_levels(m_num_resource_max, l0len, l1len, l2len, l3len);
                u32* const l1 = m_free_resource_map.m_l[0];
                u32* const l2 = m_free_resource_map.m_l[1];
                u32* const l3 = m_free_resource_map.m_l[2];
                m_free_resource_map.init_all_used_lazy(m_num_resource_max, l0len, l1, l1len, l2, l2len, l3, l3len);
            }
            else
            {
                m_free_resource_map.set_free(handle);
            }
        }

        void* object_pool_t::access_resource(u32 handle)
        {
            ASSERT(handle < m_free_resource_index);
            if (handle != c_invalid_index)
            {
                ASSERTS(m_free_resource_map.is_used(handle), "Error: resource is not marked as being in use!");
                return &m_resource_memory[handle * m_resource_sizeof];
            }
            return nullptr;
        }

        const void* object_pool_t::access_resource(u32 handle) const
        {
            ASSERT(handle < m_free_resource_index);
            if (handle != c_invalid_index)
            {
                ASSERTS(m_free_resource_map.is_used(handle), "Error: resource is not marked as being in use!");
                return &m_resource_memory[handle * m_resource_sizeof];
            }
            return nullptr;
        }

    }  // namespace ngfx
}  // namespace ncore

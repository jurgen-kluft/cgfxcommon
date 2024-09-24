#ifndef __C_GFX_COMMON_RESOURCE_POOL_H__
#define __C_GFX_COMMON_RESOURCE_POOL_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "cbase/c_debug.h"
#include "cbase/c_hbb.h"

namespace ncore
{
    class alloc_t;

    namespace ngfx
    {
        typedef u32 handle_t;

        struct object_pool_t
        {
            void init(alloc_t* allocator, u32 max_num_resources, u32 sizeof_resource);
            void shutdown(alloc_t* allocator);

            handle_t obtain();  // Returns a handle/index to a resource.
            void     release(handle_t handle);
            void     free_all();

            void*       get_access(handle_t handle);
            const void* get_access(handle_t handle) const;

            static const handle_t c_invalid_handle = 0xffffffff;

            binmap_t m_free_resource_map;
            byte*    m_resource_memory;
            u32      m_free_resource_index;
            u32      m_num_resource_used;
            u32      m_num_resource_max;
            u32      m_resource_sizeof;
        };

        template <typename T>
        struct resource_pool_t
        {
            void init(alloc_t* allocator, u32 max_num_resources);
            void shutdown();

            handle_t obtain();
            void     release(handle_t);

            T*       get_access(handle_t handle);
            const T* get_access(handle_t handle) const;
            T*       obtain_access()
            {
                handle_t h = obtain();
                return get_access(h);
            }

        protected:
            object_pool_t m_resource_pool;
            alloc_t*      m_allocator = nullptr;
        };

        template <typename T>
        inline void resource_pool_t<T>::init(alloc_t* allocator_, u32 max_num_resources)
        {
            m_allocator = allocator_;
            m_resource_pool.init(m_allocator, max_num_resources, sizeof(T));
        }

        template <typename T>
        inline void resource_pool_t<T>::shutdown()
        {
            m_resource_pool.shutdown(m_allocator);
        }

        template <typename T>
        inline u32 resource_pool_t<T>::obtain()
        {
            return m_resource_pool.obtain();
        }

        template <typename T>
        inline void resource_pool_t<T>::release(handle_t handle)
        {
            m_resource_pool.release(handle);
        }

        template <typename T>
        inline T* resource_pool_t<T>::get_access(handle_t handle)
        {
            return (T*)m_resource_pool.get_access(handle);
        }

        template <typename T>
        inline const T* resource_pool_t<T>::get_access(handle_t handle) const
        {
            return (const T*)m_resource_pool.get_access(handle);
        }

        // A multi resource pool, where an item is of a specific resource type and the pool holds multiple resource pools.
        // We can allocate a specific resource and the index encodes the resource type so that we know which pool it belongs to.

        template <typename T>
        struct resource_type_t
        {
            static s16 s_type_index;
        };

#define DEFINE_RESOURCE(T) static resource_type_t<T> s_resource_type_##T

#define DECLARE_RESOURCE(T) \
    template <>             \
    s16 resource_type_t<T>::s_type_index = -1;

        // Pool that holds multiple resource pools
        struct resources_pool_t
        {
            void init(alloc_t* allocator, u32 max_num_resources_per_type, u16 max_num_types);
            void shutdown();

            void*       get_access(u32 handle);
            const void* get_access(u32 handle) const;

            template <typename T>
            T* get(handle_t handle)
            {
                return (T*)get_access(handle);
            }

            template <typename T>
            const T* get(handle_t handle) const
            {
                return (const T*)get_access(handle);
            }

            // Register 'resource' by type
            template <typename T>
            handle_t register_resource()
            {
                if (resource_type_t<T>::s_type_index < 0)
                {
                    if (m_num_types < m_max_types)
                    {
                        resource_type_t<T>::s_type_index = m_num_types++;
                        register_resource_pool(resource_type_t<T>::s_type_index, m_max_resources_per_type, sizeof(T));
                    }
                    else
                    {
                        return c_invalid_handle;
                    }
                }

                handle_t handle = m_types[resource_type_t<T>::s_type_index].m_resource_pool->obtain();

                // Encode in the highest 8 bit the type index
                return (handle_t)((resource_type_t<T>::s_type_index << 24) | handle);
            }

            template <typename T>
            bool is_resource_type(handle_t handle) const
            {
                return (handle >> 24) == resource_type_t<T>::s_type_index;
            }

            void release_resource(handle_t handle)
            {
                const u32      type_index = handle >> 24;
                const handle_t h          = handle & 0x00ffffff;
                ASSERT(type_index < m_num_types);
                m_types[type_index].m_resource_pool->release(h);
            }

            static const handle_t c_invalid_handle = 0xffffffff;

            struct type_t
            {
                object_pool_t* m_resource_pool;
            };

            void register_resource_pool(s16 type_index, u32 max_num_resources, u32 sizeof_resource);

            type_t*  m_types;
            alloc_t* m_allocator;
            s16      m_num_types;
            u16      m_max_types;
            u32      m_max_resources_per_type;
        };

    }  // namespace ngfx
}  // namespace ncore

#endif

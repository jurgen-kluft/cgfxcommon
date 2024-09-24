#ifndef __C_GFX_COMMON_RESOURCE_POOL_H__
#define __C_GFX_COMMON_RESOURCE_POOL_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "cbase/c_hbb.h"

namespace ncore
{
    class alloc_t;

    namespace ngfx
    {
        struct object_pool_t
        {
            void init(alloc_t* allocator, u32 max_num_resources, u32 sizeof_resource);
            void shutdown(alloc_t* allocator);

            u32  obtain_resource();  // Returns a handle/index to a resource.
            void release_resource(u32 index);
            void free_all_resources();

            void*       access_resource(u32 index);
            const void* access_resource(u32 index) const;

            static const u32 c_invalid_index = 0xffffffff;

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

            T*   obtain();
            void release(T* resource);

            T*       get(u32 resource_index);
            const T* get(u32 resource_index) const;

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
        inline T* resource_pool_t<T>::obtain()
        {
            u32 const resource_index = m_resource_pool.obtain_resource();
            if (resource_index != object_pool_t::c_invalid_index)
            {
                return (T*)m_resource_pool.access_resource(resource_index);
            }

            return nullptr;
        }

        template <typename T>
        inline void resource_pool_t<T>::release(T* resource)
        {
            u32 const resource_index = (u32)(((byte*)resource - m_resource_pool.m_resource_memory) / m_resource_pool.m_resource_sizeof);
            m_resource_pool.release_resource(resource_index);
        }

        template <typename T>
        inline T* resource_pool_t<T>::get(u32 resource_index)
        {
            return (T*)m_resource_pool.access_resource(resource_index);
        }

        template <typename T>
        inline const T* resource_pool_t<T>::get(u32 resource_index) const
        {
            return (const T*)m_resource_pool.access_resource(resource_index);
        }
    }  // namespace ngfx
}  // namespace ncore

#endif

#ifndef __C_GFX_COMMON_RESOURCE_POOL_H__
#define __C_GFX_COMMON_RESOURCE_POOL_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

#include "cbase/c_debug.h"
#include "cbase/c_hbb.h"
#include "ccore/c_allocator.h"

namespace ncore
{
    class alloc_t;

    namespace ngfx
    {
        struct handle_t
        {
            u32 index;
            u32 type;
        };

        namespace nobject
        {
            struct array_t
            {
                array_t();

                byte* m_memory;
                u32   m_sizeof;
                u32   m_num_used;
                u32   m_num_max;

                void init(alloc_t* allocator, u32 max_num_resources, u32 sizeof_resource);
                void shutdown(alloc_t* allocator);

                void*       get_access(u32 index);
                const void* get_access(u32 index) const;
            };

            // An inventory is using array_t but it has an additional bit array to mark if an item is used or free.
            struct inventory_t
            {
                inventory_t();

                void init(alloc_t* allocator, u32 max_num_resources, u32 sizeof_resource);
                void shutdown(alloc_t* allocator);

                inline void allocate(u32 index)
                {
                    ASSERT(is_free(index));
                    set_used(index);
                }

                inline void deallocate(u32 index)
                {
                    ASSERT(is_used(index));
                    set_free(index);
                }

                void free_all();

                template <typename T>
                void construct(u32 index)
                {
                    allocate(index);
                    void* ptr = get_access(index);
                    new (signature_t(), ptr) T();
                }

                template <typename T>
                void destruct(u32 index)
                {
                    deallocate(index);
                    void* ptr = get_access(index);
                    ((T*)ptr)->~T();
                }

                inline bool        is_free(u32 index) const { return (m_bitarray[index >> 5] & (1 << (index & 31))) == 0; }
                inline bool        is_used(u32 index) const { return (m_bitarray[index >> 5] & (1 << (index & 31))) != 0; }
                inline void        set_free(u32 index) { m_bitarray[index >> 5] &= ~(1 << (index & 31)); }
                inline void        set_used(u32 index) { m_bitarray[index >> 5] |= (1 << (index & 31)); }
                inline void*       get_access(u32 index) { return m_array.get_access(index); }
                inline const void* get_access(u32 index) const { return m_array.get_access(index); }

                u32*    m_bitarray;
                array_t m_array;
            };
        }  // namespace nobject

        namespace nobjects_with_resources2
        {
            // Limitations:
            // - 1024 object types
            // - 1024 resource types
            // - 256 million objects (2^28)
            struct pool_t
            {
                void init(alloc_t* allocator, u32 max_num_object_types, u32 max_num_resource_types);
                void shutdown();

                template <typename T>
                T* get_access(handle_t handle)
                {
                    return (T*)get_access(handle);
                }

                template <typename T>
                const T* get_access(handle_t handle) const
                {
                    return (const T*)get_access(handle);
                }

                template <typename T>
                handle_t allocate_object()
                {
                    return allocate_object(T::s_object_type_index);
                }

                template <typename T>
                handle_t construct_object()
                {
                    handle_t handle = allocate_object(T::s_object_type_index);
                    void*    ptr    = get_access(handle);
                    new (signature_t(), ptr) T();
                    return handle;
                }

                template <typename T>
                bool is_object(handle_t handle) const
                {
                    return is_handle_an_object(handle) && get_object_type_index(handle) == T::s_object_type_index;
                }

                void deallocate_object(handle_t handle)
                {
                    const u32 object_type_index = get_object_type_index(handle);
                    ASSERT(object_type_index < m_num_object_types);
                    const u32 object_index = get_object_index(handle);
                    m_objects[object_type_index].m_object_pool->deallocate(get_object_index(handle));
                }

                template <typename T>
                void destruct_object(handle_t handle)
                {
                    const u32 object_type_index = get_object_type_index(handle);
                    ASSERT(object_type_index < m_num_object_types);
                    const u32 object_index = get_object_index(handle);
                    void*     ptr          = m_objects[object_type_index].m_object_pool->get_access(object_index);
                    ((T*)ptr)->~T();
                    m_objects[object_type_index].m_object_pool->deallocate(object_index);
                }

                // Add a 'resource' to an 'object'
                template <typename T>
                handle_t allocate_resource(handle_t object_handle)
                {
                    const u32 resource_type_index = T::s_resource_type_index;
                    const u32 object_type_index   = get_object_type_index(object_handle);
                    const u32 object_index        = get_object_index(object_handle);
                    if (m_objects[object_type_index].m_a_resources[] != nullptr)
                    {
                        m_objects[object_type_index].m_a_resources[resource_type_index]->allocate(object_index);
                        return make_resource_handle(get_object_type_index(object_handle), resource_type_index, object_index);
                    }
                    return c_invalid_handle;
                }

                template <typename T>
                handle_t construct_resource(handle_t object_handle)
                {
                    const u32 resource_type_index = T::s_resource_type_index;
                    const u32 object_type_index   = get_object_type_index(object_handle);
                    const u32 object_index        = get_object_index(object_handle);
                    if (m_objects[object_type_index].m_a_resources[resource_type_index] != nullptr)
                    {
                        m_objects[object_type_index].m_a_resources[resource_type_index]->construct<T>(object_index);
                        return make_resource_handle(get_object_type_index(object_handle), resource_type_index, object_index);
                    }
                    return c_invalid_handle;
                }

                template <typename T>
                bool is_resource(handle_t handle) const
                {
                    return is_handle_a_resource(handle) && get_resource_type_index(handle) == T::s_resource_type_index;
                }

                void deallocate_resource(handle_t handle)
                {
                    const u32 object_type_index   = get_object_type_index(handle);
                    const u32 resource_type_index = get_resource_type_index(handle);
                    const u32 resource_index      = get_resource_index(handle);
                    ASSERT(object_type_index < m_num_object_types);
                    ASSERT(resource_type_index < m_num_resource_types);
                    m_objects[object_type_index].m_a_resources[resource_type_index]->deallocate(resource_index);
                }

                template <typename T>
                void destruct_resource(handle_t handle)
                {
                    const u32 object_type_index   = get_object_type_index(handle);
                    const u32 resource_type_index = get_resource_type_index(handle);
                    const u32 resource_index      = get_resource_index(handle);
                    ASSERT(object_type_index < m_num_object_types);
                    ASSERT(resource_type_index < m_num_resource_types);
                    m_objects[object_type_index].m_a_resources[resource_type_index]->destruct<T>(resource_index);
                }

                template <typename T>
                bool has_resource(handle_t object_handle) const
                {
                    const u16 resource_type_index = T::s_resource_type_index;
                    const u32 object_type_index   = get_object_type_index(object_handle);
                    ASSERT(object_type_index < m_num_object_types);
                    if (m_objects[object_type_index].m_a_resources[resource_type_index] != nullptr)
                        return m_objects[object_type_index].m_a_resources[resource_type_index]->is_used(get_object_index(object_handle));
                    return false;
                }

                static const handle_t c_invalid_handle;

            private:
                struct object_t
                {
                    nobject::pool_t*       m_object_pool;
                    nobject::inventory_t** m_a_resources;  // m_a_resources[m_max_resources]
                };

                const u32 c_object_type   = 1;
                const u32 c_resource_type = 3;
                const u32 c_tag_type      = 7;

                inline handle_t make_object_handle(u32 object_type_index, u32 object_index) const
                {
                    handle_t handle;
                    handle.index = object_index | (c_object_type << 28);
                    handle.type  = ((u32)object_type_index << 16) | 0xFFFF;
                    return handle;
                }

                inline handle_t make_resource_handle(u32 object_type_index, u16 resource_type_index, u16 resource_index) const
                {
                    handle_t handle;
                    handle.index = resource_index | (c_resource_type << 28);
                    handle.type  = ((u32)object_type_index << 16) | (u32)resource_type_index;
                    return handle;
                }

                inline u16  get_object_type_index(handle_t handle) const { return (handle.type >> 16) & 0xFFFF; }
                inline u16  get_resource_type_index(handle_t handle) const { return handle.type & 0xFFFF; }
                inline u32  get_object_index(handle_t handle) const { return handle.index & 0x0FFFFFFF; }
                inline u32  get_resource_index(handle_t handle) const { return handle.index & 0x0FFFFFFF; }
                inline u32  get_handle_type(handle_t handle) const { return handle.index >> 28; }
                inline bool is_handle_an_object(handle_t handle) const { return get_handle_type(handle) == c_object_type; }
                inline bool is_handle_a_resource(handle_t handle) const { return get_handle_type(handle) == c_resource_type; }

                void*       get_access(handle_t handle);
                const void* get_access(handle_t handle) const;
                void        register_object_type(u16 object_type_index, u32 max_num_objects, u32 sizeof_object, u32 max_num_resources);
                void        register_resource_type(u16 object_type_index, u16 resource_type_index, u32 sizeof_resource);
                handle_t    allocate_object(u16 object_type_index);
                handle_t    allocate_resource(handle_t object_handle, u16 resource_type_index);

                object_t* m_objects;
                alloc_t*  m_allocator;
                s16       m_num_object_types;
                u16       m_max_object_types;
                s16       m_num_resource_types;
                u16       m_max_resource_types;
            };

#define DECLARE_OBJECT_TYPE(N)         static const u16 s_object_type_index = N;
#define DEFINE_OBJECT_RESOURCE_TYPE(N) static const u16 s_resource_type_index = N;

        }  // namespace nobjects_with_resources2

    }  // namespace ngfx
}  // namespace ncore

#endif

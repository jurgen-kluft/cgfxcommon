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

            struct pool_t
            {
                pool_t();

                void init(alloc_t* allocator, u32 max_num_resources, u32 sizeof_resource);
                void shutdown(alloc_t* allocator);

                u32  allocate();  //
                void deallocate(u32 index);
                void free_all();

                template<typename T>
                u32 construct()
                {
                    const u32 index = allocate();
                    void* ptr = get_access(index);
                    new (signature_t(), ptr) T();
                    return index;
                }

                template<typename T>
                void destruct(u32 index)
                {
                    void* ptr = get_access(index);
                    ((T*)ptr)->~T();
                    deallocate(index);
                }

                void*       get_access(u32 index);
                const void* get_access(u32 index) const;

                static const u32 c_invalid_handle = 0xFFFFFFFF;

                array_t  m_object_array;
                u32      m_free_resource_index;
                binmap_t m_free_resource_map;
            };

            namespace ntyped
            {
                template <typename T>
                struct pool_t
                {
                    void init(alloc_t* allocator, u32 max_num_resources);
                    void shutdown();

                    u32  allocate();
                    void deallocate(u32 index);

                    T*       get_access(u32 index);
                    const T* get_access(u32 index) const;
                    T*       obtain_access()
                    {
                        u32 index = allocate();
                        return get_access(index);
                    }

                protected:
                    nobject::pool_t m_object_pool;
                    alloc_t*        m_allocator = nullptr;
                };

                template <typename T>
                inline void pool_t<T>::init(alloc_t* allocator_, u32 max_num_resources)
                {
                    m_allocator = allocator_;
                    m_object_pool.init(m_allocator, max_num_resources, sizeof(T));
                }

                template <typename T>
                inline void pool_t<T>::shutdown()
                {
                    m_object_pool.shutdown(m_allocator);
                }

                template <typename T>
                inline u32 pool_t<T>::allocate()
                {
                    const u32 index = m_object_pool.allocate();
                    void*     ptr   = m_object_pool.get_access(index);
                    new (signature_t(), ptr) T();
                    return index;
                }

                template <typename T>
                inline void pool_t<T>::deallocate(u32 index)
                {
                    void* ptr = m_object_pool.get_access(index);
                    ((T*)ptr)->~T();
                    m_object_pool.deallocate(index);
                }

                template <typename T>
                inline T* pool_t<T>::get_access(u32 index)
                {
                    return (T*)m_object_pool.get_access(index);
                }

                template <typename T>
                inline const T* pool_t<T>::get_access(u32 index) const
                {
                    return (const T*)m_object_pool.get_access(index);
                }

            }  // namespace ntyped
        }  // namespace nobject

        // A multi resource pool, where an item is of a specific resource type and the pool holds multiple resource pools.
        // We can allocate a specific resource and the index encodes the resource type so that we know which pool it belongs to.

        // Pool that holds multiple resource pools
        namespace nresources
        {
            struct pool_t
            {
                template <typename T>
                struct resource_type_t
                {
                    static s16 s_type_index;
                };

                void init(alloc_t* allocator, u16 max_num_types);
                void shutdown();

                template <typename T>
                T* get_access(handle_t handle)
                {
                    return (T*)get_access_raw(handle);
                }

                template <typename T>
                const T* get_access(handle_t handle) const
                {
                    return (const T*)get_access_raw(handle);
                }

                // Register 'resource' by type
                template <typename T>
                bool register_resource(u32 max_num_resources)
                {
                    if (resource_type_t<T>::s_type_index < 0)
                    {
                        if (m_num_types < m_max_types)
                        {
                            resource_type_t<T>::s_type_index = m_num_types++;
                            register_resource_pool(resource_type_t<T>::s_type_index, max_num_resources, sizeof(T));
                            return true;
                        }
                    }
                    return false;
                }

                template <typename T>
                bool is_resource_type(handle_t handle) const
                {
                    return handle.type == resource_type_t<T>::s_type_index;
                }

                template <typename T>
                handle_t allocate()
                {
                    if (resource_type_t<T>::s_type_index < 0)
                        return c_invalid_handle;

                    u32 const index = m_types[resource_type_t<T>::s_type_index].m_resource_pool->allocate();
                    void*     ptr   = m_types[resource_type_t<T>::s_type_index].m_resource_pool->get_access(index);
                    new (signature_t(), ptr) T();
                    return make_handle(resource_type_t<T>::s_type_index, index);
                }

                template <typename T>
                void deallocate(handle_t handle)
                {
                    const u32 type_index = handle.type;
                    const u32 res_index  = handle.index;
                    ASSERT(type_index == resource_type_t<T>::s_type_index);
                    void* ptr = m_types[type_index].m_resource_pool->get_access(res_index);
                    ((T*)ptr)->~T();
                    m_types[type_index].m_resource_pool->deallocate(res_index);
                }

                static const handle_t c_invalid_handle;

            private:
                struct type_t
                {
                    nobject::pool_t* m_resource_pool;
                };

                inline handle_t make_handle(u32 type_index, u32 index) const
                {
                    handle_t handle;
                    handle.index = index;
                    handle.type  = type_index;
                    return handle;
                }

                void*       get_access_raw(handle_t handle);
                const void* get_access_raw(handle_t handle) const;
                void        register_resource_pool(s16 type_index, u32 max_num_resources, u32 sizeof_resource);

                type_t*  m_types;
                alloc_t* m_allocator;
                s16      m_num_types;
                u16      m_max_types;
            };

#define DEFINE_RESOURCE_TYPE(T)                                     \
    namespace nresources                                       \
    {                                                          \
        static pool_t::resource_type_t<T> s_resource_type_##T; \
    }

#define DECLARE_RESOURCE_TYPE(T) \
    template <>             \
    s16 nresources::pool_t::resource_type_t<T>::s_type_index = -1;

        }  // namespace nresources

        namespace nobject_resources
        {
            // Limitations:
            // - 1024 object types
            // - 1024 resource types
            // - 256 million objects (2^28)

            struct pool_t
            {
                template <typename T>
                struct object_type_t
                {
                    static u16 s_type_index;
                };

                template <typename T>
                struct resource_type_t
                {
                    static u16 s_type_index;
                };

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

                // Register 'object' by type
                template <typename T>
                bool register_object_type(u32 max_instances)
                {
                    if (object_type_t<T>::s_type_index != 0xFFFF)
                        return true;

                    if (m_num_object_types < m_max_object_types)
                    {
                        object_type_t<T>::s_type_index = m_num_object_types++;
                        register_object_type(object_type_t<T>::s_type_index, max_instances, sizeof(T), m_max_resource_types);
                        return true;
                    }
                    return false;
                }

                // Register 'resource' by type
                template <typename T, typename R>
                bool register_resource_type()
                {
                    if (object_type_t<T>::s_type_index == 0xFFFF)
                        return false;
                    if (resource_type_t<R>::s_type_index != 0xFFFF)
                        return true;

                    if (m_num_resource_types < m_max_resource_types)
                    {
                        resource_type_t<R>::s_type_index = m_num_resource_types++;
                        register_resource_type(object_type_t<T>::s_type_index, resource_type_t<R>::s_type_index, sizeof(T));
                        return true;
                    }

                    return false;
                }

                template <typename T>
                handle_t allocate_object()
                {
                    if (object_type_t<T>::s_type_index == 0xFFFF)
                        return c_invalid_handle;
                    return allocate_object(object_type_t<T>::s_type_index);
                }

                template <typename T>
                handle_t construct_object()
                {
                    if (object_type_t<T>::s_type_index == 0xFFFF)
                        return c_invalid_handle;
                    handle_t handle = allocate_object(object_type_t<T>::s_type_index);
                    void*    ptr    = get_access(handle);
                    new (signature_t(), ptr) T();
                    return handle;
                }

                template <typename T>
                bool is_object(handle_t handle) const
                {
                    return is_handle_an_object(handle) && get_object_type_index(handle) == object_type_t<T>::s_type_index;
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
                    void* ptr = m_objects[object_type_index].m_object_pool->get_access(object_index);
                    ((T*)ptr)->~T();
                    m_objects[object_type_index].m_object_pool->deallocate(object_index);
                }

                // Add a 'resource' to an 'object'
                template <typename T>
                handle_t add_resource(handle_t object_handle)
                {
                    if (resource_type_t<T>::s_type_index == 0xFFFF)
                        return c_invalid_handle;
                    return allocate_resource(object_handle, resource_type_t<T>::s_type_index);
                }

                template <typename T>
                handle_t construct_resource(handle_t object_handle)
                {
                    if (resource_type_t<T>::s_type_index == 0xFFFF)
                        return c_invalid_handle;
                    handle_t handle = allocate_resource(object_handle, resource_type_t<T>::s_type_index);
                    void*    ptr    = get_access(handle);
                    new (signature_t(), ptr) T();
                    return handle;
                }

                template <typename T>
                bool is_resource(handle_t handle) const
                {
                    return is_handle_a_resource(handle) && get_resource_type_index(handle) == resource_type_t<T>::s_type_index;
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
                    void* ptr = m_objects[object_type_index].m_a_resources[resource_type_index]->get_access(resource_index);
                    ((T*)ptr)->~T();
                    m_objects[object_type_index].m_a_resources[resource_type_index]->deallocate(resource_index);
                }

                template <typename T>
                bool has_resource(handle_t object_handle) const
                {
                    const u32 object_type_index = get_object_type_index(object_handle);
                    ASSERT(object_type_index < m_num_object_types);
                    return m_objects[object_type_index].m_a_resources[resource_type_t<T>::s_type_index] != nullptr;
                }

                static const handle_t c_invalid_handle;

            private:
                struct object_t
                {
                    nobject::pool_t*  m_object_pool;
                    nobject::pool_t** m_a_resources;  // m_a_resources[m_max_resources]
                };

                const u32 c_object_type   = 1;
                const u32 c_resource_type = 3;

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

#define DEFINE_OBJECT_TYPE(T)                              \
    namespace nobject_resources                            \
    {                                                      \
        static pool_t::object_type_t<T> s_object_type_##T; \
    }

#define DECLARE_OBJECT_TYPE(T) \
    template <>                \
    u16 nobject_resources::pool_t::object_type_t<T>::s_type_index = 0xFFFF;

#define DEFINE_OBJECT_RESOURCE_TYPE(T) static nobject_resources::pool_t::resource_type_t<T> s_orp_resource_type_##T
#define DECLARE_OBJECT_RESOURCE_TYPE(T) \
    template <>                         \
    u16 nobject_resources::pool_t::resource_type_t<T>::s_type_index = 0xFFFF;

        }  // namespace nobject_resources

    }  // namespace ngfx
}  // namespace ncore

#endif

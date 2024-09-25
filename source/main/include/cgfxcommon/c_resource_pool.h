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

                u32  alloc();  //
                void dealloc(u32 index);
                void free_all();

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

                    u32  alloc();
                    void dealloc(u32 index);

                    T*       get_access(u32 index);
                    const T* get_access(u32 index) const;
                    T*       obtain_access()
                    {
                        u32 index = alloc();
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
                inline u32 pool_t<T>::alloc()
                {
                    return m_object_pool.alloc();
                }

                template <typename T>
                inline void pool_t<T>::dealloc(u32 index)
                {
                    m_object_pool.dealloc(index);
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

                void init(alloc_t* allocator, u32 max_num_resources_per_type, u16 max_num_types);
                void shutdown();

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
                bool register_resource()
                {
                    if (resource_type_t<T>::s_type_index < 0)
                    {
                        if (m_num_types < m_max_types)
                        {
                            resource_type_t<T>::s_type_index = m_num_types++;
                            register_resource_pool(resource_type_t<T>::s_type_index, m_max_resources_per_type, sizeof(T));
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
                handle_t alloc_resource()
                {
                    if (resource_type_t<T>::s_type_index < 0)
                        return c_invalid_handle;

                    handle_t handle;
                    handle.index = m_types[resource_type_t<T>::s_type_index].m_resource_pool->alloc();
                    handle.type  = resource_type_t<T>::s_type_index;
                    return handle;
                }

                void dealloc_resource(handle_t handle)
                {
                    const u32 type_index = handle.type;
                    const u32 res_index  = handle.index;
                    ASSERT(type_index < m_num_types);
                    m_types[type_index].m_resource_pool->dealloc(res_index);
                }

                static const handle_t c_invalid_handle;

            private:
                struct type_t
                {
                    nobject::pool_t* m_resource_pool;
                };

                void*       get_access(handle_t handle);
                const void* get_access(handle_t handle) const;
                void        register_resource_pool(s16 type_index, u32 max_num_resources, u32 sizeof_resource);

                type_t*  m_types;
                alloc_t* m_allocator;
                s16      m_num_types;
                u16      m_max_types;
                u32      m_max_resources_per_type;
            };
        }  // namespace nresources
#define DEFINE_RESOURCE(T)                                     \
    namespace nresources                                       \
    {                                                          \
        static pool_t::resource_type_t<T> s_resource_type_##T; \
    }

#define DECLARE_RESOURCE(T) \
    template <>             \
    s16 nresources::pool_t::resource_type_t<T>::s_type_index = -1;

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Now we need a pool of resources where a resource is a specific type and can have additional 'components' that are also resources.
        namespace nobject_resources
        {
            struct pool_t
            {
                template <typename T>
                struct object_type_t
                {
                    static s8 s_type_index;
                };

                template <typename T>
                struct resource_type_t
                {
                    static s8 s_type_index;
                };

                void init(alloc_t* allocator, u32 max_num_object_types, u32 max_num_resource_types);
                void shutdown();

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

                // Register 'object' by type
                template <typename T>
                bool register_object_type(u32 max_instances)
                {
                    if (object_type_t<T>::s_type_index < 0)
                    {
                        if (m_num_objects < m_max_objects)
                        {
                            object_type_t<T>::s_type_index = m_num_objects++;
                            register_object_type(object_type_t<T>::s_type_index, max_instances, sizeof(T), m_max_resources);
                            return true;
                        }
                    }
                    return false;
                }

                // Register 'object' by type
                template <typename T, typename R>
                bool register_resource_type(u32 max_num_resources)
                {
                    if (object_type_t<T>::s_type_index < 0)
                        return false;

                    if (resource_type_t<R>::s_type_index < 0)
                    {
                        if (m_num_resources < m_max_resources)
                        {
                            resource_type_t<R>::s_type_index = m_num_resources++;
                            register_resource_type(object_type_t<T>::s_type_index, resource_type_t<R>::s_type_index, max_num_resources, sizeof(T));
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
                handle_t alloc_object()
                {
                    if (object_type_t<T>::s_type_index < 0)
                        return c_invalid_handle;
                    return alloc_object(object_type_t<T>::s_type_index);
                }

                // Add a 'resource' to an 'object'
                template <typename T>
                handle_t add_resource(handle_t object_handle)
                {
                    if (resource_type_t<T>::s_type_index < 0)
                        return c_invalid_handle;
                    return alloc_resource(object_handle, resource_type_t<T>::s_type_index);
                }

                template <typename T>
                bool is_object(handle_t handle) const
                {
                    return handle.type == resource_type_t<T>::s_type_index;
                }

                void dealloc_resource(handle_t handle)
                {
                    const u32 type_index = handle.type;
                    const u32 res_index  = handle.index;
                    ASSERT(type_index < m_num_objects);
                    m_objects[type_index].m_object_pool->dealloc(res_index);
                }

                template <typename T>
                bool has_resource(handle_t object_handle) const
                {
                    const u32 obj_type_index     = object_handle.type;
                    const u32 obj_instance_index = object_handle.index;
                    const u32 res_type_index     = resource_type_t<T>::s_type_index;
                    ASSERT(obj_type_index < m_num_objects);
                    return m_objects[obj_type_index].m_a_resources[res_type_index] != nullptr;
                }

                static const handle_t c_invalid_handle;

            private:
                struct object_t
                {
                    nobject::pool_t*  m_object_pool;
                    nobject::pool_t** m_a_resources;  // m_a_resources[m_max_resources]
                };

                void*       get_access(handle_t handle);
                const void* get_access(handle_t handle) const;
                void        register_object_type(s16 object_index, u32 max_num_objects, u32 sizeof_object, u32 max_num_resources);
                void        register_resource_type(s16 object_index, s16 resource_index, u32 max_num_resources, u32 sizeof_resource);
                handle_t    alloc_object(s16 object_type_index);
                handle_t    alloc_resource(handle_t object_handle, s16 resource_type_index);

                object_t* m_objects;
                alloc_t*  m_allocator;
                s16       m_num_objects;
                u16       m_max_objects;
                s16       m_num_resources;
                u16       m_max_resources;
            };
        }  // namespace nobject_resources

#define DEFINE_OBJECT_TYPE(T)                              \
    namespace nobject_resources                            \
    {                                                      \
        static pool_t::object_type_t<T> s_object_type_##T; \
    }

#define DECLARE_OBJECT_TYPE(T) \
    template <>                \
    s8 nobject_resources::pool_t::object_type_t<T>::s_type_index = -1;

#define DEFINE_OBJECT_RESOURCE_TYPE(T) static nobject_resources::pool_t::resource_type_t<T> s_orp_resource_type_##T
#define DECLARE_OBJECT_RESOURCE_TYPE(T) \
    template <>                         \
    s8 nobject_resources::pool_t::resource_type_t<T>::s_type_index = -1;

    }  // namespace ngfx
}  // namespace ncore

#endif

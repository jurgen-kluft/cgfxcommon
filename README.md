# cgfx common

C++ library for next-gen graphics, the common part.

## offset allocator

This is based on the implementation of the offset allocator written by Seb Aaltonen. The original implementation can be found
[here](https://github.com/sebbbi/OffsetAllocator).

## object pool

An object pool where the objects are opaque and the pool holds an array of objects.

```c++
struct myresource_t {
    u32 data;
};

ngfx::nobject::array_t array(allocator, 10, sizeof(myresource_t));
ngfx::nobject::pool_t pool;
pool.setup(&array, allocator);
u32 index = pool.allocate();
void* resource = pool.get_access(index);
pool.deallocate(index);
pool.teardown();
```

## object pool (typed)

An object pool where the objects are typed. The implementation is using the `object pool`.

```c++
struct myresource_t {
    u32 data;
};

ngfx::nobject::ntyped::pool_t<myresource_t> pool;
pool.setup(allocator, 10);
ngfx::handle_t handle = pool.allocate();
myresource_t* resource = pool.get_access(handle);
pool.deallocate(index);
pool.teardown();
```

## resources pool (typed)

A resource pool where the resources are typed and the pool can manage multiple resources. The implementation is using the `object pool`.

```c++
struct myresource_a_t {
    u32 data;
};
DEFINE_RESOURCE_TYPE(resource_a_t); // In the header file
DECLARE_RESOURCE_TYPE(resource_a_t); // In the source file

struct myresource_b_t {
    u32 data;
};
DEFINE_RESOURCE_TYPE(resource_b_t); // In the header file
DECLARE_RESOURCE_TYPE(resource_b_t); // In the source file

ngfx::nresources::pool_t pool;
pool.setup(allocator, 10); // maximum 10 resource types
pool.register_resource<myresource_a_t>(32); // maximum 32 resources of type myresource_a_t
pool.register_resource<myresource_b_t>(32); // maximum 32 resources of type myresource_b_t

ngfx::handle_t handle_a = pool.allocate<myresource_a_t>();
ngfx::handle_t handle_b = pool.allocate<myresource_b_t>();

myresource_a_t* resource_a = pool.get_access<myresource_a_t>(handle_a);
myresource_b_t* resource_b = pool.get_access<myresource_b_t>(handle_b);

pool.deallocate<myresource_a_t>(handle_a);
pool.deallocate<myresource_b_t>(handle_b);

pool.teardown();
```


## object resources pool

An object pool where an object can have multiple resources associated with it, this is roughly similar to an Entity-Component type of design.

```c++
struct myobject_a_t {
    u32 data;
};
DEFINE_OBJECT_TYPE(object_a_t); // In the header file
DECLARE_OBJECT_TYPE(object_a_t); // In the source file

struct myresource_a_t {
    u32 data;
};
DEFINE_OBJECT_RESOURCE_TYPE(resource_a_t); // In the header file
DECLARE_OBJECT_RESOURCE_TYPE(resource_a_t); // In the source file

struct myresource_b_t {
    u32 data;
};
DEFINE_OBJECT_RESOURCE_TYPE(resource_b_t); // In the header file
DECLARE_OBJECT_RESOURCE_TYPE(resource_b_t); // In the source file

ngfx::nobject_resources::pool_t pool;
pool.setup(allocator, 10, 10); // maximum 10 object types and 10 resource types

pool.register_object_type<myobject_a_t>(32); // maximum 32 objects of type myobject_a_t
pool.register_resource_type<myobject_a_t, myresource_a_t>(); 
pool.register_resource_type<myobject_a_t, myresource_b_t>(); 

ngfx::handle_t handle_a = pool.allocate_object<myobject_a_t>();
ngfx::handle_t handle_a_resource_a = pool.allocate_resource<myresource_a_t>(handle_a);
ngfx::handle_t handle_a_resource_b = pool.allocate_resource<myresource_b_t>(handle_a);

myobject_a_t* object_a = pool.get_access<myobject_a_t>(handle_a);
myresource_a_t* resource_a = pool.get_access<myresource_a_t>(handle_a_resource_a);
myresource_b_t* resource_b = pool.get_access<myresource_b_t>(handle_a_resource_b);

pool.deallocate_resource<myresource_a_t>(handle_a_resource_a);
pool.deallocate_resource<myresource_b_t>(handle_a_resource_b);

pool.deallocate_object<myobject_a_t>(handle_a);

pool.teardown();
```

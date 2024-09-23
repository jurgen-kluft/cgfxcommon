#ifndef __C_GFX_COMMON_OFFSET_ALLOCATOR_H__
#define __C_GFX_COMMON_OFFSET_ALLOCATOR_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

namespace ncore
{
    class alloc_t;

    namespace ngfx
    {
        // 16 bit offsets mode will halve the metadata storage cost
        // But it only supports up to 65536 maximum allocation count
#ifdef USE_16_BIT_NODE_INDICES
        typedef u16 NodeIndex;
#else
        typedef u32 NodeIndex;
#endif

        static constexpr u32 NUM_TOP_BINS         = 32;
        static constexpr u32 BINS_PER_LEAF        = 8;
        static constexpr u32 TOP_BINS_INDEX_SHIFT = 3;
        static constexpr u32 LEAF_BINS_INDEX_MASK = 0x7;
        static constexpr u32 NUM_LEAF_BINS        = NUM_TOP_BINS * BINS_PER_LEAF;

        struct Allocation
        {
            static constexpr u32 NO_SPACE = 0xffffffff;

            u32       offset   = NO_SPACE;
            NodeIndex metadata = NO_SPACE;  // internal: node index
        };

        struct StorageReport
        {
            u32 totalFreeSpace;
            u32 largestFreeRegion;
        };

        struct StorageReportFull
        {
            struct Region
            {
                u32 size;
                u32 count;
            };

            Region freeRegions[NUM_LEAF_BINS];
        };

        class Allocator
        {
        public:
            Allocator(u32 size, u32 maxAllocs = 128 * 1024);
            Allocator(Allocator&& other);
            ~Allocator();
            void reset();

            Allocation allocate(u32 size);
            void       free(Allocation allocation);

            u32               allocationSize(Allocation allocation) const;
            StorageReport     storageReport() const;
            StorageReportFull storageReportFull() const;

        private:
            u32  insertNodeIntoBin(u32 size, u32 dataOffset);
            void removeNodeFromBin(u32 nodeIndex);

            struct Node
            {
                static constexpr NodeIndex unused = 0xffffffff;

                u32       dataOffset   = 0;
                u32       dataSize     = 0;
                NodeIndex binListPrev  = unused;
                NodeIndex binListNext  = unused;
                NodeIndex neighborPrev = unused;
                NodeIndex neighborNext = unused;
                bool      used         = false;  // TODO: Merge as bit flag
            };

            u32 m_size;
            u32 m_maxAllocs;
            u32 m_freeStorage;

            u32       m_usedBinsTop;
            u8        m_usedBins[NUM_TOP_BINS];
            NodeIndex m_binIndices[NUM_LEAF_BINS];

            Node*      m_nodes;
            NodeIndex* m_freeNodes;
            u32        m_freeOffset;
        };
    }  // namespace ngfx
}  // namespace ncore

#endif  // __C_GFX_COMMON_OFFSET_ALLOCATOR_H__

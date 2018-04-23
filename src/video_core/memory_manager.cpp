// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/alignment.h"
#include "common/assert.h"
#include "video_core/memory_manager.h"

namespace Tegra {

GPUVAddr MemoryManager::AllocateSpace(u64 size, u64 align) {
    boost::optional<GPUVAddr> gpu_addr = FindFreeBlock(size, align);
    ASSERT(gpu_addr);

    for (u64 offset = 0; offset < size; offset += PAGE_SIZE) {
        ASSERT(PageSlot(*gpu_addr + offset) == static_cast<u64>(PageStatus::Unmapped));
        PageSlot(*gpu_addr + offset) = static_cast<u64>(PageStatus::Allocated);
    }

    return *gpu_addr;
}

GPUVAddr MemoryManager::AllocateSpace(GPUVAddr gpu_addr, u64 size, u64 align) {
    for (u64 offset = 0; offset < size; offset += PAGE_SIZE) {
        ASSERT(PageSlot(gpu_addr + offset) == static_cast<u64>(PageStatus::Unmapped));
        PageSlot(gpu_addr + offset) = static_cast<u64>(PageStatus::Allocated);
    }

    return gpu_addr;
}

GPUVAddr MemoryManager::MapBufferEx(VAddr cpu_addr, u64 size) {
    boost::optional<GPUVAddr> gpu_addr = FindFreeBlock(size, PAGE_SIZE);
    ASSERT(gpu_addr);

    for (u64 offset = 0; offset < size; offset += PAGE_SIZE) {
        ASSERT(PageSlot(*gpu_addr + offset) == static_cast<u64>(PageStatus::Unmapped));
        PageSlot(*gpu_addr + offset) = cpu_addr + offset;
    }

    MappedRegion region{cpu_addr, *gpu_addr, size};
    mapped_regions.push_back(region);

    return *gpu_addr;
}

GPUVAddr MemoryManager::MapBufferEx(VAddr cpu_addr, GPUVAddr gpu_addr, u64 size) {
    ASSERT((gpu_addr & PAGE_MASK) == 0);

    for (u64 offset = 0; offset < size; offset += PAGE_SIZE) {
        ASSERT(PageSlot(gpu_addr + offset) == static_cast<u64>(PageStatus::Allocated));
        PageSlot(gpu_addr + offset) = cpu_addr + offset;
    }

    MappedRegion region{cpu_addr, gpu_addr, size};
    mapped_regions.push_back(region);

    return gpu_addr;
}

boost::optional<GPUVAddr> MemoryManager::FindFreeBlock(u64 size, u64 align) {
    GPUVAddr gpu_addr = 0;
    u64 free_space = 0;
    align = (align + PAGE_MASK) & ~PAGE_MASK;

    while (gpu_addr + free_space < MAX_ADDRESS) {
        if (!IsPageMapped(gpu_addr + free_space)) {
            free_space += PAGE_SIZE;
            if (free_space >= size) {
                return gpu_addr;
            }
        } else {
            gpu_addr += free_space + PAGE_SIZE;
            free_space = 0;
            gpu_addr = Common::AlignUp(gpu_addr, align);
        }
    }

    return {};
}

boost::optional<VAddr> MemoryManager::GpuToCpuAddress(GPUVAddr gpu_addr) {
    VAddr base_addr = PageSlot(gpu_addr);
    ASSERT(base_addr != static_cast<u64>(PageStatus::Unmapped));

    if (base_addr == static_cast<u64>(PageStatus::Allocated)) {
        return {};
    }

    return base_addr + (gpu_addr & PAGE_MASK);
}

std::vector<GPUVAddr> MemoryManager::CpuToGpuAddress(VAddr cpu_addr) {
    std::vector<GPUVAddr> results;
    for (const auto& region : mapped_regions) {
        if (cpu_addr >= region.cpu_addr && cpu_addr < (region.cpu_addr + region.size)) {
            u64 offset = cpu_addr - region.cpu_addr;
            results.push_back(region.gpu_addr + offset);
        }
    }
    return results;
}

bool MemoryManager::IsPageMapped(GPUVAddr gpu_addr) {
    return PageSlot(gpu_addr) != static_cast<u64>(PageStatus::Unmapped);
}

VAddr& MemoryManager::PageSlot(GPUVAddr gpu_addr) {
    auto& block = page_table[(gpu_addr >> (PAGE_BITS + PAGE_TABLE_BITS)) & PAGE_TABLE_MASK];
    if (!block) {
        block = std::make_unique<PageBlock>();
        for (unsigned index = 0; index < PAGE_BLOCK_SIZE; index++) {
            (*block)[index] = static_cast<u64>(PageStatus::Unmapped);
        }
    }
    return (*block)[(gpu_addr >> PAGE_BITS) & PAGE_BLOCK_MASK];
}

} // namespace Tegra

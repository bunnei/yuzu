// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "core/hle/kernel/memory/address_space_info.h"

namespace Kernel::Memory {

namespace {

constexpr std::size_t _1_MB{0x100000};
constexpr std::size_t _2_MB{2 * _1_MB};
constexpr std::size_t _128_MB{128 * _1_MB};
constexpr std::size_t _1_GB{0x40000000};
constexpr std::size_t _2_GB{2 * _1_GB};
constexpr std::size_t _4_GB{4 * _1_GB};
constexpr std::size_t _6_GB{6 * _1_GB};
constexpr std::size_t _64_GB{64 * _1_GB};
constexpr std::size_t _512_GB{512 * _1_GB};
constexpr u64 Invalid{std::numeric_limits<u64>::max()};

// clang-format off
constexpr AddressSpaceInfo AddressSpaceInfos[]{
   { 32 /*bit_width*/, _2_MB   /*addr*/, _1_GB   - _2_MB   /*size*/, AddressSpaceInfo::Type::Is32Bit,    },
   { 32 /*bit_width*/, _1_GB   /*addr*/, _4_GB   - _1_GB   /*size*/, AddressSpaceInfo::Type::Small64Bit, },
   { 32 /*bit_width*/, Invalid /*addr*/, _1_GB             /*size*/, AddressSpaceInfo::Type::Heap,       },
   { 32 /*bit_width*/, Invalid /*addr*/, _1_GB             /*size*/, AddressSpaceInfo::Type::Alias,      },
   { 36 /*bit_width*/, _128_MB /*addr*/, _2_GB   - _128_MB /*size*/, AddressSpaceInfo::Type::Is32Bit,    },
   { 36 /*bit_width*/, _2_GB   /*addr*/, _64_GB  - _2_GB   /*size*/, AddressSpaceInfo::Type::Small64Bit, },
   { 36 /*bit_width*/, Invalid /*addr*/, _6_GB             /*size*/, AddressSpaceInfo::Type::Heap,       },
   { 36 /*bit_width*/, Invalid /*addr*/, _6_GB             /*size*/, AddressSpaceInfo::Type::Alias,      },
   { 39 /*bit_width*/, _128_MB /*addr*/, _512_GB - _128_MB /*size*/, AddressSpaceInfo::Type::Large64Bit, },
   { 39 /*bit_width*/, Invalid /*addr*/, _64_GB            /*size*/, AddressSpaceInfo::Type::Is32Bit     },
   { 39 /*bit_width*/, Invalid /*addr*/, _6_GB             /*size*/, AddressSpaceInfo::Type::Heap,       },
   { 39 /*bit_width*/, Invalid /*addr*/, _64_GB            /*size*/, AddressSpaceInfo::Type::Alias,      },
   { 39 /*bit_width*/, Invalid /*addr*/, _2_GB             /*size*/, AddressSpaceInfo::Type::Stack,      },
};
// clang-format on

constexpr bool IsAllowedIndexForAddress(std::size_t index) {
    return index < std::size(AddressSpaceInfos) && AddressSpaceInfos[index].GetAddress() != Invalid;
}

constexpr std::size_t
    AddressSpaceIndices32Bit[static_cast<std::size_t>(AddressSpaceInfo::Type::Count)]{
        0, 1, 0, 2, 0, 3,
    };

constexpr std::size_t
    AddressSpaceIndices36Bit[static_cast<std::size_t>(AddressSpaceInfo::Type::Count)]{
        4, 5, 4, 6, 4, 7,
    };

constexpr std::size_t
    AddressSpaceIndices39Bit[static_cast<std::size_t>(AddressSpaceInfo::Type::Count)]{
        9, 8, 8, 10, 12, 11,
    };

constexpr bool IsAllowed32BitType(AddressSpaceInfo::Type type) {
    return type < AddressSpaceInfo::Type::Count && type != AddressSpaceInfo::Type::Large64Bit &&
           type != AddressSpaceInfo::Type::Stack;
}

constexpr bool IsAllowed36BitType(AddressSpaceInfo::Type type) {
    return type < AddressSpaceInfo::Type::Count && type != AddressSpaceInfo::Type::Large64Bit &&
           type != AddressSpaceInfo::Type::Stack;
}

constexpr bool IsAllowed39BitType(AddressSpaceInfo::Type type) {
    return type < AddressSpaceInfo::Type::Count && type != AddressSpaceInfo::Type::Small64Bit;
}

} // namespace

u64 AddressSpaceInfo::GetAddressSpaceStart(std::size_t width, AddressSpaceInfo::Type type) {
    const std::size_t index{static_cast<std::size_t>(type)};
    switch (width) {
    case 32:
        ASSERT(IsAllowed32BitType(type));
        ASSERT(IsAllowedIndexForAddress(AddressSpaceIndices32Bit[index]));
        return AddressSpaceInfos[AddressSpaceIndices32Bit[index]].GetAddress();
    case 36:
        ASSERT(IsAllowed36BitType(type));
        ASSERT(IsAllowedIndexForAddress(AddressSpaceIndices36Bit[index]));
        return AddressSpaceInfos[AddressSpaceIndices36Bit[index]].GetAddress();
    case 39:
        ASSERT(IsAllowed39BitType(type));
        ASSERT(IsAllowedIndexForAddress(AddressSpaceIndices39Bit[index]));
        return AddressSpaceInfos[AddressSpaceIndices39Bit[index]].GetAddress();
    }
    UNREACHABLE();
}

std::size_t AddressSpaceInfo::GetAddressSpaceSize(std::size_t width, AddressSpaceInfo::Type type) {
    const std::size_t index{static_cast<std::size_t>(type)};
    switch (width) {
    case 32:
        ASSERT(IsAllowed32BitType(type));
        return AddressSpaceInfos[AddressSpaceIndices32Bit[index]].GetSize();
    case 36:
        ASSERT(IsAllowed36BitType(type));
        return AddressSpaceInfos[AddressSpaceIndices36Bit[index]].GetSize();
    case 39:
        ASSERT(IsAllowed39BitType(type));
        return AddressSpaceInfos[AddressSpaceIndices39Bit[index]].GetSize();
    }
    UNREACHABLE();
}

} // namespace Kernel::Memory

// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

namespace Kernel::Svc {

enum MemoryState : u64 {
    MemoryState_Free = 0x00,
    MemoryState_Io = 0x01,
    MemoryState_Static = 0x02,
    MemoryState_Code = 0x03,
    MemoryState_CodeData = 0x04,
    MemoryState_Normal = 0x05,
    MemoryState_Shared = 0x06,
    MemoryState_Alias = 0x07,
    MemoryState_AliasCode = 0x08,
    MemoryState_AliasCodeData = 0x09,
    MemoryState_Ipc = 0x0A,
    MemoryState_Stack = 0x0B,
    MemoryState_ThreadLocal = 0x0C,
    MemoryState_Transfered = 0x0D,
    MemoryState_SharedTransfered = 0x0E,
    MemoryState_SharedCode = 0x0F,
    MemoryState_Inaccessible = 0x10,
    MemoryState_NonSecureIpc = 0x11,
    MemoryState_NonDeviceIpc = 0x12,
    MemoryState_Kernel = 0x13,
    MemoryState_GeneratedCode = 0x14,
    MemoryState_CodeOut = 0x15,
};

enum MemoryAttribute : u32 {
    MemoryAttribute_Locked = (1 << 0),
    MemoryAttribute_IpcLocked = (1 << 1),
    MemoryAttribute_DeviceShared = (1 << 2),
    MemoryAttribute_Uncached = (1 << 3),
};

enum MemoryPermission : u32 {
    MemoryPermission_None = (0 << 0),
    MemoryPermission_Read = (1 << 0),
    MemoryPermission_Write = (1 << 1),
    MemoryPermission_Execute = (1 << 2),
    MemoryPermission_ReadWrite = MemoryPermission_Read | MemoryPermission_Write,
    MemoryPermission_ReadExecute = MemoryPermission_Read | MemoryPermission_Execute,
    MemoryPermission_DontCare = (1 << 28),
};

struct MemoryInfo {
    u64 addr{};
    u64 size{};
    MemoryState state{};
    MemoryAttribute attr{};
    MemoryPermission perm{};
    u32 ipc_refcount{};
    u32 device_refcount{};
    u32 padding{};
};

} // namespace Kernel::Svc

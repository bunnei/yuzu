// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/config_mem.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/memory.h"
#include "core/hle/kernel/object_address_table.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/resource_limit.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/kernel/timer.h"
#include "core/hle/shared_page.h"

namespace Kernel {

unsigned int Object::next_object_id;

/// Initialize the kernel
void Init(u32 system_mode) {
    ConfigMem::Init();
    SharedPage::Init();

    Kernel::MemoryInit(system_mode);

    Kernel::ResourceLimitsInit();
    Kernel::ThreadingInit();
    Kernel::TimersInit();

    Object::next_object_id = 0;
    // TODO(Subv): Start the process ids from 10 for now, as lower PIDs are
    // reserved for low-level services
    Process::next_process_id = 10;
}

/// Shutdown the kernel
void Shutdown() {
    // Free all kernel objects
    g_handle_table.Clear();
    g_object_address_table.Clear();

    Kernel::ThreadingShutdown();

    Kernel::TimersShutdown();
    Kernel::ResourceLimitsShutdown();
    Kernel::MemoryShutdown();
}

} // namespace Kernel

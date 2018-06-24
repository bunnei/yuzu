// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include "common/common_types.h"
#include "core/hle/kernel/kernel.h"
#include "core/loader/linker.h"
#include "core/loader/loader.h"

namespace Loader {

/// Loads an NSO file
class AppLoader_NSO final : public AppLoader, Linker {
public:
    AppLoader_NSO(v_file file);

    /**
     * Returns the type of the file
     * @param file std::shared_ptr<VfsFile> open file
     * @return FileType found, or FileType::Error if this loader doesn't know it
     */
    static FileType IdentifyType(v_file file);

    FileType GetFileType() override {
        return IdentifyType(file);
    }

    static VAddr LoadModule(v_file file, VAddr load_base);

    ResultStatus Load(Kernel::SharedPtr<Kernel::Process>& process) override;
};

} // namespace Loader

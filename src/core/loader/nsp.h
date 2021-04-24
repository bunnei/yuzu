// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "common/common_types.h"
#include "core/file_sys/vfs.h"
#include "core/loader/loader.h"

namespace FileSys {
class ContentProvider;
class NACP;
class NSP;
} // namespace FileSys

namespace Service::FileSystem {
class FileSystemController;
}

namespace Loader {

class AppLoader_NCA;

/// Loads an XCI file
class AppLoader_NSP final : public AppLoader {
public:
    explicit AppLoader_NSP(FileSys::VirtualFile file,
                           const Service::FileSystem::FileSystemController& fsc,
                           const FileSys::ContentProvider& content_provider,
                           std::size_t program_index);
    ~AppLoader_NSP() override;

    /**
     * Returns the type of the file
     * @param file open file
     * @return FileType found, or FileType::Error if this loader doesn't know it
     */
    static FileType IdentifyType(const FileSys::VirtualFile& file);

    FileType GetFileType() const override {
        return IdentifyType(file);
    }

    LoadResult Load(Kernel::KProcess& process, Core::System& system) override;

    ResultStatus ReadRomFS(FileSys::VirtualFile& file) override;
    u64 ReadRomFSIVFCOffset() const override;
    ResultStatus ReadUpdateRaw(FileSys::VirtualFile& file) override;
    ResultStatus ReadProgramId(u64& out_program_id) override;
    ResultStatus ReadIcon(std::vector<u8>& buffer) override;
    ResultStatus ReadTitle(std::string& title) override;
    ResultStatus ReadControlData(FileSys::NACP& nacp) override;
    ResultStatus ReadManualRomFS(FileSys::VirtualFile& file) override;

    ResultStatus ReadBanner(std::vector<u8>& buffer) override;
    ResultStatus ReadLogo(std::vector<u8>& buffer) override;

    ResultStatus ReadNSOModules(Modules& modules) override;

private:
    std::unique_ptr<FileSys::NSP> nsp;
    std::unique_ptr<AppLoader> secondary_loader;

    FileSys::VirtualFile icon_file;
    std::unique_ptr<FileSys::NACP> nacp_file;
    u64 title_id;
};

} // namespace Loader

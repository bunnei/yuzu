// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <string>
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/file_sys/vfs_real.h"
#include "core/hle/kernel/process.h"
#include "core/loader/deconstructed_rom_directory.h"
#include "core/loader/elf.h"
#include "core/loader/nca.h"
#include "core/loader/nro.h"
#include "core/loader/nso.h"

namespace Loader {

const std::initializer_list<Kernel::AddressMapping> default_address_mappings = {
    {0x1FF50000, 0x8000, true},    // part of DSP RAM
    {0x1FF70000, 0x8000, true},    // part of DSP RAM
    {0x1F000000, 0x600000, false}, // entire VRAM
};

FileType IdentifyFile(v_file file) {
    FileType type;

#define CHECK_TYPE(loader)                                                                         \
    type = AppLoader_##loader::IdentifyType(file);                                                 \
    if (FileType::Error != type)                                                                   \
        return type;

    CHECK_TYPE(DeconstructedRomDirectory)
    CHECK_TYPE(ELF)
    CHECK_TYPE(NSO)
    CHECK_TYPE(NRO)
    CHECK_TYPE(NCA)

#undef CHECK_TYPE

    return FileType::Unknown;
}

FileType IdentifyFile(const std::string& file_name) {
    return IdentifyFile(v_file(std::make_shared<FileSys::RealVfsFile>(file_name)));
}

FileType GuessFromExtension(const std::string& extension_) {
    std::string extension = Common::ToLower(extension_);

    if (extension == "elf")
        return FileType::ELF;
    else if (extension == "nro")
        return FileType::NRO;
    else if (extension == "nso")
        return FileType::NSO;
    else if (extension == "nca")
        return FileType::NCA;

    return FileType::Unknown;
}

const char* GetFileTypeString(FileType type) {
    switch (type) {
    case FileType::ELF:
        return "ELF";
    case FileType::NRO:
        return "NRO";
    case FileType::NSO:
        return "NSO";
    case FileType::NCA:
        return "NCA";
    case FileType::DeconstructedRomDirectory:
        return "Directory";
    case FileType::Error:
    case FileType::Unknown:
        break;
    }

    return "unknown";
}

/**
 * Get a loader for a file with a specific type
 * @param file The file to load
 * @param type The type of the file
 * @param filename the file name (without path)
 * @param filepath the file full path (with name)
 * @return std::unique_ptr<AppLoader> a pointer to a loader object;  nullptr for unsupported type
 */
static std::unique_ptr<AppLoader> GetFileLoader(v_file file, FileType type) {
    switch (type) {

    // Standard ELF file format.
    case FileType::ELF:
        return std::make_unique<AppLoader_ELF>(file);

    // NX NSO file format.
    case FileType::NSO:
        return std::make_unique<AppLoader_NSO>(file);

    // NX NRO file format.
    case FileType::NRO:
        return std::make_unique<AppLoader_NRO>(file);

    // NX NCA file format.
    case FileType::NCA:
        return std::make_unique<AppLoader_NCA>(file);

    // NX deconstructed ROM directory.
    case FileType::DeconstructedRomDirectory:
        return std::make_unique<AppLoader_DeconstructedRomDirectory>(file);

    default:
        return nullptr;
    }
}

std::unique_ptr<AppLoader> GetLoader(v_file file) {
    FileType type = IdentifyFile(file);
    FileType filename_type = GuessFromExtension(file->GetExtension());

    if (type != filename_type) {
        NGLOG_WARNING(Loader, "File {} has a different type than its extension.", file->GetName());
        if (FileType::Unknown == type)
            type = filename_type;
    }

    NGLOG_DEBUG(Loader, "Loading file {} as {}...", file->GetName(), GetFileTypeString(type));

    return GetFileLoader(file, type);
}

} // namespace Loader

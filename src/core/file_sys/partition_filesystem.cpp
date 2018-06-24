// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <utility>
#include "common/file_util.h"
#include "common/logging/log.h"
#include "core/file_sys/partition_filesystem.h"
#include "core/loader/loader.h"
#include "vfs_offset.h"

namespace FileSys {

Loader::ResultStatus PartitionFilesystem::Load(std::shared_ptr<VfsFile> file) {
    // At least be as large as the header
    if (file->GetSize() < sizeof(Header))
        return Loader::ResultStatus::Error;

    file.Seek(offset, SEEK_SET);
    // For cartridges, HFSs can get very large, so we need to calculate the size up to
    // the actual content itself instead of just blindly reading in the entire file.
    Header pfs_header;
    if (!file->ReadObject(&pfs_header))
        return Loader::ResultStatus::Error;

    if (pfs_header.magic != Common::MakeMagic('H', 'F', 'S', '0') &&
        pfs_header.magic != Common::MakeMagic('P', 'F', 'S', '0')) {
        return Loader::ResultStatus::ErrorInvalidFormat;
    }

    bool is_hfs = pfs_header.magic == Common::MakeMagic('H', 'F', 'S', '0');

    size_t entry_size = is_hfs ? sizeof(HFSEntry) : sizeof(PFSEntry);
    size_t metadata_size =
        sizeof(Header) + (pfs_header.num_entries * entry_size) + pfs_header.strtab_size;

    // Actually read in now...
    std::vector<u8> file_data = file->ReadBytes(metadata_size);

    if (file_data.size() != metadata_size)
        return Loader::ResultStatus::Error;

    size_t total_size = file_data.size();
    if (total_size < sizeof(Header))
        return Loader::ResultStatus::Error;

    memcpy(&pfs_header, &file_data[offset], sizeof(Header));
    if (pfs_header.magic != Common::MakeMagic('H', 'F', 'S', '0') &&
        pfs_header.magic != Common::MakeMagic('P', 'F', 'S', '0')) {
        return Loader::ResultStatus::ErrorInvalidFormat;
    }

    is_hfs = pfs_header.magic == Common::MakeMagic('H', 'F', 'S', '0');

    size_t entries_offset = offset + sizeof(Header);
    size_t strtab_offset = entries_offset + (pfs_header.num_entries * entry_size);
    content_offset = strtab_offset + pfs_header.strtab_size;
    for (u16 i = 0; i < pfs_header.num_entries; i++) {
        FSEntry entry;

        memcpy(&entry, &file_data[entries_offset + (i * entry_size)], sizeof(FSEntry));
        std::string name(
            reinterpret_cast<const char*>(&file_data[strtab_offset + entry.strtab_offset]));

        pfs_files.emplace_back(
            std::make_shared<OffsetVfsFile>(file, entry.size, content_offset + entry.offset, name));
    }

    return Loader::ResultStatus::Success;
}

std::vector<std::shared_ptr<VfsFile>> PartitionFilesystem::GetFiles() const {
    return pfs_files;
}

std::vector<std::shared_ptr<VfsDirectory>> PartitionFilesystem::GetSubdirectories() const {
    return {};
}

bool PartitionFilesystem::IsWritable() const {
    return false;
}

bool PartitionFilesystem::IsReadable() const {
    return true;
}

std::string PartitionFilesystem::GetName() const {
    return is_hfs ? "HFS0" : "PFS0";
}

std::shared_ptr<VfsDirectory> PartitionFilesystem::GetParentDirectory() const {
    // TODO(DarkLordZach): Add support for nested containers.
    return nullptr;
}

std::shared_ptr<VfsDirectory> PartitionFilesystem::CreateSubdirectory(const std::string& name) {
    return nullptr;
}

std::shared_ptr<VfsFile> PartitionFilesystem::CreateFile(const std::string& name) {
    return nullptr;
}

bool PartitionFilesystem::DeleteSubdirectory(const std::string& name) {
    return false;
}

bool PartitionFilesystem::DeleteFile(const std::string& name) {
    return false;
}

bool PartitionFilesystem::Rename(const std::string& name) {
    return false;
}

void PartitionFilesystem::PrintDebugInfo() const {
    NGLOG_DEBUG(Service_FS, "Magic:                  {:.4}", pfs_header.magic.data());
    NGLOG_DEBUG(Service_FS, "Files:                  {}", pfs_header.num_entries);
    for (u32 i = 0; i < pfs_header.num_entries; i++) {
        NGLOG_DEBUG(Service_FS, " > File {}:              {} (0x{:X} bytes, at 0x{:X})", i,
                    pfs_files[i]->GetName(), pfs_files[i]->GetSize(),
                    dynamic_cast<OffsetVfsFile*>(pfs_files[i].get())->GetOffset());
    }
}
} // namespace FileSys

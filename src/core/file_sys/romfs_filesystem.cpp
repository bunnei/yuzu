// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include <memory>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/file_sys/romfs_filesystem.h"

namespace FileSys {

std::string RomFS_FileSystem::GetName() const {
    return "RomFS";
}

ResultVal<std::unique_ptr<StorageBackend>> RomFS_FileSystem::OpenFile(const std::string& path,
                                                                      Mode mode) const {
    return MakeResult<std::unique_ptr<StorageBackend>>(
        std::make_unique<RomFS_Storage>(romfs_file, data_offset, data_size));
}

ResultCode RomFS_FileSystem::DeleteFile(const std::string& path) const {
    NGLOG_CRITICAL(Service_FS, "Attempted to delete a file from an ROMFS archive ({}).", GetName());
    // TODO(bunnei): Use correct error code
    return ResultCode(-1);
}

ResultCode RomFS_FileSystem::RenameFile(const std::string& src_path,
                                        const std::string& dest_path) const {
    NGLOG_CRITICAL(Service_FS, "Attempted to rename a file within an ROMFS archive ({}).",
                   GetName());
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode RomFS_FileSystem::DeleteDirectory(const Path& path) const {
    NGLOG_CRITICAL(Service_FS, "Attempted to delete a directory from an ROMFS archive ({}).",
                   GetName());
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode RomFS_FileSystem::DeleteDirectoryRecursively(const Path& path) const {
    NGLOG_CRITICAL(Service_FS, "Attempted to delete a directory from an ROMFS archive ({}).",
                   GetName());
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode RomFS_FileSystem::CreateFile(const std::string& path, u64 size) const {
    NGLOG_CRITICAL(Service_FS, "Attempted to create a file in an ROMFS archive ({}).", GetName());
    // TODO(bunnei): Use correct error code
    return ResultCode(-1);
}

ResultCode RomFS_FileSystem::CreateDirectory(const std::string& path) const {
    NGLOG_CRITICAL(Service_FS, "Attempted to create a directory in an ROMFS archive ({}).",
                   GetName());
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode RomFS_FileSystem::RenameDirectory(const Path& src_path, const Path& dest_path) const {
    NGLOG_CRITICAL(Service_FS, "Attempted to rename a file within an ROMFS archive ({}).",
                   GetName());
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultVal<std::unique_ptr<DirectoryBackend>> RomFS_FileSystem::OpenDirectory(
    const std::string& path) const {
    NGLOG_WARNING(Service_FS, "Opening Directory in a ROMFS archive");
    return MakeResult<std::unique_ptr<DirectoryBackend>>(std::make_unique<ROMFSDirectory>());
}

u64 RomFS_FileSystem::GetFreeSpaceSize() const {
    NGLOG_WARNING(Service_FS, "Attempted to get the free space in an ROMFS archive");
    return 0;
}

ResultVal<FileSys::EntryType> RomFS_FileSystem::GetEntryType(const std::string& path) const {
    NGLOG_CRITICAL(Service_FS, "Called within an ROMFS archive (path {}).", path);
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultVal<size_t> RomFS_Storage::Read(const u64 offset, const size_t length, u8* buffer) const {
    NGLOG_TRACE(Service_FS, "called offset={}, length={}", offset, length);
    romfs_file->Seek(data_offset + offset, SEEK_SET);
    size_t read_length = (size_t)std::min((u64)length, data_size - offset);

    return MakeResult<size_t>(romfs_file->ReadBytes(buffer, read_length));
}

ResultVal<size_t> RomFS_Storage::Write(const u64 offset, const size_t length, const bool flush,
                                       const u8* buffer) const {
    NGLOG_ERROR(Service_FS, "Attempted to write to ROMFS file");
    // TODO(Subv): Find error code
    return MakeResult<size_t>(0);
}

u64 RomFS_Storage::GetSize() const {
    return data_size;
}

bool RomFS_Storage::SetSize(const u64 size) const {
    NGLOG_ERROR(Service_FS, "Attempted to set the size of an ROMFS file");
    return false;
}

} // namespace FileSys

// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <filesystem>
#include "common/file_util.h"
#include "core/file_sys/vfs.h"

namespace FileSys {

struct RealVfsFile : public VfsFile {
    RealVfsFile(const std::filesystem::path& name, std::filesystem::perms perms);

    std::string GetName() const override;
    size_t GetSize() const override;
    bool Resize(size_t new_size) override;
    std::shared_ptr<VfsDirectory> GetContainingDirectory() const override;
    bool IsWritable() const override;
    bool IsReadable() const override;
    size_t Read(u8* data, size_t length, size_t offset) const override;
    size_t Write(const u8* data, size_t length, size_t offset) override;
    bool Rename(const std::string& name) override;

private:
    FileUtil::IOFile backing;
    std::filesystem::path path;
    std::filesystem::perms perms;
};

struct RealVfsDirectory : public VfsDirectory {
    RealVfsDirectory(const std::filesystem::path& path, std::filesystem::perms perms);

    std::vector<std::shared_ptr<VfsFile>> GetFiles() const override;
    std::vector<std::shared_ptr<VfsDirectory>> GetSubdirectories() const override;
    bool IsWritable() const override;
    bool IsReadable() const override;
    bool IsRoot() const override;
    std::string GetName() const override;
    std::shared_ptr<VfsDirectory> GetParentDirectory() const override;
    std::shared_ptr<VfsDirectory> CreateSubdirectory(const std::string& name) override;
    std::shared_ptr<VfsFile> CreateFile(const std::string& name) override;
    bool DeleteSubdirectory(const std::string& name) override;
    bool DeleteFile(const std::string& name) override;
    bool Rename(const std::string& name) override;

private:
    std::filesystem::path path;
    std::filesystem::perms perms;
    std::vector<std::shared_ptr<VfsFile>> files;
    std::vector<std::shared_ptr<VfsDirectory>> subdirectories;
};

} // namespace FileSys

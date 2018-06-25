// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "common/common_types.h"
#include "core/file_sys/vfs.h"
#include "core/hle/result.h"

namespace FileSys {
class FileSystemBackend;
class FileSystemFactory;
class Path;
} // namespace FileSys

namespace Service {

namespace SM {
class ServiceManager;
} // namespace SM

namespace FileSystem {

/// Supported FileSystem types
enum class Type {
    RomFS = 1,
    SaveData = 2,
    SDMC = 3,
};

class VfsDirectoryServiceWrapper {
    v_dir backing;

public:
    VfsDirectoryServiceWrapper(v_dir backing);

    /**
     * Get a descriptive name for the archive (e.g. "RomFS", "SaveData", etc.)
     */
    std::string GetName() const;

    /**
     * Create a file specified by its path
     * @param path Path relative to the Archive
     * @param size The size of the new file, filled with zeroes
     * @return Result of the operation
     */
    ResultCode CreateFile(const std::string& path, u64 size) const;

    /**
     * Delete a file specified by its path
     * @param path Path relative to the archive
     * @return Result of the operation
     */
    ResultCode DeleteFile(const std::string& path) const;

    /**
     * Create a directory specified by its path
     * @param path Path relative to the archive
     * @return Result of the operation
     */
    ResultCode CreateDirectory(const std::string& path) const;

    /**
     * Delete a directory specified by its path
     * @param path Path relative to the archive
     * @return Result of the operation
     */
    ResultCode DeleteDirectory(const std::string& path) const;

    /**
     * Delete a directory specified by its path and anything under it
     * @param path Path relative to the archive
     * @return Result of the operation
     */
    ResultCode DeleteDirectoryRecursively(const std::string& path) const;

    /**
     * Rename a File specified by its path
     * @param src_path Source path relative to the archive
     * @param dest_path Destination path relative to the archive
     * @return Result of the operation
     */
    ResultCode RenameFile(const std::string& src_path, const std::string& dest_path) const;

    /**
     * Rename a Directory specified by its path
     * @param src_path Source path relative to the archive
     * @param dest_path Destination path relative to the archive
     * @return Result of the operation
     */
    ResultCode RenameDirectory(const std::string& src_path, const std::string& dest_path) const;

    /**
     * Open a file specified by its path, using the specified mode
     * @param path Path relative to the archive
     * @param mode Mode to open the file with
     * @return Opened file, or error code
     */
    ResultVal<v_file> OpenFile(const std::string& path, FileSys::Mode mode) const;

    /**
     * Open a directory specified by its path
     * @param path Path relative to the archive
     * @return Opened directory, or error code
     */
    ResultVal<v_dir> OpenDirectory(const std::string& path) const;

    /**
     * Get the free space
     * @return The number of free bytes in the archive
     */
    u64 GetFreeSpaceSize() const;

    /**
     * Get the type of the specified path
     * @return The type of the specified path or error code
     */
    ResultVal<FileSys::EntryType> GetEntryType(const std::string& path) const;
};

class VfsFileServiceWrapper {
    v_file backing;
};

/**
 * Registers a FileSystem, instances of which can later be opened using its IdCode.
 * @param factory FileSystem backend interface to use
 * @param type Type used to access this type of FileSystem
 */
ResultCode RegisterFileSystem(v_dir fs, Type type);

ResultCode RegisterRomFS(v_file fs);

/**
 * Opens a file system
 * @param type Type of the file system to open
 * @param path Path to the file system, used with Binary paths
 * @return FileSys::FileSystemBackend interface to the file system
 */
ResultVal<v_dir> OpenFileSystem(Type type);

ResultVal<v_file> OpenRomFS();

/**
 * Formats a file system
 * @param type Type of the file system to format
 * @return ResultCode of the operation
 */
ResultCode FormatFileSystem(Type type);

/// Registers all Filesystem services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace FileSystem
} // namespace Service

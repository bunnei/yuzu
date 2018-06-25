// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/file_sys/directory.h"
#include "core/file_sys/filesystem.h"
#include "core/file_sys/storage.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/hle/service/filesystem/fsp_srv.h"

namespace Service::FileSystem {

class IStorage final : public ServiceFramework<IStorage> {
public:
    IStorage(v_file backend_) : ServiceFramework("IStorage"), backend(backend_) {
        static const FunctionInfo functions[] = {
            {0, &IStorage::Read, "Read"}, {1, nullptr, "Write"},   {2, nullptr, "Flush"},
            {3, nullptr, "SetSize"},      {4, nullptr, "GetSize"}, {5, nullptr, "OperateRange"},
        };
        RegisterHandlers(functions);
    }

private:
    v_file backend;

    void Read(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const s64 offset = rp.Pop<s64>();
        const s64 length = rp.Pop<s64>();

        NGLOG_DEBUG(Service_FS, "called, offset=0x{:X}, length={}", offset, length);

        // Error checking
        if (length < 0) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultCode(ErrorModule::FS, ErrorDescription::InvalidLength));
            return;
        }
        if (offset < 0) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultCode(ErrorModule::FS, ErrorDescription::InvalidOffset));
            return;
        }

        // Read the data from the Storage backend
        std::vector<u8> output = backend->ReadBytes(length, offset);
        auto res = MakeResult<size_t>(output.size());
        if (res.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(res.Code());
            return;
        }

        // Write the data to memory
        ctx.WriteBuffer(output);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }
};

class IFile final : public ServiceFramework<IFile> {
public:
    explicit IFile(v_file backend_) : ServiceFramework("IFile"), backend(backend_) {
        static const FunctionInfo functions[] = {
            {0, &IFile::Read, "Read"},       {1, &IFile::Write, "Write"},
            {2, &IFile::Flush, "Flush"},     {3, &IFile::SetSize, "SetSize"},
            {4, &IFile::GetSize, "GetSize"}, {5, nullptr, "OperateRange"},
        };
        RegisterHandlers(functions);
    }

private:
    v_file backend;

    void Read(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 unk = rp.Pop<u64>();
        const s64 offset = rp.Pop<s64>();
        const s64 length = rp.Pop<s64>();

        NGLOG_DEBUG(Service_FS, "called, offset=0x{:X}, length={}", offset, length);

        // Error checking
        if (length < 0) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultCode(ErrorModule::FS, ErrorDescription::InvalidLength));
            return;
        }
        if (offset < 0) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultCode(ErrorModule::FS, ErrorDescription::InvalidOffset));
            return;
        }

        // Read the data from the Storage backend
        std::vector<u8> output = backend->ReadBytes(length, offset);
        auto res = MakeResult<size_t>(output.size());
        if (res.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(res.Code());
            return;
        }

        // Write the data to memory
        ctx.WriteBuffer(output);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push(static_cast<u64>(*res));
    }

    void Write(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 unk = rp.Pop<u64>();
        const s64 offset = rp.Pop<s64>();
        const s64 length = rp.Pop<s64>();

        NGLOG_DEBUG(Service_FS, "called, offset=0x{:X}, length={}", offset, length);

        // Error checking
        if (length < 0) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultCode(ErrorModule::FS, ErrorDescription::InvalidLength));
            return;
        }
        if (offset < 0) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultCode(ErrorModule::FS, ErrorDescription::InvalidOffset));
            return;
        }

        // Write the data to the Storage backend
        auto res = MakeResult<size_t>(backend->WriteBytes(ctx.ReadBuffer(), length, offset));
        if (res.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(res.Code());
            return;
        }

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    void Flush(Kernel::HLERequestContext& ctx) {
        NGLOG_DEBUG(Service_FS, "called");

        // Exists for SDK compatibiltity -- No need to flush file.

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    void SetSize(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 size = rp.Pop<u64>();
        backend->Resize(size);
        NGLOG_DEBUG(Service_FS, "called, size={}", size);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    void GetSize(Kernel::HLERequestContext& ctx) {
        const u64 size = backend->GetSize();
        NGLOG_DEBUG(Service_FS, "called, size={}", size);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u64>(size);
    }
};

class IDirectory final : public ServiceFramework<IDirectory> {
public:
    explicit IDirectory(std::unique_ptr<FileSys::DirectoryBackend>&& backend)
        : ServiceFramework("IDirectory"), backend(std::move(backend)) {
        static const FunctionInfo functions[] = {
            {0, &IDirectory::Read, "Read"},
            {1, &IDirectory::GetEntryCount, "GetEntryCount"},
        };
        RegisterHandlers(functions);
    }

private:
    std::unique_ptr<FileSys::DirectoryBackend> backend;

    void Read(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 unk = rp.Pop<u64>();

        NGLOG_DEBUG(Service_FS, "called, unk=0x{:X}", unk);

        // Calculate how many entries we can fit in the output buffer
        u64 count_entries = ctx.GetWriteBufferSize() / sizeof(FileSys::Entry);

        // Read the data from the Directory backend
        std::vector<FileSys::Entry> entries(count_entries);
        u64 read_entries = backend->Read(count_entries, entries.data());

        // Convert the data into a byte array
        std::vector<u8> output(entries.size() * sizeof(FileSys::Entry));
        std::memcpy(output.data(), entries.data(), output.size());

        // Write the data to memory
        ctx.WriteBuffer(output);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push(read_entries);
    }

    void GetEntryCount(Kernel::HLERequestContext& ctx) {
        NGLOG_DEBUG(Service_FS, "called");

        u64 count = backend->GetEntryCount();

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push(count);
    }
};

class IFileSystem final : public ServiceFramework<IFileSystem> {
public:
    explicit IFileSystem(v_dir backend)
        : ServiceFramework("IFileSystem"), backend(std::move(backend)) {
        static const FunctionInfo functions[] = {
            {0, &IFileSystem::CreateFile, "CreateFile"},
            {1, &IFileSystem::DeleteFile, "DeleteFile"},
            {2, &IFileSystem::CreateDirectory, "CreateDirectory"},
            {3, nullptr, "DeleteDirectory"},
            {4, nullptr, "DeleteDirectoryRecursively"},
            {5, &IFileSystem::RenameFile, "RenameFile"},
            {6, nullptr, "RenameDirectory"},
            {7, &IFileSystem::GetEntryType, "GetEntryType"},
            {8, &IFileSystem::OpenFile, "OpenFile"},
            {9, &IFileSystem::OpenDirectory, "OpenDirectory"},
            {10, &IFileSystem::Commit, "Commit"},
            {11, nullptr, "GetFreeSpaceSize"},
            {12, nullptr, "GetTotalSpaceSize"},
            {13, nullptr, "CleanDirectoryRecursively"},
            {14, nullptr, "GetFileTimeStampRaw"},
            {15, nullptr, "QueryEntry"},
        };
        RegisterHandlers(functions);
    }

    void CreateFile(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        auto file_buffer = ctx.ReadBuffer();
        std::string name = Common::StringFromBuffer(file_buffer);

        u64 mode = rp.Pop<u64>();
        u32 size = rp.Pop<u32>();

        NGLOG_DEBUG(Service_FS, "called file {} mode 0x{:X} size 0x{:08X}", name, mode, size);

        IPC::ResponseBuilder rb{ctx, 2};
        auto b1 = backend->CreateFile(name);
        if (b1 == nullptr) {
        }
        auto b2 = b1->Res

                      rb.Push(? RESULT_SUCCESS : ResultCode(-1));
    }

    void DeleteFile(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        auto file_buffer = ctx.ReadBuffer();
        std::string name = Common::StringFromBuffer(file_buffer);

        NGLOG_DEBUG(Service_FS, "called file {}", name);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(backend->DeleteFile(name));
    }

    void CreateDirectory(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        auto file_buffer = ctx.ReadBuffer();
        std::string name = Common::StringFromBuffer(file_buffer);

        NGLOG_DEBUG(Service_FS, "called directory {}", name);

        IPC::ResponseBuilder rb{ctx, 2};
        auto dir = backend->CreateSubdirectory(name);
        rb.Push(dir == nullptr ? -1 : 0);
    }

    void RenameFile(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        std::vector<u8> buffer;
        buffer.resize(ctx.BufferDescriptorX()[0].Size());
        Memory::ReadBlock(ctx.BufferDescriptorX()[0].Address(), buffer.data(), buffer.size());
        std::string src_name = Common::StringFromBuffer(buffer);

        buffer.resize(ctx.BufferDescriptorX()[1].Size());
        Memory::ReadBlock(ctx.BufferDescriptorX()[1].Address(), buffer.data(), buffer.size());
        std::string dst_name = Common::StringFromBuffer(buffer);

        NGLOG_DEBUG(Service_FS, "called file '{}' to file '{}'", src_name, dst_name);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(backend->RenameFile(src_name, dst_name));
    }

    void OpenFile(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        auto file_buffer = ctx.ReadBuffer();
        std::string name = Common::StringFromBuffer(file_buffer);

        auto mode = static_cast<FileSys::Mode>(rp.Pop<u32>());

        NGLOG_DEBUG(Service_FS, "called file {} mode {}", name, static_cast<u32>(mode));

        auto result = backend->OpenFile(name, mode);
        if (result.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(result.Code());
            return;
        }

        auto file = std::move(result.Unwrap());

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IFile>(std::move(file));
    }

    void OpenDirectory(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        auto file_buffer = ctx.ReadBuffer();
        std::string name = Common::StringFromBuffer(file_buffer);

        // TODO(Subv): Implement this filter.
        u32 filter_flags = rp.Pop<u32>();

        NGLOG_DEBUG(Service_FS, "called directory {} filter {}", name, filter_flags);

        auto result = backend->OpenDirectory(name);
        if (result.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(result.Code());
            return;
        }

        auto directory = std::move(result.Unwrap());

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDirectory>(std::move(directory));
    }

    void GetEntryType(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        auto file_buffer = ctx.ReadBuffer();
        std::string name = Common::StringFromBuffer(file_buffer);

        NGLOG_DEBUG(Service_FS, "called file {}", name);

        auto result = backend->GetEntryType(name);
        if (result.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(result.Code());
            return;
        }

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(static_cast<u32>(*result));
    }

    void Commit(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_FS, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

private:
    v_dir backend;
};

FSP_SRV::FSP_SRV() : ServiceFramework("fsp-srv") {
    static const FunctionInfo functions[] = {
        {0, nullptr, "MountContent"},
        {1, &FSP_SRV::Initialize, "Initialize"},
        {2, nullptr, "OpenDataFileSystemByCurrentProcess"},
        {7, nullptr, "OpenFileSystemWithPatch"},
        {8, nullptr, "OpenFileSystemWithId"},
        {9, nullptr, "OpenDataFileSystemByApplicationId"},
        {11, nullptr, "OpenBisFileSystem"},
        {12, nullptr, "OpenBisStorage"},
        {13, nullptr, "InvalidateBisCache"},
        {17, nullptr, "OpenHostFileSystem"},
        {18, &FSP_SRV::MountSdCard, "MountSdCard"},
        {19, nullptr, "FormatSdCardFileSystem"},
        {21, nullptr, "DeleteSaveDataFileSystem"},
        {22, &FSP_SRV::CreateSaveData, "CreateSaveData"},
        {23, nullptr, "CreateSaveDataFileSystemBySystemSaveDataId"},
        {24, nullptr, "RegisterSaveDataFileSystemAtomicDeletion"},
        {25, nullptr, "DeleteSaveDataFileSystemBySaveDataSpaceId"},
        {26, nullptr, "FormatSdCardDryRun"},
        {27, nullptr, "IsExFatSupported"},
        {28, nullptr, "DeleteSaveDataFileSystemBySaveDataAttribute"},
        {30, nullptr, "OpenGameCardStorage"},
        {31, nullptr, "OpenGameCardFileSystem"},
        {32, nullptr, "ExtendSaveDataFileSystem"},
        {33, nullptr, "DeleteCacheStorage"},
        {34, nullptr, "GetCacheStorageSize"},
        {51, &FSP_SRV::MountSaveData, "MountSaveData"},
        {52, nullptr, "OpenSaveDataFileSystemBySystemSaveDataId"},
        {53, nullptr, "OpenReadOnlySaveDataFileSystem"},
        {57, nullptr, "ReadSaveDataFileSystemExtraDataBySaveDataSpaceId"},
        {58, nullptr, "ReadSaveDataFileSystemExtraData"},
        {59, nullptr, "WriteSaveDataFileSystemExtraData"},
        {60, nullptr, "OpenSaveDataInfoReader"},
        {61, nullptr, "OpenSaveDataInfoReaderBySaveDataSpaceId"},
        {62, nullptr, "OpenCacheStorageList"},
        {64, nullptr, "OpenSaveDataInternalStorageFileSystem"},
        {65, nullptr, "UpdateSaveDataMacForDebug"},
        {66, nullptr, "WriteSaveDataFileSystemExtraData2"},
        {80, nullptr, "OpenSaveDataMetaFile"},
        {81, nullptr, "OpenSaveDataTransferManager"},
        {82, nullptr, "OpenSaveDataTransferManagerVersion2"},
        {100, nullptr, "OpenImageDirectoryFileSystem"},
        {110, nullptr, "OpenContentStorageFileSystem"},
        {200, &FSP_SRV::OpenDataStorageByCurrentProcess, "OpenDataStorageByCurrentProcess"},
        {201, nullptr, "OpenDataStorageByProgramId"},
        {202, nullptr, "OpenDataStorageByDataId"},
        {203, &FSP_SRV::OpenRomStorage, "OpenRomStorage"},
        {400, nullptr, "OpenDeviceOperator"},
        {500, nullptr, "OpenSdCardDetectionEventNotifier"},
        {501, nullptr, "OpenGameCardDetectionEventNotifier"},
        {510, nullptr, "OpenSystemDataUpdateEventNotifier"},
        {511, nullptr, "NotifySystemDataUpdateEvent"},
        {600, nullptr, "SetCurrentPosixTime"},
        {601, nullptr, "QuerySaveDataTotalSize"},
        {602, nullptr, "VerifySaveDataFileSystem"},
        {603, nullptr, "CorruptSaveDataFileSystem"},
        {604, nullptr, "CreatePaddingFile"},
        {605, nullptr, "DeleteAllPaddingFiles"},
        {606, nullptr, "GetRightsId"},
        {607, nullptr, "RegisterExternalKey"},
        {608, nullptr, "UnregisterAllExternalKey"},
        {609, nullptr, "GetRightsIdByPath"},
        {610, nullptr, "GetRightsIdAndKeyGenerationByPath"},
        {611, nullptr, "SetCurrentPosixTimeWithTimeDifference"},
        {612, nullptr, "GetFreeSpaceSizeForSaveData"},
        {613, nullptr, "VerifySaveDataFileSystemBySaveDataSpaceId"},
        {614, nullptr, "CorruptSaveDataFileSystemBySaveDataSpaceId"},
        {615, nullptr, "QuerySaveDataInternalStorageTotalSize"},
        {620, nullptr, "SetSdCardEncryptionSeed"},
        {630, nullptr, "SetSdCardAccessibility"},
        {631, nullptr, "IsSdCardAccessible"},
        {640, nullptr, "IsSignedSystemPartitionOnSdCardValid"},
        {700, nullptr, "OpenAccessFailureResolver"},
        {701, nullptr, "GetAccessFailureDetectionEvent"},
        {702, nullptr, "IsAccessFailureDetected"},
        {710, nullptr, "ResolveAccessFailure"},
        {720, nullptr, "AbandonAccessFailure"},
        {800, nullptr, "GetAndClearFileSystemProxyErrorInfo"},
        {1000, nullptr, "SetBisRootForHost"},
        {1001, nullptr, "SetSaveDataSize"},
        {1002, nullptr, "SetSaveDataRootPath"},
        {1003, nullptr, "DisableAutoSaveDataCreation"},
        {1004, nullptr, "SetGlobalAccessLogMode"},
        {1005, &FSP_SRV::GetGlobalAccessLogMode, "GetGlobalAccessLogMode"},
        {1006, nullptr, "OutputAccessLogToSdCard"},
        {1007, nullptr, "RegisterUpdatePartition"},
        {1008, nullptr, "OpenRegisteredUpdatePartition"},
        {1009, nullptr, "GetAndClearMemoryReportInfo"},
        {1100, nullptr, "OverrideSaveDataTransferTokenSignVerificationKey"},
    };
    RegisterHandlers(functions);
}

void FSP_SRV::TryLoadRomFS() {
    if (romfs) {
        return;
    }
    auto res = OpenRomFS();
    if (res.Succeeded()) {
        romfs = res.Unwrap();
    }
}

void FSP_SRV::Initialize(Kernel::HLERequestContext& ctx) {
    NGLOG_WARNING(Service_FS, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

void FSP_SRV::MountSdCard(Kernel::HLERequestContext& ctx) {
    NGLOG_DEBUG(Service_FS, "called");

    auto filesystem = OpenFileSystem(Type::SDMC).Unwrap();

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IFileSystem>(std::move(filesystem));
}

void FSP_SRV::CreateSaveData(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    auto save_struct = rp.PopRaw<std::array<u8, 0x40>>();
    auto save_create_struct = rp.PopRaw<std::array<u8, 0x40>>();
    u128 uid = rp.PopRaw<u128>();

    NGLOG_WARNING(Service_FS, "(STUBBED) called uid = {:016X}{:016X}", uid[1], uid[0]);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

void FSP_SRV::MountSaveData(Kernel::HLERequestContext& ctx) {
    NGLOG_WARNING(Service_FS, "(STUBBED) called");

    FileSys::Path unused;
    auto filesystem = OpenFileSystem(Type::SaveData, unused).Unwrap();

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IFileSystem>(std::move(filesystem));
}

void FSP_SRV::GetGlobalAccessLogMode(Kernel::HLERequestContext& ctx) {
    NGLOG_WARNING(Service_FS, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(5);
}

void FSP_SRV::OpenDataStorageByCurrentProcess(Kernel::HLERequestContext& ctx) {
    NGLOG_DEBUG(Service_FS, "called");

    TryLoadRomFS();
    if (!romfs) {
        // TODO (bunnei): Find the right error code to use here
        NGLOG_CRITICAL(Service_FS, "no file system interface available!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultCode(-1));
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IStorage>(std::move(storage.Unwrap()));
}

void FSP_SRV::OpenRomStorage(Kernel::HLERequestContext& ctx) {
    NGLOG_WARNING(Service_FS, "(STUBBED) called, using OpenDataStorageByCurrentProcess");
    OpenDataStorageByCurrentProcess(ctx);
}

} // namespace Service::FileSystem

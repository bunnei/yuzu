// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "partition_filesystem.h"

namespace FileSys {

enum class NCAContentType : u8 { Program = 0, Meta = 1, Control = 2, Manual = 3, Data = 4 };

struct NCASectionTableEntry {
    u32_le media_offset;
    u32_le media_end_offset;
    INSERT_PADDING_BYTES(0x8);
};
static_assert(sizeof(NCASectionTableEntry) == 0x10, "NCASectionTableEntry has incorrect size.");

struct NCAHeader {
    std::array<u8, 0x100> rsa_signature_1;
    std::array<u8, 0x100> rsa_signature_2;
    u32_le magic;
    u8 is_system;
    NCAContentType content_type;
    u8 crypto_type;
    u8 key_index;
    u64_le size;
    u64_le title_id;
    INSERT_PADDING_BYTES(0x4);
    u32_le sdk_version;
    u8 crypto_type_2;
    INSERT_PADDING_BYTES(15);
    std::array<u8, 0x10> rights_id;
    std::array<NCASectionTableEntry, 0x4> section_tables;
    std::array<std::array<u8, 0x20>, 0x4> hash_tables;
    std::array<std::array<u8, 0x10>, 0x4> key_area;
    INSERT_PADDING_BYTES(0xC0);
};
static_assert(sizeof(NCAHeader) == 0x400, "NCAHeader has incorrect size.");

static bool IsDirectoryExeFS(std::shared_ptr<FileSys::VfsDirectory> pfs) {
    // According to switchbrew, an exefs must only contain these two files:
    return pfs->GetFile("main") != nullptr && pfs->GetFile("main.npdm") != nullptr;
}

static bool IsValidNCA(const NCAHeader& header) {
    return header.magic == Common::MakeMagic('N', 'C', 'A', '2') ||
           header.magic == Common::MakeMagic('N', 'C', 'A', '3');
}

// An implementation of VfsDirectory that represents a Nintendo Content Archive (NCA) conatiner.
// After construction, use GetStatus to determine if the file is valid and ready to be used.
class NCA : public ReadOnlyVfsDirectory {
    std::vector<v_dir> dirs{};
    std::vector<v_file> files{};

    v_file romfs = nullptr;
    v_dir exefs = nullptr;
    v_file file;

    NCAHeader header{};

    Loader::ResultStatus status{};

public:
    explicit NCA(v_file file);
    Loader::ResultStatus GetStatus() const;

    std::vector<std::shared_ptr<VfsFile>> GetFiles() const override;
    std::vector<std::shared_ptr<VfsDirectory>> GetSubdirectories() const override;
    std::string GetName() const override;
    std::shared_ptr<VfsDirectory> GetParentDirectory() const override;

    NCAContentType GetType() const;
    u64 GetTitleId() const;

    v_file GetRomFS() const;
    v_dir GetExeFS() const;

protected:
    bool ReplaceFileWithSubdirectory(v_file file, v_dir dir) override;
};

} // namespace FileSys

// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/acc/acc.h"
#include "core/hle/service/acc/acc_aa.h"
#include "core/hle/service/acc/acc_su.h"
#include "core/hle/service/acc/acc_u0.h"
#include "core/hle/service/acc/acc_u1.h"

namespace Service::Account {

// TODO: RE this structure
struct UserData {
    INSERT_PADDING_WORDS(1);
    u32 icon_id;
    u8 bg_color_id;
    INSERT_PADDING_BYTES(0x7);
    INSERT_PADDING_BYTES(0x10);
    INSERT_PADDING_BYTES(0x60);
};
static_assert(sizeof(UserData) == 0x80, "UserData structure has incorrect size");

struct ProfileBase {
    u8 user_id[0x10];
    u64 timestamp;
    u8 username[0x20];
};
static_assert(sizeof(ProfileBase) == 0x38, "ProfileBase structure has incorrect size");

using Uid = std::array<u64, 2>;
static constexpr Uid DEFAULT_USER_ID{0x10ull, 0x20ull};

class IProfile final : public ServiceFramework<IProfile> {
public:
    IProfile() : ServiceFramework("IProfile") {
        static const FunctionInfo functions[] = {
            {0, nullptr, "Get"},
            {1, &IProfile::GetBase, "GetBase"},
            {10, nullptr, "GetImageSize"},
            {11, nullptr, "LoadImage"},
        };
        RegisterHandlers(functions);
    }

private:
    void GetBase(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_ACC, "(STUBBED) called");
        ProfileBase profile_base{};
        IPC::ResponseBuilder rb{ctx, 16};
        rb.Push(RESULT_SUCCESS);
        rb.PushRaw(profile_base);
    }
};

class IManagerForApplication final : public ServiceFramework<IManagerForApplication> {
public:
    IManagerForApplication() : ServiceFramework("IManagerForApplication") {
        static const FunctionInfo functions[] = {
            {0, &IManagerForApplication::CheckAvailability, "CheckAvailability"},
            {1, &IManagerForApplication::GetAccountId, "GetAccountId"},
            {2, nullptr, "EnsureIdTokenCacheAsync"},
            {3, nullptr, "LoadIdTokenCache"},
            {130, nullptr, "GetNintendoAccountUserResourceCacheForApplication"},
            {150, nullptr, "CreateAuthorizationRequest"},
            {160, nullptr, "StoreOpenContext"},
        };
        RegisterHandlers(functions);
    }

private:
    void CheckAvailability(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_ACC, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push(true); // TODO: Check when this is supposed to return true and when not
    }

    void GetAccountId(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_ACC, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u64>(0x12345678ABCDEF);
    }
};

void Module::Interface::GetUserExistence(Kernel::HLERequestContext& ctx) {
    NGLOG_WARNING(Service_ACC, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push(true); // TODO: Check when this is supposed to return true and when not
}

void Module::Interface::ListAllUsers(Kernel::HLERequestContext& ctx) {
    NGLOG_WARNING(Service_ACC, "(STUBBED) called");
    constexpr std::array<u128, 10> user_ids{DEFAULT_USER_ID};
    ctx.WriteBuffer(user_ids.data(), user_ids.size());
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

void Module::Interface::ListOpenUsers(Kernel::HLERequestContext& ctx) {
    NGLOG_WARNING(Service_ACC, "(STUBBED) called");
    constexpr std::array<u128, 10> user_ids{DEFAULT_USER_ID};
    ctx.WriteBuffer(user_ids.data(), user_ids.size());
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

void Module::Interface::GetProfile(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IProfile>();
    NGLOG_DEBUG(Service_ACC, "called");
}

void Module::Interface::InitializeApplicationInfo(Kernel::HLERequestContext& ctx) {
    NGLOG_WARNING(Service_ACC, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

void Module::Interface::GetBaasAccountManagerForApplication(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IManagerForApplication>();
    NGLOG_DEBUG(Service_ACC, "called");
}

void Module::Interface::GetLastOpenedUser(Kernel::HLERequestContext& ctx) {
    NGLOG_WARNING(Service_ACC, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 6};
    rb.Push(RESULT_SUCCESS);
    rb.PushRaw(DEFAULT_USER_ID);
}

Module::Interface::Interface(std::shared_ptr<Module> module, const char* name)
    : ServiceFramework(name), module(std::move(module)) {}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    auto module = std::make_shared<Module>();
    std::make_shared<ACC_AA>(module)->InstallAsService(service_manager);
    std::make_shared<ACC_SU>(module)->InstallAsService(service_manager);
    std::make_shared<ACC_U0>(module)->InstallAsService(service_manager);
    std::make_shared<ACC_U1>(module)->InstallAsService(service_manager);
}

} // namespace Service::Account

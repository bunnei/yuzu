// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/event.h"
#include "core/hle/service/nifm/nifm.h"
#include "core/hle/service/nifm/nifm_a.h"
#include "core/hle/service/nifm/nifm_s.h"
#include "core/hle/service/nifm/nifm_u.h"

namespace Service::NIFM {

class IScanRequest final : public ServiceFramework<IScanRequest> {
public:
    explicit IScanRequest() : ServiceFramework("IScanRequest") {
        static const FunctionInfo functions[] = {
            {0, nullptr, "Submit"},
            {1, nullptr, "IsProcessing"},
            {2, nullptr, "GetResult"},
            {3, nullptr, "GetSystemEventReadableHandle"},
        };
        RegisterHandlers(functions);
    }
};

class IRequest final : public ServiceFramework<IRequest> {
public:
    explicit IRequest() : ServiceFramework("IRequest") {
        static const FunctionInfo functions[] = {
            {0, &IRequest::GetRequestState, "GetRequestState"},
            {1, &IRequest::GetResult, "GetResult"},
            {2, &IRequest::GetSystemEventReadableHandles, "GetSystemEventReadableHandles"},
            {3, &IRequest::Cancel, "Cancel"},
            {4, nullptr, "Submit"},
            {5, nullptr, "SetRequirement"},
            {6, nullptr, "SetRequirementPreset"},
            {8, nullptr, "SetPriority"},
            {9, nullptr, "SetNetworkProfileId"},
            {10, nullptr, "SetRejectable"},
            {11, nullptr, "SetConnectionConfirmationOption"},
            {12, nullptr, "SetPersistent"},
            {13, nullptr, "SetInstant"},
            {14, nullptr, "SetSustainable"},
            {15, nullptr, "SetRawPriority"},
            {16, nullptr, "SetGreedy"},
            {17, nullptr, "SetSharable"},
            {18, nullptr, "SetRequirementByRevision"},
            {19, nullptr, "GetRequirement"},
            {20, nullptr, "GetRevision"},
            {21, nullptr, "GetAppletInfo"},
            {22, nullptr, "GetAdditionalInfo"},
            {23, nullptr, "SetKeptInSleep"},
            {24, nullptr, "RegisterSocketDescriptor"},
            {25, nullptr, "UnregisterSocketDescriptor"},
        };
        RegisterHandlers(functions);

        event1 = Kernel::Event::Create(Kernel::ResetType::OneShot, "IRequest:Event1");
        event2 = Kernel::Event::Create(Kernel::ResetType::OneShot, "IRequest:Event2");
    }

private:
    void GetRequestState(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_NIFM, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(0);
    }
    void GetResult(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_NIFM, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }
    void GetSystemEventReadableHandles(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_NIFM, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 2, 2};
        rb.Push(RESULT_SUCCESS);
        rb.PushCopyObjects(event1, event2);
    }
    void Cancel(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_NIFM, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    Kernel::SharedPtr<Kernel::Event> event1, event2;
};

class INetworkProfile final : public ServiceFramework<INetworkProfile> {
public:
    explicit INetworkProfile() : ServiceFramework("INetworkProfile") {
        static const FunctionInfo functions[] = {
            {0, nullptr, "Update"},
            {1, nullptr, "PersistOld"},
            {2, nullptr, "Persist"},
        };
        RegisterHandlers(functions);
    }
};

class IGeneralService final : public ServiceFramework<IGeneralService> {
public:
    IGeneralService();

private:
    void GetClientId(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_NIFM, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u64>(0);
    }
    void CreateScanRequest(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};

        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IScanRequest>();

        NGLOG_DEBUG(Service_NIFM, "called");
    }
    void CreateRequest(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};

        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IRequest>();

        NGLOG_DEBUG(Service_NIFM, "called");
    }
    void RemoveNetworkProfile(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_NIFM, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }
    void CreateTemporaryNetworkProfile(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};

        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<INetworkProfile>();

        NGLOG_DEBUG(Service_NIFM, "called");
    }
};

IGeneralService::IGeneralService() : ServiceFramework("IGeneralService") {
    static const FunctionInfo functions[] = {
        {1, &IGeneralService::GetClientId, "GetClientId"},
        {2, &IGeneralService::CreateScanRequest, "CreateScanRequest"},
        {4, &IGeneralService::CreateRequest, "CreateRequest"},
        {5, nullptr, "GetCurrentNetworkProfile"},
        {6, nullptr, "EnumerateNetworkInterfaces"},
        {7, nullptr, "EnumerateNetworkProfiles"},
        {8, nullptr, "GetNetworkProfile"},
        {9, nullptr, "SetNetworkProfile"},
        {10, &IGeneralService::RemoveNetworkProfile, "RemoveNetworkProfile"},
        {11, nullptr, "GetScanDataOld"},
        {12, nullptr, "GetCurrentIpAddress"},
        {13, nullptr, "GetCurrentAccessPointOld"},
        {14, &IGeneralService::CreateTemporaryNetworkProfile, "CreateTemporaryNetworkProfile"},
        {15, nullptr, "GetCurrentIpConfigInfo"},
        {16, nullptr, "SetWirelessCommunicationEnabled"},
        {17, nullptr, "IsWirelessCommunicationEnabled"},
        {18, nullptr, "GetInternetConnectionStatus"},
        {19, nullptr, "SetEthernetCommunicationEnabled"},
        {20, nullptr, "IsEthernetCommunicationEnabled"},
        {21, nullptr, "IsAnyInternetRequestAccepted"},
        {22, nullptr, "IsAnyForegroundRequestAccepted"},
        {23, nullptr, "PutToSleep"},
        {24, nullptr, "WakeUp"},
        {25, nullptr, "GetSsidListVersion"},
        {26, nullptr, "SetExclusiveClient"},
        {27, nullptr, "GetDefaultIpSetting"},
        {28, nullptr, "SetDefaultIpSetting"},
        {29, nullptr, "SetWirelessCommunicationEnabledForTest"},
        {30, nullptr, "SetEthernetCommunicationEnabledForTest"},
        {31, nullptr, "GetTelemetorySystemEventReadableHandle"},
        {32, nullptr, "GetTelemetryInfo"},
        {33, nullptr, "ConfirmSystemAvailability"},
        {34, nullptr, "SetBackgroundRequestEnabled"},
        {35, nullptr, "GetScanData"},
        {36, nullptr, "GetCurrentAccessPoint"},
        {37, nullptr, "Shutdown"},
    };
    RegisterHandlers(functions);
}

void Module::Interface::CreateGeneralServiceOld(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IGeneralService>();
    NGLOG_DEBUG(Service_NIFM, "called");
}

void Module::Interface::CreateGeneralService(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IGeneralService>();
    NGLOG_DEBUG(Service_NIFM, "called");
}

Module::Interface::Interface(std::shared_ptr<Module> module, const char* name)
    : ServiceFramework(name), module(std::move(module)) {}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    auto module = std::make_shared<Module>();
    std::make_shared<NIFM_A>(module)->InstallAsService(service_manager);
    std::make_shared<NIFM_S>(module)->InstallAsService(service_manager);
    std::make_shared<NIFM_U>(module)->InstallAsService(service_manager);
}

} // namespace Service::NIFM

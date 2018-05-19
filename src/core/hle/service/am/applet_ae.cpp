// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/am/applet_ae.h"
#include "core/hle/service/nvflinger/nvflinger.h"

namespace Service::AM {

class ILibraryAppletProxy final : public ServiceFramework<ILibraryAppletProxy> {
public:
    ILibraryAppletProxy(std::shared_ptr<NVFlinger::NVFlinger> nvflinger)
        : ServiceFramework("ILibraryAppletProxy"), nvflinger(std::move(nvflinger)) {
        static const FunctionInfo functions[] = {
            {0, &ILibraryAppletProxy::GetCommonStateGetter, "GetCommonStateGetter"},
            {1, &ILibraryAppletProxy::GetSelfController, "GetSelfController"},
            {2, &ILibraryAppletProxy::GetWindowController, "GetWindowController"},
            {3, &ILibraryAppletProxy::GetAudioController, "GetAudioController"},
            {4, &ILibraryAppletProxy::GetDisplayController, "GetDisplayController"},
            {10, &ILibraryAppletProxy::GetProcessWindingController, "GetProcessWindingController"},
            {11, &ILibraryAppletProxy::GetLibraryAppletCreator, "GetLibraryAppletCreator"},
            {20, &ILibraryAppletProxy::GetApplicationFunctions, "GetApplicationFunctions"},
            {1000, &ILibraryAppletProxy::GetDebugFunctions, "GetDebugFunctions"},
        };
        RegisterHandlers(functions);
    }

private:
    void GetCommonStateGetter(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ICommonStateGetter>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetSelfController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ISelfController>(nvflinger);
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetWindowController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IWindowController>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetAudioController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IAudioController>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetDisplayController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDisplayController>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetProcessWindingController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IProcessWindingController>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetDebugFunctions(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDebugFunctions>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetLibraryAppletCreator(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ILibraryAppletCreator>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetApplicationFunctions(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IApplicationFunctions>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    std::shared_ptr<NVFlinger::NVFlinger> nvflinger;
};

class ISystemAppletProxy final : public ServiceFramework<ISystemAppletProxy> {
public:
    explicit ISystemAppletProxy(std::shared_ptr<NVFlinger::NVFlinger> nvflinger)
        : ServiceFramework("ISystemAppletProxy"), nvflinger(std::move(nvflinger)) {
        static const FunctionInfo functions[] = {
            {0, &ISystemAppletProxy::GetCommonStateGetter, "GetCommonStateGetter"},
            {1, &ISystemAppletProxy::GetSelfController, "GetSelfController"},
            {2, &ISystemAppletProxy::GetWindowController, "GetWindowController"},
            {3, &ISystemAppletProxy::GetAudioController, "GetAudioController"},
            {4, &ISystemAppletProxy::GetDisplayController, "GetDisplayController"},
            {10, nullptr, "GetProcessWindingController"},
            {11, &ISystemAppletProxy::GetLibraryAppletCreator, "GetLibraryAppletCreator"},
            {20, &ISystemAppletProxy::GetHomeMenuFunctions, "GetHomeMenuFunctions"},
            {21, &ISystemAppletProxy::GetGlobalStateController, "GetGlobalStateController"},
            {22, &ISystemAppletProxy::GetApplicationCreator, "GetApplicationCreator"},
            {1000, &ISystemAppletProxy::GetDebugFunctions, "GetDebugFunctions"},
        };
        RegisterHandlers(functions);
    }

private:
    void GetCommonStateGetter(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ICommonStateGetter>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetSelfController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ISelfController>(nvflinger);
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetWindowController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IWindowController>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetAudioController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IAudioController>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetDisplayController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDisplayController>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetDebugFunctions(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDebugFunctions>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetLibraryAppletCreator(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ILibraryAppletCreator>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetHomeMenuFunctions(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IHomeMenuFunctions>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetGlobalStateController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IGlobalStateController>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetApplicationCreator(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IApplicationCreator>();
        NGLOG_DEBUG(Service_AM, "called");
    }
    std::shared_ptr<NVFlinger::NVFlinger> nvflinger;
};

void AppletAE::OpenSystemAppletProxy(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<ISystemAppletProxy>(nvflinger);
    NGLOG_DEBUG(Service_AM, "called");
}

void AppletAE::OpenLibraryAppletProxy(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<ILibraryAppletProxy>(nvflinger);
    NGLOG_DEBUG(Service_AM, "called");
}

void AppletAE::OpenLibraryAppletProxyOld(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<ILibraryAppletProxy>(nvflinger);
    NGLOG_DEBUG(Service_AM, "called");
}

AppletAE::AppletAE(std::shared_ptr<NVFlinger::NVFlinger> nvflinger)
    : ServiceFramework("appletAE"), nvflinger(std::move(nvflinger)) {
    static const FunctionInfo functions[] = {
        {100, &AppletAE::OpenSystemAppletProxy, "OpenSystemAppletProxy"},
        {200, &AppletAE::OpenLibraryAppletProxyOld, "OpenLibraryAppletProxyOld"},
        {201, &AppletAE::OpenLibraryAppletProxy, "OpenLibraryAppletProxy"},
        {300, nullptr, "OpenOverlayAppletProxy"},
        {350, nullptr, "OpenSystemApplicationProxy"},
        {400, nullptr, "CreateSelfLibraryAppletCreatorForDevelop"},
    };
    RegisterHandlers(functions);
}

} // namespace Service::AM

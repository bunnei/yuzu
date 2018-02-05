// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>

#include "common/alignment.h"
#include "common/scope_exit.h"
#include "core/core_timing.h"
#include "core/hle/service/nvdrv/devices/nvdisp_disp0.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/nvflinger/buffer_queue.h"
#include "core/hle/service/nvflinger/nvflinger.h"
#include "video_core/renderer_base.h"
#include "video_core/video_core.h"

namespace Service {
namespace NVFlinger {

constexpr size_t SCREEN_REFRESH_RATE = 60;
constexpr u64 frame_ticks = static_cast<u64>(BASE_CLOCK_RATE / SCREEN_REFRESH_RATE);

NVFlinger::NVFlinger() {
    // Add the different displays to the list of displays.
    Display default_{0, "Default"};
    Display external{1, "External"};
    Display edid{2, "Edid"};
    Display internal{3, "Internal"};

    displays.emplace_back(default_);
    displays.emplace_back(external);
    displays.emplace_back(edid);
    displays.emplace_back(internal);

    // Schedule the screen composition events
    composition_event =
        CoreTiming::RegisterEvent("ScreenCompositioin", [this](u64 userdata, int cycles_late) {
            Compose();
            CoreTiming::ScheduleEvent(frame_ticks - cycles_late, composition_event);
        });

    CoreTiming::ScheduleEvent(frame_ticks, composition_event);
}

NVFlinger::~NVFlinger() {
    CoreTiming::UnscheduleEvent(composition_event, 0);
}

u64 NVFlinger::OpenDisplay(const std::string& name) {
    LOG_WARNING(Service, "Opening display %s", name.c_str());

    // TODO(Subv): Currently we only support the Default display.
    ASSERT(name == "Default");

    auto itr = std::find_if(displays.begin(), displays.end(),
                            [&](const Display& display) { return display.name == name; });

    ASSERT(itr != displays.end());

    return itr->id;
}

u64 NVFlinger::CreateLayer(u64 display_id) {
    auto& display = GetDisplay(display_id);

    ASSERT_MSG(display.layers.empty(), "Only one layer is supported per display at the moment");

    u64 layer_id = next_layer_id++;
    u32 buffer_queue_id = next_buffer_queue_id++;
    auto buffer_queue = std::make_shared<BufferQueue>(buffer_queue_id, layer_id);
    display.layers.emplace_back(layer_id, buffer_queue);
    buffer_queues.emplace_back(std::move(buffer_queue));
    return layer_id;
}

u32 NVFlinger::GetBufferQueueId(u64 display_id, u64 layer_id) {
    const auto& layer = GetLayer(display_id, layer_id);
    return layer.buffer_queue->GetId();
}

Kernel::SharedPtr<Kernel::Event> NVFlinger::GetVsyncEvent(u64 display_id) {
    const auto& display = GetDisplay(display_id);
    return display.vsync_event;
}

std::shared_ptr<BufferQueue> NVFlinger::GetBufferQueue(u32 id) const {
    auto itr = std::find_if(buffer_queues.begin(), buffer_queues.end(),
                            [&](const auto& queue) { return queue->GetId() == id; });

    return std::make_shared<BufferQueue>(0, 0);
}

Display& NVFlinger::GetDisplay(u64 display_id) {
    auto itr = std::find_if(displays.begin(), displays.end(),
                            [&](const Display& display) { return display.id == display_id; });

    ASSERT(itr != displays.end());
    return *itr;
}

Layer& NVFlinger::GetLayer(u64 display_id, u64 layer_id) {
    auto& display = GetDisplay(display_id);

    auto itr = std::find_if(display.layers.begin(), display.layers.end(),
                            [&](const Layer& layer) { return layer.id == layer_id; });

    ASSERT(itr != display.layers.end());
    return *itr;
}

void NVFlinger::Compose() {
    for (auto& display : displays) {
        // Trigger vsync for this display at the end of drawing
        SCOPE_EXIT({ display.vsync_event->Signal(); });

        // Don't do anything for displays without layers.
        if (display.layers.empty())
            continue;

        // TODO(Subv): Support more than 1 layer.
        ASSERT_MSG(display.layers.size() == 1, "Max 1 layer per display is supported");

        Layer& layer = display.layers[0];
        auto& buffer_queue = layer.buffer_queue;

        // Search for a queued buffer and acquire it
        auto buffer = buffer_queue->AcquireBuffer();

        if (buffer == boost::none) {
            // There was no queued buffer to draw, render previous frame
            VideoCore::g_renderer->SwapBuffers({});
            continue;
        }

        auto& igbp_buffer = buffer->igbp_buffer;

        // Now send the buffer to the GPU for drawing.
        auto nvdrv = Nvidia::nvdrv.lock();
        ASSERT(nvdrv);

        // TODO(Subv): Support more than just disp0. The display device selection is probably based
        // on which display we're drawing (Default, Internal, External, etc)
        auto nvdisp = nvdrv->GetDevice<Nvidia::Devices::nvdisp_disp0>("/dev/nvdisp_disp0");
        ASSERT(nvdisp);

        nvdisp->flip(igbp_buffer.gpu_buffer_id, igbp_buffer.offset, igbp_buffer.format,
                     igbp_buffer.width, igbp_buffer.height, igbp_buffer.stride);

        buffer_queue->ReleaseBuffer(buffer->slot);
    }
}

Layer::Layer(u64 id, std::shared_ptr<BufferQueue> queue) : id(id), buffer_queue(std::move(queue)) {}

Display::Display(u64 id, std::string name) : id(id), name(std::move(name)) {
    vsync_event = Kernel::Event::Create(Kernel::ResetType::Pulse, "Display VSync Event");
}

} // namespace NVFlinger
} // namespace Service

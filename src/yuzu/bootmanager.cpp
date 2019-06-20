// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma optimize("", off)

#include <QApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QOffscreenSurface>
#include <QOpenGLWindow>
#include <QPainter>
#include <QScreen>
#include <QWindow>
#include <common/swap.h>
#include <fmt/format.h>
#include "common/microprofile.h"
#include "common/scm_rev.h"
#include "core/arm/arm_interface.h"
#include "core/core.h"
#include "core/frontend/framebuffer_layout.h"
#include "core/hle/kernel/process.h"
#include "core/memory.h"
#include "core/settings.h"
#include "input_common/keyboard.h"
#include "input_common/main.h"
#include "input_common/motion_emu.h"
#include "video_core/renderer_base.h"
#include "video_core/video_core.h"
#include "yuzu/bootmanager.h"
#include "yuzu/main.h"

EmuThread::EmuThread(GRenderWindow* render_window) : render_window(render_window) {}

void EmuThread::run() {
    render_window->MakeCurrent();

    MicroProfileOnThreadCreate("EmuThread");

    emit LoadProgress(VideoCore::LoadCallbackStage::Prepare, 0, 0);

    Core::System::GetInstance().Renderer().Rasterizer().LoadDiskResources(
        stop_run, [this](VideoCore::LoadCallbackStage stage, std::size_t value, std::size_t total) {
            emit LoadProgress(stage, value, total);
        });

    emit LoadProgress(VideoCore::LoadCallbackStage::Complete, 0, 0);

    if (Settings::values.use_asynchronous_gpu_emulation) {
        // Release OpenGL context for the GPU thread
        render_window->DoneCurrent();
    }

    // Holds whether the cpu was running during the last iteration,
    // so that the DebugModeLeft signal can be emitted before the
    // next execution step
    bool was_active = false;
    while (!stop_run) {
        if (running) {
            if (!was_active)
                emit DebugModeLeft();

            Core::System::ResultStatus result = Core::System::GetInstance().RunLoop();
            if (result != Core::System::ResultStatus::Success) {
                this->SetRunning(false);
                emit ErrorThrown(result, Core::System::GetInstance().GetStatusDetails());
            }

            was_active = running || exec_step;
            if (!was_active && !stop_run)
                emit DebugModeEntered();
        } else if (exec_step) {
            if (!was_active)
                emit DebugModeLeft();

            exec_step = false;
            Core::System::GetInstance().SingleStep();
            emit DebugModeEntered();
            yieldCurrentThread();

            was_active = false;
        } else {
            std::unique_lock lock{running_mutex};
            running_cv.wait(lock, [this] { return IsRunning() || exec_step || stop_run; });
        }
    }

    // Shutdown the core emulation
    Core::System::GetInstance().Shutdown();

#if MICROPROFILE_ENABLED
    MicroProfileOnThreadExit();
#endif

    render_window->moveContext();
}

class GGLContext : public Core::Frontend::GraphicsContext {
public:
    explicit GGLContext(QOpenGLContext* shared_context) : shared_context{shared_context} {
        context.setFormat(shared_context->format());
        context.setShareContext(shared_context);
        context.create();
    }

    void MakeCurrent() override {
        context.makeCurrent(shared_context->surface());
    }

    void DoneCurrent() override {
        context.doneCurrent();
    }

    void SwapBuffers() override {}

private:
    QOpenGLContext* shared_context;
    QOpenGLContext context;
};

// This class overrides paintEvent and resizeEvent to prevent the GUI thread from stealing GL
// context.
// The corresponding functionality is handled in EmuThread instead
class GGLWidgetInternal : public QOpenGLWindow {
public:
    GGLWidgetInternal(GRenderWindow* parent, QOpenGLContext* shared_context)
        : QOpenGLWindow(shared_context), parent(parent) {}

    void paintEvent(QPaintEvent* ev) override {
        if (do_painting) {
            QPainter painter(this);
        }
    }

    void resizeEvent(QResizeEvent* ev) override {
        parent->OnClientAreaResized(ev->size().width(), ev->size().height());
        parent->OnFramebufferSizeChanged();
    }

    void keyPressEvent(QKeyEvent* event) override {
        InputCommon::GetKeyboard()->PressKey(event->key());
    }

    void keyReleaseEvent(QKeyEvent* event) override {
        InputCommon::GetKeyboard()->ReleaseKey(event->key());
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->source() == Qt::MouseEventSynthesizedBySystem)
            return; // touch input is handled in TouchBeginEvent

        const auto pos{event->pos()};
        if (event->button() == Qt::LeftButton) {
            const auto [x, y] = parent->ScaleTouch(pos);
            parent->TouchPressed(x, y);
        } else if (event->button() == Qt::RightButton) {
            InputCommon::GetMotionEmu()->BeginTilt(pos.x(), pos.y());
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (event->source() == Qt::MouseEventSynthesizedBySystem)
            return; // touch input is handled in TouchUpdateEvent

        const auto pos{event->pos()};
        const auto [x, y] = parent->ScaleTouch(pos);
        parent->TouchMoved(x, y);
        InputCommon::GetMotionEmu()->Tilt(pos.x(), pos.y());
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->source() == Qt::MouseEventSynthesizedBySystem)
            return; // touch input is handled in TouchEndEvent

        if (event->button() == Qt::LeftButton)
            parent->TouchReleased();
        else if (event->button() == Qt::RightButton)
            InputCommon::GetMotionEmu()->EndTilt();
    }

    void DisablePainting() {
        do_painting = false;
    }

    void EnablePainting() {
        do_painting = true;
    }

private:
    GRenderWindow* parent;
    bool do_painting;
};

GRenderWindow::GRenderWindow(QWidget* parent, EmuThread* emu_thread)
    : QWidget(parent), emu_thread(emu_thread) {
    setWindowTitle(QStringLiteral("yuzu %1 | %2-%3")
                       .arg(QString::fromUtf8(Common::g_build_name),
                            QString::fromUtf8(Common::g_scm_branch),
                            QString::fromUtf8(Common::g_scm_desc)));
    setAttribute(Qt::WA_AcceptTouchEvents);

    InputCommon::Init();
    connect(this, &GRenderWindow::FirstFrameDisplayed, static_cast<GMainWindow*>(parent),
            &GMainWindow::OnLoadComplete);
}

GRenderWindow::~GRenderWindow() {
    InputCommon::Shutdown();
}

void GRenderWindow::moveContext() {
    DoneCurrent();

    // If the thread started running, move the GL Context to the new thread. Otherwise, move it
    // back.
    auto thread = (QThread::currentThread() == qApp->thread() && emu_thread != nullptr)
                      ? emu_thread
                      : qApp->thread();
    context->moveToThread(thread);
}
//
// std::vector<u8> backing_memory;
// void MapHookMemory() {
//    auto process = Core::System::GetInstance().CurrentProcess();
//
//    backing_memory.resize(0x40000);
//
//    process->VMManager().MapBackingMemory(0x100000, backing_memory.data(), backing_memory.size(),
//                                          Kernel::MemoryState::ModuleCode);
//}
//
// void WriteHook(VAddr patch_addr, VAddr hook_addr) {
//    auto process = Core::System::GetInstance().CurrentProcess();
//
//    auto vma = process->VMManager().FindVMA(patch_addr);
//    auto& vma2 = vma->second;
//
//    Memory::Write32(hook_addr + 0, 0xd4001001); // 01 10 00 D4  | SVC #0xFF
//    Memory::Write32(hook_addr + 4, 0xd65f03c0); // C0 03 5F D6  | RET
//    Memory::Write32(patch_addr, hook_addr);
//
//    Core::System::GetInstance().ArmInterface(0).ClearInstructionCache();
//    Core::System::GetInstance().ArmInterface(1).ClearInstructionCache();
//    Core::System::GetInstance().ArmInterface(2).ClearInstructionCache();
//    Core::System::GetInstance().ArmInterface(3).ClearInstructionCache();
//}

void GRenderWindow::SwapBuffers() {
    // In our multi-threaded QWidget use case we shouldn't need to call `makeCurrent`,
    // since we never call `doneCurrent` in this thread.
    // However:
    // - The Qt debug runtime prints a bogus warning on the console if `makeCurrent` wasn't called
    // since the last time `swapBuffers` was executed;
    // - On macOS, if `makeCurrent` isn't called explicitly, resizing the buffer breaks.
    context->makeCurrent(child);

    context->swapBuffers(child);
    if (!first_frame) {
        // Memory::Wr

        emit FirstFrameDisplayed();
        first_frame = true;

        /*MapHookMemory();
        WriteHook(0x0000000008005000 + 0x0000000001e4d700, 0x100000);*/

        // Core::System::GetInstance().CurrentArmInterface().WriteHleHooks();

        // u32 value0 = Common::swap32(Memory::Read32(0x0000000008005000 + 0x0000000001e4d700 + 0));
        // u32 value1 = Common::swap32(Memory::Read32(0x0000000008005000 + 0x0000000001e4d700 + 4));
        // u32 value2 = Common::swap32(Memory::Read32(0x0000000008005000 + 0x0000000001e4d700 + 8));

        //        Memory::Write32(0x0000000008005000 + 0x0000000001e4d700,
        //                      /*0xE11F00d4*/ 0xdeadbeef);
        //// Memory::Write32(0x0000000008005000 + 0x0000000001e4d704, /*0xE11F00d4*/ 0xbadc0de);

        // E11F00d4
    }
}

void GRenderWindow::MakeCurrent() {
    context->makeCurrent(child);
}

void GRenderWindow::DoneCurrent() {
    context->doneCurrent();
}

void GRenderWindow::PollEvents() {}

// On Qt 5.0+, this correctly gets the size of the framebuffer (pixels).
//
// Older versions get the window size (density independent pixels),
// and hence, do not support DPI scaling ("retina" displays).
// The result will be a viewport that is smaller than the extent of the window.
void GRenderWindow::OnFramebufferSizeChanged() {
    // Screen changes potentially incur a change in screen DPI, hence we should update the
    // framebuffer size
    qreal pixelRatio = GetWindowPixelRatio();
    unsigned width = child->QPaintDevice::width() * pixelRatio;
    unsigned height = child->QPaintDevice::height() * pixelRatio;
    UpdateCurrentFramebufferLayout(width, height);
}

void GRenderWindow::ForwardKeyPressEvent(QKeyEvent* event) {
    if (child) {
        child->keyPressEvent(event);
    }
}

void GRenderWindow::ForwardKeyReleaseEvent(QKeyEvent* event) {
    if (child) {
        child->keyReleaseEvent(event);
    }
}

void GRenderWindow::BackupGeometry() {
    geometry = ((QWidget*)this)->saveGeometry();
}

void GRenderWindow::RestoreGeometry() {
    // We don't want to back up the geometry here (obviously)
    QWidget::restoreGeometry(geometry);
}

void GRenderWindow::restoreGeometry(const QByteArray& geometry) {
    // Make sure users of this class don't need to deal with backing up the geometry themselves
    QWidget::restoreGeometry(geometry);
    BackupGeometry();
}

QByteArray GRenderWindow::saveGeometry() {
    // If we are a top-level widget, store the current geometry
    // otherwise, store the last backup
    if (parent() == nullptr)
        return ((QWidget*)this)->saveGeometry();
    else
        return geometry;
}

qreal GRenderWindow::GetWindowPixelRatio() const {
    // windowHandle() might not be accessible until the window is displayed to screen.
    return windowHandle() ? windowHandle()->screen()->devicePixelRatio() : 1.0f;
}

std::pair<unsigned, unsigned> GRenderWindow::ScaleTouch(const QPointF pos) const {
    const qreal pixel_ratio = GetWindowPixelRatio();
    return {static_cast<unsigned>(std::max(std::round(pos.x() * pixel_ratio), qreal{0.0})),
            static_cast<unsigned>(std::max(std::round(pos.y() * pixel_ratio), qreal{0.0}))};
}

void GRenderWindow::closeEvent(QCloseEvent* event) {
    emit Closed();
    QWidget::closeEvent(event);
}

void GRenderWindow::TouchBeginEvent(const QTouchEvent* event) {
    // TouchBegin always has exactly one touch point, so take the .first()
    const auto [x, y] = ScaleTouch(event->touchPoints().first().pos());
    this->TouchPressed(x, y);
}

void GRenderWindow::TouchUpdateEvent(const QTouchEvent* event) {
    QPointF pos;
    int active_points = 0;

    // average all active touch points
    for (const auto tp : event->touchPoints()) {
        if (tp.state() & (Qt::TouchPointPressed | Qt::TouchPointMoved | Qt::TouchPointStationary)) {
            active_points++;
            pos += tp.pos();
        }
    }

    pos /= active_points;

    const auto [x, y] = ScaleTouch(pos);
    this->TouchMoved(x, y);
}

void GRenderWindow::TouchEndEvent() {
    this->TouchReleased();
}

bool GRenderWindow::event(QEvent* event) {
    if (event->type() == QEvent::TouchBegin) {
        TouchBeginEvent(static_cast<QTouchEvent*>(event));
        return true;
    } else if (event->type() == QEvent::TouchUpdate) {
        TouchUpdateEvent(static_cast<QTouchEvent*>(event));
        return true;
    } else if (event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel) {
        TouchEndEvent();
        return true;
    }

    return QWidget::event(event);
}

void GRenderWindow::focusOutEvent(QFocusEvent* event) {
    QWidget::focusOutEvent(event);
    InputCommon::GetKeyboard()->ReleaseAllKeys();
}

void GRenderWindow::OnClientAreaResized(unsigned width, unsigned height) {
    NotifyClientAreaSizeChanged(std::make_pair(width, height));
}

std::unique_ptr<Core::Frontend::GraphicsContext> GRenderWindow::CreateSharedContext() const {
    return std::make_unique<GGLContext>(context.get());
}

void GRenderWindow::InitRenderTarget() {
    shared_context.reset();
    context.reset();

    delete child;
    child = nullptr;

    delete container;
    container = nullptr;

    delete layout();

    first_frame = false;

    // TODO: One of these flags might be interesting: WA_OpaquePaintEvent, WA_NoBackground,
    // WA_DontShowOnScreen, WA_DeleteOnClose
    QSurfaceFormat fmt;
    fmt.setVersion(4, 3);
    if (Settings::values.use_compatibility_profile) {
        fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
        fmt.setOption(QSurfaceFormat::FormatOption::DeprecatedFunctions);
    } else {
        fmt.setProfile(QSurfaceFormat::CoreProfile);
    }
    // TODO: expose a setting for buffer value (ie default/single/double/triple)
    fmt.setSwapBehavior(QSurfaceFormat::DefaultSwapBehavior);
    shared_context = std::make_unique<QOpenGLContext>();
    shared_context->setFormat(fmt);
    shared_context->create();
    context = std::make_unique<QOpenGLContext>();
    context->setShareContext(shared_context.get());
    context->setFormat(fmt);
    context->create();
    fmt.setSwapInterval(false);

    child = new GGLWidgetInternal(this, shared_context.get());
    container = QWidget::createWindowContainer(child, this);

    QBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(container);
    layout->setMargin(0);
    setLayout(layout);

    // Reset minimum size to avoid unwanted resizes when this function is called for a second time.
    setMinimumSize(1, 1);

    // Show causes the window to actually be created and the OpenGL context as well, but we don't
    // want the widget to be shown yet, so immediately hide it.
    show();
    hide();

    resize(Layout::ScreenUndocked::Width, Layout::ScreenUndocked::Height);
    child->resize(Layout::ScreenUndocked::Width, Layout::ScreenUndocked::Height);
    container->resize(Layout::ScreenUndocked::Width, Layout::ScreenUndocked::Height);

    OnMinimalClientAreaChangeRequest(GetActiveConfig().min_client_area_size);

    OnFramebufferSizeChanged();
    NotifyClientAreaSizeChanged(std::pair<unsigned, unsigned>(child->width(), child->height()));

    BackupGeometry();
}

void GRenderWindow::CaptureScreenshot(u16 res_scale, const QString& screenshot_path) {
    auto& renderer = Core::System::GetInstance().Renderer();

    if (!res_scale)
        res_scale = VideoCore::GetResolutionScaleFactor(renderer);

    const Layout::FramebufferLayout layout{Layout::FrameLayoutFromResolutionScale(res_scale)};
    screenshot_image = QImage(QSize(layout.width, layout.height), QImage::Format_RGB32);
    renderer.RequestScreenshot(
        screenshot_image.bits(),
        [=] {
            screenshot_image.mirrored(false, true).save(screenshot_path);
            LOG_INFO(Frontend, "The screenshot is saved.");
        },
        layout);
}

void GRenderWindow::OnMinimalClientAreaChangeRequest(std::pair<unsigned, unsigned> minimal_size) {
    setMinimumSize(minimal_size.first, minimal_size.second);
}

void GRenderWindow::OnEmulationStarting(EmuThread* emu_thread) {
    this->emu_thread = emu_thread;
    child->DisablePainting();
}

void GRenderWindow::OnEmulationStopping() {
    emu_thread = nullptr;
    child->EnablePainting();
}

void GRenderWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    // windowHandle() is not initialized until the Window is shown, so we connect it here.
    connect(windowHandle(), &QWindow::screenChanged, this, &GRenderWindow::OnFramebufferSizeChanged,
            Qt::UniqueConnection);
}

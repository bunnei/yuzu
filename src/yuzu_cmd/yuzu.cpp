// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <iostream>
#include <memory>
#include <string>
#include <thread>

// This needs to be included before getopt.h because the latter #defines symbols used by it
#include "common/microprofile.h"

#ifdef _MSC_VER
#include <getopt.h>
#else
#include <getopt.h>
#include <unistd.h>
#endif

#ifdef _WIN32
// windows.h needs to be included before shellapi.h
#include <windows.h>

#include <shellapi.h>
#endif

#include "common/logging/backend.h"
#include "common/logging/filter.h"
#include "common/logging/log.h"
#include "common/scm_rev.h"
#include "common/scope_exit.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/gdbstub/gdbstub.h"
#include "core/loader/loader.h"
#include "core/settings.h"
#include "yuzu_cmd/config.h"
#include "yuzu_cmd/emu_window/emu_window_sdl2.h"

#ifdef _WIN32
extern "C" {
// tells Nvidia and AMD drivers to use the dedicated GPU by default on laptops with switchable
// graphics
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

static void PrintHelp(const char* argv0) {
    std::cout << "Usage: " << argv0
              << " [options] <filename>\n"
                 "-g, --gdbport=NUMBER  Enable gdb stub on port NUMBER\n"
                 "-f, --fullscreen     Start in fullscreen mode\n"
                 "-h, --help            Display this help and exit\n"
                 "-v, --version         Output version information and exit\n";
}

static void PrintVersion() {
    std::cout << "yuzu " << Common::g_scm_branch << " " << Common::g_scm_desc << std::endl;
}

/// Application entry point
int main(int argc, char** argv) {
    Config config;
    int option_index = 0;
    bool use_gdbstub = Settings::values.use_gdbstub;
    u32 gdb_port = static_cast<u32>(Settings::values.gdbstub_port);
    char* endarg;
#ifdef _WIN32
    int argc_w;
    auto argv_w = CommandLineToArgvW(GetCommandLineW(), &argc_w);

    if (argv_w == nullptr) {
        NGLOG_CRITICAL(Frontend, "Failed to get command line arguments");
        return -1;
    }
#endif
    std::string filepath;

    bool fullscreen = false;

    static struct option long_options[] = {
        {"gdbport", required_argument, 0, 'g'},
        {"fullscreen", no_argument, 0, 'f'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0},
    };

    while (optind < argc) {
        char arg = getopt_long(argc, argv, "g:fhv", long_options, &option_index);
        if (arg != -1) {
            switch (arg) {
            case 'g':
                errno = 0;
                gdb_port = strtoul(optarg, &endarg, 0);
                use_gdbstub = true;
                if (endarg == optarg)
                    errno = EINVAL;
                if (errno != 0) {
                    perror("--gdbport");
                    exit(1);
                }
                break;
            case 'f':
                fullscreen = true;
                NGLOG_INFO(Frontend, "Starting in fullscreen mode...");
                break;
            case 'h':
                PrintHelp(argv[0]);
                return 0;
            case 'v':
                PrintVersion();
                return 0;
            }
        } else {
#ifdef _WIN32
            filepath = Common::UTF16ToUTF8(argv_w[optind]);
#else
            filepath = argv[optind];
#endif
            optind++;
        }
    }

#ifdef _WIN32
    LocalFree(argv_w);
#endif

    Log::Filter log_filter(Log::Level::Debug);
    Log::SetFilter(&log_filter);

    MicroProfileOnThreadCreate("EmuThread");
    SCOPE_EXIT({ MicroProfileShutdown(); });

    if (filepath.empty()) {
        NGLOG_CRITICAL(Frontend, "Failed to load ROM: No ROM specified");
        return -1;
    }

    log_filter.ParseFilterString(Settings::values.log_filter);

    // Apply the command line arguments
    Settings::values.gdbstub_port = gdb_port;
    Settings::values.use_gdbstub = use_gdbstub;
    Settings::Apply();

    std::unique_ptr<EmuWindow_SDL2> emu_window{std::make_unique<EmuWindow_SDL2>(fullscreen)};

    Core::System& system{Core::System::GetInstance()};

    SCOPE_EXIT({ system.Shutdown(); });

    const Core::System::ResultStatus load_result{system.Load(emu_window.get(), filepath)};

    switch (load_result) {
    case Core::System::ResultStatus::ErrorGetLoader:
        NGLOG_CRITICAL(Frontend, "Failed to obtain loader for %s!", filepath.c_str());
        return -1;
    case Core::System::ResultStatus::ErrorLoader:
        NGLOG_CRITICAL(Frontend, "Failed to load ROM!");
        return -1;
    case Core::System::ResultStatus::ErrorLoader_ErrorEncrypted:
        NGLOG_CRITICAL(Frontend, "The game that you are trying to load must be decrypted before "
                                 "being used with yuzu. \n\n For more information on dumping and "
                                 "decrypting games, please refer to: "
                                 "https://yuzu-emu.org/wiki/dumping-game-cartridges/");
        return -1;
    case Core::System::ResultStatus::ErrorLoader_ErrorInvalidFormat:
        NGLOG_CRITICAL(Frontend, "Error while loading ROM: The ROM format is not supported.");
        return -1;
    case Core::System::ResultStatus::ErrorNotInitialized:
        NGLOG_CRITICAL(Frontend, "CPUCore not initialized");
        return -1;
    case Core::System::ResultStatus::ErrorSystemMode:
        NGLOG_CRITICAL(Frontend, "Failed to determine system mode!");
        return -1;
    case Core::System::ResultStatus::ErrorVideoCore:
        NGLOG_CRITICAL(Frontend, "VideoCore not initialized");
        return -1;
    case Core::System::ResultStatus::Success:
        break; // Expected case
    }

    Core::Telemetry().AddField(Telemetry::FieldType::App, "Frontend", "SDL");

    while (emu_window->IsOpen()) {
        system.RunLoop();
    }

    return 0;
}

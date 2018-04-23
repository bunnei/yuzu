// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include "common/assert.h"
#include "core/core.h"
#include "video_core/debug_utils/debug_utils.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/rasterizer_interface.h"
#include "video_core/renderer_base.h"
#include "video_core/textures/decoders.h"
#include "video_core/textures/texture.h"
#include "video_core/video_core.h"

namespace Tegra {
namespace Engines {

/// First register id that is actually a Macro call.
constexpr u32 MacroRegistersStart = 0xE00;

Maxwell3D::Maxwell3D(MemoryManager& memory_manager)
    : memory_manager(memory_manager), macro_interpreter(*this) {}

void Maxwell3D::SubmitMacroCode(u32 entry, std::vector<u32> code) {
    uploaded_macros[entry * 2 + MacroRegistersStart] = std::move(code);
}

void Maxwell3D::CallMacroMethod(u32 method, std::vector<u32> parameters) {
    auto macro_code = uploaded_macros.find(method);
    // The requested macro must have been uploaded already.
    ASSERT_MSG(macro_code != uploaded_macros.end(), "Macro %08X was not uploaded", method);

    // Reset the current macro and execute it.
    executing_macro = 0;
    macro_interpreter.Execute(macro_code->second, std::move(parameters));
}

void Maxwell3D::WriteReg(u32 method, u32 value, u32 remaining_params) {
    ASSERT_MSG(method < Regs::NUM_REGS,
               "Invalid Maxwell3D register, increase the size of the Regs structure");

    auto debug_context = Core::System::GetInstance().GetGPUDebugContext();

    // It is an error to write to a register other than the current macro's ARG register before it
    // has finished execution.
    if (executing_macro != 0) {
        ASSERT(method == executing_macro + 1);
    }

    // Methods after 0xE00 are special, they're actually triggers for some microcode that was
    // uploaded to the GPU during initialization.
    if (method >= MacroRegistersStart) {
        // We're trying to execute a macro
        if (executing_macro == 0) {
            // A macro call must begin by writing the macro method's register, not its argument.
            ASSERT_MSG((method % 2) == 0,
                       "Can't start macro execution by writing to the ARGS register");
            executing_macro = method;
        }

        macro_params.push_back(value);

        // Call the macro when there are no more parameters in the command buffer
        if (remaining_params == 0) {
            CallMacroMethod(executing_macro, std::move(macro_params));
        }
        return;
    }

    if (debug_context) {
        debug_context->OnEvent(Tegra::DebugContext::Event::MaxwellCommandLoaded, nullptr);
    }

    regs.reg_array[method] = value;

    switch (method) {
    case MAXWELL3D_REG_INDEX(code_address.code_address_high):
    case MAXWELL3D_REG_INDEX(code_address.code_address_low): {
        // Note: For some reason games (like Puyo Puyo Tetris) seem to write 0 to the CODE_ADDRESS
        // register, we do not currently know if that's intended or a bug, so we assert it lest
        // stuff breaks in other places (like the shader address calculation).
        ASSERT_MSG(regs.code_address.CodeAddress() == 0, "Unexpected CODE_ADDRESS register value.");
        break;
    }
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[0]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[1]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[2]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[3]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[4]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[5]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[6]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[7]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[8]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[9]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[10]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[11]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[12]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[13]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[14]):
    case MAXWELL3D_REG_INDEX(const_buffer.cb_data[15]): {
        ProcessCBData(value);
        break;
    }
    case MAXWELL3D_REG_INDEX(cb_bind[0].raw_config): {
        ProcessCBBind(Regs::ShaderStage::Vertex);
        break;
    }
    case MAXWELL3D_REG_INDEX(cb_bind[1].raw_config): {
        ProcessCBBind(Regs::ShaderStage::TesselationControl);
        break;
    }
    case MAXWELL3D_REG_INDEX(cb_bind[2].raw_config): {
        ProcessCBBind(Regs::ShaderStage::TesselationEval);
        break;
    }
    case MAXWELL3D_REG_INDEX(cb_bind[3].raw_config): {
        ProcessCBBind(Regs::ShaderStage::Geometry);
        break;
    }
    case MAXWELL3D_REG_INDEX(cb_bind[4].raw_config): {
        ProcessCBBind(Regs::ShaderStage::Fragment);
        break;
    }
    case MAXWELL3D_REG_INDEX(draw.vertex_end_gl): {
        DrawArrays();
        break;
    }
    case MAXWELL3D_REG_INDEX(query.query_get): {
        ProcessQueryGet();
        break;
    }
    default:
        break;
    }

    VideoCore::g_renderer->Rasterizer()->NotifyMaxwellRegisterChanged(method);

    if (debug_context) {
        debug_context->OnEvent(Tegra::DebugContext::Event::MaxwellCommandProcessed, nullptr);
    }
}

void Maxwell3D::ProcessQueryGet() {
    GPUVAddr sequence_address = regs.query.QueryAddress();
    // Since the sequence address is given as a GPU VAddr, we have to convert it to an application
    // VAddr before writing.
    boost::optional<VAddr> address = memory_manager.GpuToCpuAddress(sequence_address);

    switch (regs.query.query_get.mode) {
    case Regs::QueryMode::Write: {
        // Write the current query sequence to the sequence address.
        u32 sequence = regs.query.query_sequence;
        Memory::Write32(*address, sequence);
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Query mode %u not implemented",
                          static_cast<u32>(regs.query.query_get.mode.Value()));
    }
}

void Maxwell3D::DrawArrays() {
    LOG_DEBUG(HW_GPU, "called, topology=%d, count=%d", regs.draw.topology.Value(),
              regs.vertex_buffer.count);
    ASSERT_MSG(!(regs.index_array.count && regs.vertex_buffer.count), "Both indexed and direct?");

    auto debug_context = Core::System::GetInstance().GetGPUDebugContext();

    if (debug_context) {
        debug_context->OnEvent(Tegra::DebugContext::Event::IncomingPrimitiveBatch, nullptr);
    }

    if (debug_context) {
        debug_context->OnEvent(Tegra::DebugContext::Event::FinishedPrimitiveBatch, nullptr);
    }

    const bool is_indexed{regs.index_array.count && !regs.vertex_buffer.count};
    VideoCore::g_renderer->Rasterizer()->AccelerateDrawBatch(is_indexed);
}

void Maxwell3D::ProcessCBBind(Regs::ShaderStage stage) {
    // Bind the buffer currently in CB_ADDRESS to the specified index in the desired shader stage.
    auto& shader = state.shader_stages[static_cast<size_t>(stage)];
    auto& bind_data = regs.cb_bind[static_cast<size_t>(stage)];

    auto& buffer = shader.const_buffers[bind_data.index];

    buffer.enabled = bind_data.valid.Value() != 0;
    buffer.index = bind_data.index;
    buffer.address = regs.const_buffer.BufferAddress();
    buffer.size = regs.const_buffer.cb_size;
}

void Maxwell3D::ProcessCBData(u32 value) {
    // Write the input value to the current const buffer at the current position.
    GPUVAddr buffer_address = regs.const_buffer.BufferAddress();
    ASSERT(buffer_address != 0);

    // Don't allow writing past the end of the buffer.
    ASSERT(regs.const_buffer.cb_pos + sizeof(u32) <= regs.const_buffer.cb_size);

    boost::optional<VAddr> address =
        memory_manager.GpuToCpuAddress(buffer_address + regs.const_buffer.cb_pos);

    Memory::Write32(*address, value);

    // Increment the current buffer position.
    regs.const_buffer.cb_pos = regs.const_buffer.cb_pos + 4;
}

Texture::TICEntry Maxwell3D::GetTICEntry(u32 tic_index) const {
    GPUVAddr tic_base_address = regs.tic.TICAddress();

    GPUVAddr tic_address_gpu = tic_base_address + tic_index * sizeof(Texture::TICEntry);
    boost::optional<VAddr> tic_address_cpu = memory_manager.GpuToCpuAddress(tic_address_gpu);

    Texture::TICEntry tic_entry;
    Memory::ReadBlock(*tic_address_cpu, &tic_entry, sizeof(Texture::TICEntry));

    ASSERT_MSG(tic_entry.header_version == Texture::TICHeaderVersion::BlockLinear ||
                   tic_entry.header_version == Texture::TICHeaderVersion::Pitch,
               "TIC versions other than BlockLinear or Pitch are unimplemented");

    ASSERT_MSG((tic_entry.texture_type == Texture::TextureType::Texture2D) ||
                   (tic_entry.texture_type == Texture::TextureType::Texture2DNoMipmap),
               "Texture types other than Texture2D are unimplemented");

    auto r_type = tic_entry.r_type.Value();
    auto g_type = tic_entry.g_type.Value();
    auto b_type = tic_entry.b_type.Value();
    auto a_type = tic_entry.a_type.Value();

    // TODO(Subv): Different data types for separate components are not supported
    ASSERT(r_type == g_type && r_type == b_type && r_type == a_type);
    // TODO(Subv): Only UNORM formats are supported for now.
    ASSERT(r_type == Texture::ComponentType::UNORM);

    return tic_entry;
}

Texture::TSCEntry Maxwell3D::GetTSCEntry(u32 tsc_index) const {
    GPUVAddr tsc_base_address = regs.tsc.TSCAddress();

    GPUVAddr tsc_address_gpu = tsc_base_address + tsc_index * sizeof(Texture::TSCEntry);
    boost::optional<VAddr> tsc_address_cpu = memory_manager.GpuToCpuAddress(tsc_address_gpu);

    Texture::TSCEntry tsc_entry;
    Memory::ReadBlock(*tsc_address_cpu, &tsc_entry, sizeof(Texture::TSCEntry));
    return tsc_entry;
}

std::vector<Texture::FullTextureInfo> Maxwell3D::GetStageTextures(Regs::ShaderStage stage) const {
    std::vector<Texture::FullTextureInfo> textures;

    auto& fragment_shader = state.shader_stages[static_cast<size_t>(stage)];
    auto& tex_info_buffer = fragment_shader.const_buffers[regs.tex_cb_index];
    ASSERT(tex_info_buffer.enabled && tex_info_buffer.address != 0);

    GPUVAddr tic_base_address = regs.tic.TICAddress();

    GPUVAddr tex_info_buffer_end = tex_info_buffer.address + tex_info_buffer.size;

    // Offset into the texture constbuffer where the texture info begins.
    static constexpr size_t TextureInfoOffset = 0x20;

    for (GPUVAddr current_texture = tex_info_buffer.address + TextureInfoOffset;
         current_texture < tex_info_buffer_end; current_texture += sizeof(Texture::TextureHandle)) {

        Texture::TextureHandle tex_handle{
            Memory::Read32(*memory_manager.GpuToCpuAddress(current_texture))};

        Texture::FullTextureInfo tex_info{};
        // TODO(Subv): Use the shader to determine which textures are actually accessed.
        tex_info.index = (current_texture - tex_info_buffer.address - TextureInfoOffset) /
                         sizeof(Texture::TextureHandle);

        // Load the TIC data.
        if (tex_handle.tic_id != 0) {
            tex_info.enabled = true;

            auto tic_entry = GetTICEntry(tex_handle.tic_id);
            // TODO(Subv): Workaround for BitField's move constructor being deleted.
            std::memcpy(&tex_info.tic, &tic_entry, sizeof(tic_entry));
        }

        // Load the TSC data
        if (tex_handle.tsc_id != 0) {
            auto tsc_entry = GetTSCEntry(tex_handle.tsc_id);
            // TODO(Subv): Workaround for BitField's move constructor being deleted.
            std::memcpy(&tex_info.tsc, &tsc_entry, sizeof(tsc_entry));
        }

        if (tex_info.enabled)
            textures.push_back(tex_info);
    }

    return textures;
}

u32 Maxwell3D::GetRegisterValue(u32 method) const {
    ASSERT_MSG(method < Regs::NUM_REGS, "Invalid Maxwell3D register");
    return regs.reg_array[method];
}

bool Maxwell3D::IsShaderStageEnabled(Regs::ShaderStage stage) const {
    // The Vertex stage is always enabled.
    if (stage == Regs::ShaderStage::Vertex)
        return true;

    switch (stage) {
    case Regs::ShaderStage::TesselationControl:
        return regs.shader_config[static_cast<size_t>(Regs::ShaderProgram::TesselationControl)]
                   .enable != 0;
    case Regs::ShaderStage::TesselationEval:
        return regs.shader_config[static_cast<size_t>(Regs::ShaderProgram::TesselationEval)]
                   .enable != 0;
    case Regs::ShaderStage::Geometry:
        return regs.shader_config[static_cast<size_t>(Regs::ShaderProgram::Geometry)].enable != 0;
    case Regs::ShaderStage::Fragment:
        return regs.shader_config[static_cast<size_t>(Regs::ShaderProgram::Fragment)].enable != 0;
    }

    UNREACHABLE();
}

} // namespace Engines
} // namespace Tegra

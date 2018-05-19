// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <bitset>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <boost/optional.hpp>

#include "common/bit_field.h"
#include "common/common_types.h"

namespace Tegra {
namespace Shader {

struct Register {
    /// Number of registers
    static constexpr size_t NumRegisters = 256;

    /// Register 255 is special cased to always be 0
    static constexpr size_t ZeroIndex = 255;

    enum class Size : u64 {
        Byte = 0,
        Short = 1,
        Word = 2,
        Long = 3,
    };

    constexpr Register() = default;

    constexpr Register(u64 value) : value(value) {}

    constexpr operator u64() const {
        return value;
    }

    template <typename T>
    constexpr u64 operator-(const T& oth) const {
        return value - oth;
    }

    template <typename T>
    constexpr u64 operator&(const T& oth) const {
        return value & oth;
    }

    constexpr u64 operator&(const Register& oth) const {
        return value & oth.value;
    }

    constexpr u64 operator~() const {
        return ~value;
    }

    u64 GetSwizzledIndex(u64 elem) const {
        elem = (value + elem) & 3;
        return (value & ~3) + elem;
    }

private:
    u64 value{};
};

union Attribute {
    Attribute() = default;

    constexpr explicit Attribute(u64 value) : value(value) {}

    enum class Index : u64 {
        Position = 7,
        Attribute_0 = 8,
    };

    union {
        BitField<22, 2, u64> element;
        BitField<24, 6, Index> index;
        BitField<47, 3, u64> size;
    } fmt20;

    union {
        BitField<30, 2, u64> element;
        BitField<32, 6, Index> index;
    } fmt28;

    BitField<39, 8, u64> reg;
    u64 value{};
};

union Sampler {
    Sampler() = default;

    constexpr explicit Sampler(u64 value) : value(value) {}

    enum class Index : u64 {
        Sampler_0 = 8,
    };

    BitField<36, 13, Index> index;
    u64 value{};
};

union Uniform {
    BitField<20, 14, u64> offset;
    BitField<34, 5, u64> index;
};

} // namespace Shader
} // namespace Tegra

namespace std {

// TODO(bunnei): The below is forbidden by the C++ standard, but works fine. See #330.
template <>
struct make_unsigned<Tegra::Shader::Attribute> {
    using type = Tegra::Shader::Attribute;
};

template <>
struct make_unsigned<Tegra::Shader::Register> {
    using type = Tegra::Shader::Register;
};

} // namespace std

namespace Tegra {
namespace Shader {

enum class Pred : u64 {
    UnusedIndex = 0x7,
    NeverExecute = 0xF,
};

enum class PredCondition : u64 {
    LessThan = 1,
    Equal = 2,
    LessEqual = 3,
    GreaterThan = 4,
    NotEqual = 5,
    GreaterEqual = 6,
    // TODO(Subv): Other condition types
};

enum class PredOperation : u64 {
    And = 0,
    Or = 1,
    Xor = 2,
};

enum class SubOp : u64 {
    Cos = 0x0,
    Sin = 0x1,
    Ex2 = 0x2,
    Lg2 = 0x3,
    Rcp = 0x4,
    Rsq = 0x5,
    Min = 0x8,
};

union Instruction {
    Instruction& operator=(const Instruction& instr) {
        value = instr.value;
        return *this;
    }

    constexpr Instruction(u64 value) : value{value} {}

    BitField<0, 8, Register> gpr0;
    BitField<8, 8, Register> gpr8;
    union {
        BitField<16, 4, Pred> full_pred;
        BitField<16, 3, u64> pred_index;
    } pred;
    BitField<19, 1, u64> negate_pred;
    BitField<20, 8, Register> gpr20;
    BitField<20, 7, SubOp> sub_op;
    BitField<28, 8, Register> gpr28;
    BitField<39, 8, Register> gpr39;
    BitField<48, 16, u64> opcode;

    union {
        BitField<20, 19, u64> imm20_19;
        BitField<20, 32, u64> imm20_32;
        BitField<45, 1, u64> negate_b;
        BitField<46, 1, u64> abs_a;
        BitField<48, 1, u64> negate_a;
        BitField<49, 1, u64> abs_b;
        BitField<50, 1, u64> abs_d;
        BitField<56, 1, u64> negate_imm;

        float GetImm20_19() const {
            float result{};
            u32 imm{static_cast<u32>(imm20_19)};
            imm <<= 12;
            imm |= negate_imm ? 0x80000000 : 0;
            std::memcpy(&result, &imm, sizeof(imm));
            return result;
        }

        float GetImm20_32() const {
            float result{};
            u32 imm{static_cast<u32>(imm20_32)};
            std::memcpy(&result, &imm, sizeof(imm));
            return result;
        }
    } alu;

    union {
        BitField<48, 1, u64> negate_b;
        BitField<49, 1, u64> negate_c;
    } ffma;

    union {
        BitField<0, 3, u64> pred0;
        BitField<3, 3, u64> pred3;
        BitField<7, 1, u64> abs_a;
        BitField<39, 3, u64> pred39;
        BitField<42, 1, u64> neg_pred;
        BitField<43, 1, u64> neg_a;
        BitField<44, 1, u64> abs_b;
        BitField<45, 2, PredOperation> op;
        BitField<47, 1, u64> ftz;
        BitField<48, 4, PredCondition> cond;
        BitField<56, 1, u64> neg_b;
    } fsetp;

    union {
        BitField<39, 3, u64> pred39;
        BitField<42, 1, u64> neg_pred;
        BitField<43, 1, u64> neg_a;
        BitField<44, 1, u64> abs_b;
        BitField<45, 2, PredOperation> op;
        BitField<48, 4, PredCondition> cond;
        BitField<53, 1, u64> neg_b;
        BitField<54, 1, u64> abs_a;
        BitField<52, 1, u64> bf;
        BitField<55, 1, u64> ftz;
        BitField<56, 1, u64> neg_imm;
    } fset;

    union {
        BitField<10, 2, Register::Size> size;
        BitField<13, 1, u64> is_signed;
        BitField<41, 2, u64> selector;
        BitField<45, 1, u64> negate_a;
        BitField<49, 1, u64> abs_a;
        BitField<50, 1, u64> saturate_a;
    } conversion;

    BitField<61, 1, u64> is_b_imm;
    BitField<60, 1, u64> is_b_gpr;
    BitField<59, 1, u64> is_c_gpr;

    Attribute attribute;
    Uniform uniform;
    Sampler sampler;

    u64 value;
};
static_assert(sizeof(Instruction) == 0x8, "Incorrect structure size");
static_assert(std::is_standard_layout<Instruction>::value,
              "Structure does not have standard layout");

class OpCode {
public:
    enum class Id {
        KIL,
        LD_A,
        ST_A,
        TEXQ, // Texture Query
        TEXS, // Texture Fetch with scalar/non-vec4 source/destinations
        TLDS, // Texture Load with scalar/non-vec4 source/destinations
        EXIT,
        IPA,
        FFMA_IMM, // Fused Multiply and Add
        FFMA_CR,
        FFMA_RC,
        FFMA_RR,
        FADD_C,
        FADD_R,
        FADD_IMM,
        FMUL_C,
        FMUL_R,
        FMUL_IMM,
        FMUL32_IMM,
        MUFU, // Multi-Function Operator
        RRO,  // Range Reduction Operator
        F2F_C,
        F2F_R,
        F2F_IMM,
        F2I_C,
        F2I_R,
        F2I_IMM,
        I2F_C,
        I2F_R,
        I2F_IMM,
        I2I_C,
        I2I_R,
        I2I_IMM,
        LOP32I,
        MOV_C,
        MOV_R,
        MOV_IMM,
        MOV32_IMM,
        SHR_C,
        SHR_R,
        SHR_IMM,
        FMNMX,
        FSETP_C, // Set Predicate
        FSETP_R,
        FSETP_IMM,
        FSET_C,
        FSET_R,
        FSET_IMM,
        ISETP_C,
        ISETP_IMM,
        ISETP_R,
    };

    enum class Type {
        Trivial,
        Arithmetic,
        Ffma,
        Flow,
        Memory,
        FloatSet,
        FloatSetPredicate,
        IntegerSetPredicate,
        Conversion,
        Unknown,
    };

    class Matcher {
    public:
        Matcher(const char* const name, u16 mask, u16 expected, OpCode::Id id, OpCode::Type type)
            : name{name}, mask{mask}, expected{expected}, id{id}, type{type} {}

        const char* GetName() const {
            return name;
        }

        u16 GetMask() const {
            return mask;
        }

        Id GetId() const {
            return id;
        }

        Type GetType() const {
            return type;
        }

        /**
         * Tests to see if the given instruction is the instruction this matcher represents.
         * @param instruction The instruction to test
         * @returns true if the given instruction matches.
         */
        bool Matches(u16 instruction) const {
            return (instruction & mask) == expected;
        }

    private:
        const char* name;
        u16 mask;
        u16 expected;
        Id id;
        Type type;
    };

    static boost::optional<const Matcher&> Decode(Instruction instr) {
        static const auto table{GetDecodeTable()};

        const auto matches_instruction = [instr](const auto& matcher) {
            return matcher.Matches(static_cast<u16>(instr.opcode));
        };

        auto iter = std::find_if(table.begin(), table.end(), matches_instruction);
        return iter != table.end() ? boost::optional<const Matcher&>(*iter) : boost::none;
    }

private:
    struct Detail {
    private:
        static constexpr size_t opcode_bitsize = 16;

        /**
         * Generates the mask and the expected value after masking from a given bitstring.
         * A '0' in a bitstring indicates that a zero must be present at that bit position.
         * A '1' in a bitstring indicates that a one must be present at that bit position.
         */
        static auto GetMaskAndExpect(const char* const bitstring) {
            u16 mask = 0, expect = 0;
            for (size_t i = 0; i < opcode_bitsize; i++) {
                const size_t bit_position = opcode_bitsize - i - 1;
                switch (bitstring[i]) {
                case '0':
                    mask |= 1 << bit_position;
                    break;
                case '1':
                    expect |= 1 << bit_position;
                    mask |= 1 << bit_position;
                    break;
                default:
                    // Ignore
                    break;
                }
            }
            return std::make_tuple(mask, expect);
        }

    public:
        /// Creates a matcher that can match and parse instructions based on bitstring.
        static auto GetMatcher(const char* const bitstring, OpCode::Id op, OpCode::Type type,
                               const char* const name) {
            const auto mask_expect = GetMaskAndExpect(bitstring);
            return Matcher(name, std::get<0>(mask_expect), std::get<1>(mask_expect), op, type);
        }
    };

    static std::vector<Matcher> GetDecodeTable() {
        std::vector<Matcher> table = {
#define INST(bitstring, op, type, name) Detail::GetMatcher(bitstring, op, type, name)
            INST("111000110011----", Id::KIL, Type::Flow, "KIL"),
            INST("1110111111011---", Id::LD_A, Type::Memory, "LD_A"),
            INST("1110111111110---", Id::ST_A, Type::Memory, "ST_A"),
            INST("1101111101001---", Id::TEXQ, Type::Memory, "TEXQ"),
            INST("1101100---------", Id::TEXS, Type::Memory, "TEXS"),
            INST("1101101---------", Id::TLDS, Type::Memory, "TLDS"),
            INST("111000110000----", Id::EXIT, Type::Trivial, "EXIT"),
            INST("11100000--------", Id::IPA, Type::Trivial, "IPA"),
            INST("001100101-------", Id::FFMA_IMM, Type::Ffma, "FFMA_IMM"),
            INST("010010011-------", Id::FFMA_CR, Type::Ffma, "FFMA_CR"),
            INST("010100011-------", Id::FFMA_RC, Type::Ffma, "FFMA_RC"),
            INST("010110011-------", Id::FFMA_RR, Type::Ffma, "FFMA_RR"),
            INST("0100110001011---", Id::FADD_C, Type::Arithmetic, "FADD_C"),
            INST("0101110001011---", Id::FADD_R, Type::Arithmetic, "FADD_R"),
            INST("0011100-01011---", Id::FADD_IMM, Type::Arithmetic, "FADD_IMM"),
            INST("0100110001101---", Id::FMUL_C, Type::Arithmetic, "FMUL_C"),
            INST("0101110001101---", Id::FMUL_R, Type::Arithmetic, "FMUL_R"),
            INST("0011100-01101---", Id::FMUL_IMM, Type::Arithmetic, "FMUL_IMM"),
            INST("00011110--------", Id::FMUL32_IMM, Type::Arithmetic, "FMUL32_IMM"),
            INST("0101000010000---", Id::MUFU, Type::Arithmetic, "MUFU"),
            INST("0101110010010---", Id::RRO, Type::Arithmetic, "RRO"),
            INST("0100110010101---", Id::F2F_C, Type::Arithmetic, "F2F_C"),
            INST("0101110010101---", Id::F2F_R, Type::Arithmetic, "F2F_R"),
            INST("0011100-10101---", Id::F2F_IMM, Type::Arithmetic, "F2F_IMM"),
            INST("0100110010110---", Id::F2I_C, Type::Arithmetic, "F2I_C"),
            INST("0101110010110---", Id::F2I_R, Type::Arithmetic, "F2I_R"),
            INST("0011100-10110---", Id::F2I_IMM, Type::Arithmetic, "F2I_IMM"),
            INST("000001----------", Id::LOP32I, Type::Arithmetic, "LOP32I"),
            INST("0100110010011---", Id::MOV_C, Type::Arithmetic, "MOV_C"),
            INST("0101110010011---", Id::MOV_R, Type::Arithmetic, "MOV_R"),
            INST("0011100-10011---", Id::MOV_IMM, Type::Arithmetic, "MOV_IMM"),
            INST("000000010000----", Id::MOV32_IMM, Type::Arithmetic, "MOV32_IMM"),
            INST("0100110000101---", Id::SHR_C, Type::Arithmetic, "SHR_C"),
            INST("0101110000101---", Id::SHR_R, Type::Arithmetic, "SHR_R"),
            INST("0011100-00101---", Id::SHR_IMM, Type::Arithmetic, "SHR_IMM"),
            INST("0101110001100---", Id::FMNMX, Type::Arithmetic, "FMNMX"),
            INST("0100110011100---", Id::I2I_C, Type::Conversion, "I2I_C"),
            INST("0101110011100---", Id::I2I_R, Type::Conversion, "I2I_R"),
            INST("01110001-1000---", Id::I2I_IMM, Type::Conversion, "I2I_IMM"),
            INST("0100110010111---", Id::I2F_C, Type::Conversion, "I2F_C"),
            INST("0101110010111---", Id::I2F_R, Type::Conversion, "I2F_R"),
            INST("0011100-10111---", Id::I2F_IMM, Type::Conversion, "I2F_IMM"),
            INST("01011000--------", Id::FSET_R, Type::FloatSet, "FSET_R"),
            INST("0100100---------", Id::FSET_C, Type::FloatSet, "FSET_C"),
            INST("0011000---------", Id::FSET_IMM, Type::FloatSet, "FSET_IMM"),
            INST("010010111011----", Id::FSETP_C, Type::FloatSetPredicate, "FSETP_C"),
            INST("010110111011----", Id::FSETP_R, Type::FloatSetPredicate, "FSETP_R"),
            INST("0011011-1011----", Id::FSETP_IMM, Type::FloatSetPredicate, "FSETP_IMM"),
            INST("010010110110----", Id::ISETP_C, Type::IntegerSetPredicate, "ISETP_C"),
            INST("010110110110----", Id::ISETP_R, Type::IntegerSetPredicate, "ISETP_R"),
            INST("0011011-0110----", Id::ISETP_IMM, Type::IntegerSetPredicate, "ISETP_IMM"),
        };
#undef INST
        std::stable_sort(table.begin(), table.end(), [](const auto& a, const auto& b) {
            // If a matcher has more bits in its mask it is more specific, so it
            // should come first.
            return std::bitset<16>(a.GetMask()).count() > std::bitset<16>(b.GetMask()).count();
        });

        return table;
    }
};

} // namespace Shader
} // namespace Tegra

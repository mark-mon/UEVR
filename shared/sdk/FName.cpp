#include <spdlog/spdlog.h>

#include <utility/Scan.hpp>
#include <utility/String.hpp>
#include "EngineModule.hpp"

#include "FName.hpp"

namespace sdk {
/*
4.27:
+ 0x8 NumNetGUIDsPending
+ 0x7 SkeletalMeshComponentClothTick
+ 0x7 DefaultModulationPlugin
+ 0x4 Oculus Quest2
+ 0x4 FSequencerPlayerAnimSequence
+ 0x4 EndPhysicsTick
+ 0x4 StartPhysicsTick
+ 0x4 TickAnimationSharing
+ 0x4 LakeCollisionComponent
+ 0x4 SkeletalMeshComponentEndPhysicsTick
+ 0x4 Behavior
+ 0x4 FSlateMouseEventsMetaData
+ 0x4 FSlateCursorMetaData
+ 0x4 SoundExporterWAV
+ 0x4 FReflectionMetaData
+ 0x4 GameDefaultMap
+ 0x4 Test FName
+ 0x4 WaterBodyCollision
+ 0x4 WidgetTree
+ 0x4 FTagMetaData
+ 0x4 FSlateToolTipMetaData
+ 0x4 ParticleSystemManager
+ 0x4 Plugins
+ 0x4 FNavigationMetaData
+ 0x4 FSceneViewExtensionContext
*/

/*
4.14:
0x8 bIsPlaying
0x8 FLandscapeUniformShaderParameters
0x8 STAT_ColorList
0x7 OPUS
0x4 LightComponent
0x4 FPostProcessMaterialNode
0x4 SoundExporterWAV
0x4 Component
0x4 STextBlock
0x4 FLightPropagationVolumeSettings
0x4 CraneCameraMount
0x4 Priority
0x4 FTagMetaData
0x4 FNavigationMetaData
*/

/*
Present in both (and 5.0.3+):
+ 0x4 FTagMetaData
+ 0x4 FNavigationMetaData
*/
namespace detail {
std::optional<FName::ConstructorFn> get_constructor_from_candidate(std::wstring_view module_candidate, std::wstring_view str_candidate) {
    SPDLOG_INFO("FName::get_constructor_from_candidate: str_candidate={}", utility::narrow(str_candidate.data()));

    const auto module = sdk::get_ue_module(module_candidate.data());

    if (module == nullptr) {
        SPDLOG_ERROR("FName::get_constructor_from_candidate: Failed to get module {}", utility::narrow(module_candidate.data()));
        return std::nullopt;
    }

    const auto str_data = utility::scan_string(module, str_candidate.data());

    if (!str_data) {
        SPDLOG_ERROR("FName::get_constructor_from_candidate: Failed to get string data for {}", utility::narrow(str_candidate.data()));
        return std::nullopt;
    }

    SPDLOG_INFO("FName::get_constructor_from_candidate: str_data={:x}", *str_data);

    const auto str_ref = utility::scan_displacement_reference(module, *str_data);

    if (!str_ref) {
        SPDLOG_ERROR("FName::get_constructor_from_candidate: Failed to get string reference for {}", utility::narrow(str_candidate.data()));
        return std::nullopt;
    }

    SPDLOG_INFO("FName::get_constructor_from_candidate: str_ref={:x}", *str_ref);

    const auto next_instr = *str_ref + 4;
    const auto call_instr = utility::scan_mnemonic(next_instr, 10, "CALL");

    if (!call_instr) {
        SPDLOG_ERROR("FName::get_constructor_from_candidate: Failed to get call instruction for {}", utility::narrow(str_candidate.data()));
        return std::nullopt;
    }

    SPDLOG_INFO("FName::get_constructor_from_candidate: call_instr={:x}", *call_instr);

    const auto decoded = utility::decode_one((uint8_t*)*call_instr);

    if (!decoded) {
        SPDLOG_ERROR("FName::get_constructor_from_candidate: Failed to decode call instruction for {}", utility::narrow(str_candidate.data()));
        return std::nullopt;
    }

    const auto displacement = utility::resolve_displacement(*call_instr);

    if (!displacement) {
        SPDLOG_ERROR("FName::get_constructor_from_candidate: Failed to resolve displacement for {}", utility::narrow(str_candidate.data()));
        return std::nullopt;
    }

    std::optional<FName::ConstructorFn> result{};

    if (decoded->BranchInfo.IsIndirect) {
        SPDLOG_INFO("FName::get_constructor_from_candidate: indirect call");
        if (*displacement == 0) {
            SPDLOG_ERROR("FName::get_constructor_from_candidate: indirect call with null displacement");
            return std::nullopt;
        }

        result = *(FName::ConstructorFn*)*displacement;
    } else {
        SPDLOG_INFO("FName::get_constructor_from_candidate: direct call");
        result = (FName::ConstructorFn)*displacement;
    }

    SPDLOG_INFO("FName::get_constructor_from_candidate: result={:x}", (uintptr_t)*result);
    return result;
}
}

std::optional<FName::ConstructorFn> FName::get_constructor() {
    static auto result = []() -> std::optional<FName::ConstructorFn> {
        struct Candidate {
            std::wstring_view module;
            std::wstring_view str;
        };

        std::vector<Candidate> candidates {
            Candidate{L"SlateCore", L"FTagMetaData"},
            Candidate{L"SlateCore", L"FNavigationMetaData"}
        };

        const auto now = std::chrono::high_resolution_clock::now();

        for (const auto& candidate : candidates) {
            const auto constructor = detail::get_constructor_from_candidate(candidate.module, candidate.str);

            if (constructor) {
                const auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - now).count();
                SPDLOG_INFO("FName::get_constructor: Found constructor in {} ms", time_elapsed);
                return constructor;
            }
        }

        SPDLOG_ERROR("FName::get_constructor: Failed to find constructor");
        return std::nullopt;
    }();

    return result;
}

std::optional<FName::ToStringFn> FName::get_to_string() {
    static auto result = []() -> std::optional<FName::ToStringFn> {
        SPDLOG_INFO("FName::get_to_string");

        const auto module = sdk::get_ue_module(L"Engine");

        if (module == nullptr) {
            SPDLOG_ERROR("FName::get_to_string: Failed to get module");
            return std::nullopt;
        }

        const auto str_data = utility::scan_string(module, L"TAutoWeakObjectPtr<%s%s>");
        if (!str_data) {
            SPDLOG_ERROR("FName::get_to_string: Failed to get string data");
            return std::nullopt;
        }
        
        SPDLOG_INFO("FName::get_to_string: str_data={:x}", *str_data);

        const auto str_ref = utility::scan_displacement_reference(module, *str_data);

        if (!str_ref) {
            SPDLOG_ERROR("FName::get_to_string: Failed to get string reference");
            return std::nullopt;
        }

        SPDLOG_INFO("FName::get_to_string: str_ref={:x}", *str_ref);

        const auto containing_func = utility::find_function_start(*str_ref);

        if (!containing_func) {
            SPDLOG_ERROR("FName::get_to_string: Failed to find containing function");
            return std::nullopt;
        }

        SPDLOG_INFO("FName::get_to_string: containing_func={:x}", *containing_func);

        std::optional<uintptr_t> last_direct_call{};
        std::optional<FName::ToStringFn> result{};

        // It's known that FName::ToString precedes a string reference to TAutoWeakObjectPtr<%s%s>.
        // So we must keep track of each direct call that precedes it
        // because there's an indirect call that also precedes it.
        // TODO: Possibly fix this for modular builds.
        utility::exhaustive_decode((uint8_t*)*containing_func, 100, [&](INSTRUX& ix, uintptr_t ip) -> utility::ExhaustionResult {
            if (result) {
                return utility::ExhaustionResult::BREAK;
            }

            if (std::string_view{ix.Mnemonic}.starts_with("CALL") && !ix.BranchInfo.IsIndirect) {
                last_direct_call = utility::calculate_absolute((ip + ix.Length) - 4);
                SPDLOG_INFO("Found a direct call to {:x}", *last_direct_call);
            }

            if (std::string_view{ix.Mnemonic}.starts_with("CALL")) {
                return utility::ExhaustionResult::STEP_OVER;
            }

            const auto displacement = utility::resolve_displacement(ip);

            if (displacement && *displacement == str_data) {
                if (last_direct_call) {
                    result = (FName::ToStringFn)*last_direct_call;
                }

                return utility::ExhaustionResult::BREAK;
            }

            return utility::ExhaustionResult::CONTINUE;
        });

        if (!result) {
            SPDLOG_ERROR("FName::get_to_string: Failed to find ToString function");
            return std::nullopt;
        }

        SPDLOG_INFO("FName::get_to_string: result={:x}", (uintptr_t)*result);

        return result;
    }();

    return result;
}

FName::FName(std::wstring_view name, EFindName find_type) {
    const auto constructor = get_constructor();

    if (!constructor) {
        return;
    }

    const auto fn = *constructor;

    fn(this, name.data(), static_cast<uint32_t>(find_type));
}

std::wstring FName::to_string() const {
    const auto to_string = get_to_string();

    if (!to_string) {
        return L"";
    }

    const auto fn = *to_string;
    TArray<wchar_t> buffer{};

    fn(this, &buffer);

    return buffer.data;
}
}
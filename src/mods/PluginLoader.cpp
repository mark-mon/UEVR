#include <filesystem>

#include <imgui.h>
#include <spdlog/spdlog.h>

#include "Framework.hpp"
#include "uevr/API.h"

#include <utility/String.hpp>
#include <utility/Module.hpp>

#include <sdk/UEngine.hpp>

#include "VR.hpp"

#include "PluginLoader.hpp"

UEVR_PluginVersion g_plugin_version{
    UEVR_PLUGIN_VERSION_MAJOR, UEVR_PLUGIN_VERSION_MINOR, UEVR_PLUGIN_VERSION_PATCH};

namespace uevr {
UEVR_RendererData g_renderer_data{
    UEVR_RENDERER_D3D12, nullptr, nullptr, nullptr
};
}

namespace uevr {
void log_error(const char* format, ...) {
    va_list args{};
    va_start(args, format);
    auto str = utility::format_string(format, args);
    va_end(args);
    spdlog::error("[Plugin] {}", str);
}
void log_warn(const char* format, ...) {
    va_list args{};
    va_start(args, format);
    auto str = utility::format_string(format, args);
    va_end(args);
    spdlog::warn("[Plugin] {}", str);
}
void log_info(const char* format, ...) {
    va_list args{};
    va_start(args, format);
    auto str = utility::format_string(format, args);
    va_end(args);
    spdlog::info("[Plugin] {}", str);
}
bool is_drawing_ui() {
    return g_framework->is_drawing_ui();
}
}

namespace uevr {
bool on_present(UEVR_OnPresentCb cb) {
    if (cb == nullptr) {
        return false;
    }

    return PluginLoader::get()->add_on_present(cb);
}

bool on_device_reset(UEVR_OnDeviceResetCb cb) {
    if (cb == nullptr) {
        return false;
    }

    return PluginLoader::get()->add_on_device_reset(cb);
}

bool on_message(UEVR_OnMessageCb cb) {
    if (cb == nullptr) {
        return false;
    }

    return PluginLoader::get()->add_on_message(cb);
}
}

namespace uevr {
bool on_engine_tick(UEVR_Engine_TickCb cb) {
    if (cb == nullptr) {
        return false;
    }

    return PluginLoader::get()->add_on_engine_tick(cb);
}

bool on_slate_draw_window_render_thread(UEVR_Slate_DrawWindow_RenderThreadCb cb) {
    if (cb == nullptr) {
        return false;
    }

    return PluginLoader::get()->add_on_slate_draw_window_render_thread(cb);
}
}

UEVR_PluginCallbacks g_plugin_callbacks {
    uevr::on_present,
    uevr::on_device_reset,
    uevr::on_message
};

UEVR_PluginFunctions g_plugin_functions {
    uevr::log_error,
    uevr::log_warn,
    uevr::log_info,
    uevr::is_drawing_ui
};

UEVR_SDKFunctions g_sdk_functions {
    []() -> UEVR_UEngineHandle {
        return (UEVR_UEngineHandle)sdk::UEngine::get();
    }
};

UEVR_SDKCallbacks g_sdk_callbacks {
    uevr::on_engine_tick,
    uevr::on_slate_draw_window_render_thread
};

UEVR_SDKData g_sdk_data {
    &g_sdk_functions,
    &g_sdk_callbacks
};

namespace uevr {
namespace vr {
bool is_runtime_ready() {
    return ::VR::get()->get_runtime()->ready();
}

bool is_openvr() {
    return ::VR::get()->get_runtime()->is_openvr();
}

bool is_openxr() {
    return ::VR::get()->get_runtime()->is_openxr();
}

bool is_hmd_active() {
    return ::VR::get()->is_hmd_active();
}

void get_standing_origin(UEVR_Vector3f* out_origin) {
    auto origin = ::VR::get()->get_standing_origin();
    out_origin->x = origin.x;
    out_origin->y = origin.y;
    out_origin->z = origin.z;
}

void get_rotation_offset(UEVR_Quaternionf* out_rotation) {
    auto rotation = ::VR::get()->get_rotation_offset();
    out_rotation->x = rotation.x;
    out_rotation->y = rotation.y;
    out_rotation->z = rotation.z;
    out_rotation->w = rotation.w;
}

void set_standing_origin(const UEVR_Vector3f* origin) {
    ::VR::get()->set_standing_origin({origin->x, origin->y, origin->z, 1.0f});
}

void set_rotation_offset(const UEVR_Quaternionf* rotation) {
    ::VR::get()->set_rotation_offset({rotation->w, rotation->x, rotation->y, rotation->z});
}

void get_pose(UEVR_TrackedDeviceIndex index, UEVR_Vector3f* out_position, UEVR_Quaternionf* out_rotation) {
    static_assert(sizeof(UEVR_Vector3f) == sizeof(glm::vec3));
    static_assert(sizeof(UEVR_Quaternionf) == sizeof(glm::quat));

    auto transform = ::VR::get()->get_transform(index);

    out_position->x = transform[3].x;
    out_position->y = transform[3].y;
    out_position->z = transform[3].z;

    const auto rot = glm::quat{glm::extractMatrixRotation(transform)};
    out_rotation->x = rot.x;
    out_rotation->y = rot.y;
    out_rotation->z = rot.z;
    out_rotation->w = rot.w;
}

void get_transform(UEVR_TrackedDeviceIndex index, UEVR_Matrix4x4f* out_transform) {
    static_assert(sizeof(UEVR_Matrix4x4f) == sizeof(glm::mat4), "UEVR_Matrix4x4f and glm::mat4 must be the same size");

    const auto transform = ::VR::get()->get_transform(index);
    memcpy(out_transform, &transform, sizeof(UEVR_Matrix4x4f));
}

void get_ue_projection_matrix(UEVR_Eye eye, UEVR_Matrix4x4f* out_projection) {
    const auto& projection = ::VR::get()->get_runtime()->projections[eye];
    memcpy(out_projection, &projection, sizeof(UEVR_Matrix4x4f));
}
}
}

namespace uevr {
namespace openvr{

}
}

namespace uevr {
namespace openxr {

}
}

UEVR_VRData g_vr_data {
    uevr::vr::is_runtime_ready,
    uevr::vr::is_openvr,
    uevr::vr::is_openxr,
    uevr::vr::is_hmd_active,
    uevr::vr::get_standing_origin,
    uevr::vr::get_rotation_offset,
    uevr::vr::set_standing_origin,
    uevr::vr::set_rotation_offset,
    uevr::vr::get_pose,
    uevr::vr::get_transform,
    uevr::vr::get_ue_projection_matrix
};

UEVR_OpenVRData g_openvr_data {
};

UEVR_OpenXRData g_openxr_data {
};

UEVR_PluginInitializeParam g_plugin_initialize_param{
    nullptr, 
    &g_plugin_version, 
    &g_plugin_functions, 
    &g_plugin_callbacks,
    &uevr::g_renderer_data,
    &g_vr_data,
    &g_openvr_data,
    &g_openxr_data,
    &g_sdk_data
};

void verify_sdk_pointers() {
    auto verify = [](auto& g) {
        spdlog::info("Verifying...");

        for (auto i = 0; i < sizeof(g) / sizeof(void*); ++i) {
            if (((void**)&g)[i] == nullptr) {
                spdlog::error("SDK pointer is null at index {}", i);
            }
        }
    };

    spdlog::info("Verifying SDK pointers...");

    verify(g_sdk_data);
}

std::shared_ptr<PluginLoader>& PluginLoader::get() {
    static auto instance = std::make_shared<PluginLoader>();
    return instance;
}

void PluginLoader::early_init() {
    namespace fs = std::filesystem;

    std::scoped_lock _{m_mux};
    std::string module_path{};

    module_path.resize(1024, 0);
    module_path.resize(GetModuleFileName(nullptr, module_path.data(), module_path.size()));
    spdlog::info("[PluginLoader] Module path {}", module_path);

    auto plugin_path = fs::path{module_path}.parent_path() / "UEVR" / "plugins";

    spdlog::info("[PluginLoader] Creating directories {}", plugin_path.string());
    fs::create_directories(plugin_path);
    spdlog::info("[PluginLoader] Loading plugins...");

    // Load all dlls in the plugins directory.
    for (auto&& entry : fs::directory_iterator{plugin_path}) {
        auto&& path = entry.path();

        if (path.has_extension() && path.extension() == ".dll") {
            auto module = LoadLibrary(path.string().c_str());

            if (module == nullptr) {
                spdlog::error("[PluginLoader] Failed to load {}", path.string());
                m_plugin_load_errors.emplace(path.stem().string(), "Failed to load");
                continue;
            }

            spdlog::info("[PluginLoader] Loaded {}", path.string());
            m_plugins.emplace(path.stem().string(), module);
        }
    }
}

std::optional<std::string> PluginLoader::on_initialize() {
    std::scoped_lock _{m_mux};

    // Call UEVR_plugin_required_version on any dlls that export it.
    g_plugin_initialize_param.uevr_module = g_framework->get_framework_module();
    uevr::g_renderer_data.renderer_type = (int)g_framework->get_renderer_type();
    
    if (uevr::g_renderer_data.renderer_type == UEVR_RENDERER_D3D11) {
        auto& d3d11 = g_framework->get_d3d11_hook();

        uevr::g_renderer_data.device = d3d11->get_device();
        uevr::g_renderer_data.swapchain = d3d11->get_swap_chain();
    } else if (uevr::g_renderer_data.renderer_type == UEVR_RENDERER_D3D12) {
        auto& d3d12 = g_framework->get_d3d12_hook();

        uevr::g_renderer_data.device = d3d12->get_device();
        uevr::g_renderer_data.swapchain = d3d12->get_swap_chain();
        uevr::g_renderer_data.command_queue = d3d12->get_command_queue();
    } else {
        spdlog::error("[PluginLoader] Unsupported renderer type {}", uevr::g_renderer_data.renderer_type);
        return "PluginLoader: Unsupported renderer type detected";
    }

    verify_sdk_pointers();

    for (auto it = m_plugins.begin(); it != m_plugins.end();) {
        auto name = it->first;
        auto mod = it->second;
        auto required_version_fn = (UEVR_PluginRequiredVersionFn)GetProcAddress(mod, "uevr_plugin_required_version");

        if (required_version_fn == nullptr) {
            spdlog::info("[PluginLoader] {} has no uevr_plugin_required_version function, skipping...", name);

            ++it;
            continue;
        }

        UEVR_PluginVersion required_version{};

        try {
            required_version_fn(&required_version);
        } catch(...) {
            spdlog::error("[PluginLoader] {} has an exception in uevr_plugin_required_version, skipping...", name);
            m_plugin_load_errors.emplace(name, "Exception occurred in uevr_plugin_required_version");
            FreeLibrary(mod);
            it = m_plugins.erase(it);
            continue;
        }

        spdlog::info(
            "[PluginLoader] {} requires version {}.{}.{}", name, required_version.major, required_version.minor, required_version.patch);

        if (required_version.major != g_plugin_version.major) {
            spdlog::error("[PluginLoader] Plugin {} requires a different major version", name);
            m_plugin_load_errors.emplace(name, "Requires a different major version");
            FreeLibrary(mod);
            it = m_plugins.erase(it);
            continue;
        }

        if (required_version.minor > g_plugin_version.minor) {
            spdlog::error("[PluginLoader] Plugin {} requires a newer minor version", name);
            m_plugin_load_errors.emplace(name, "Requires a newer minor version");
            FreeLibrary(mod);
            it = m_plugins.erase(it);
            continue;
        }

        if (required_version.patch > g_plugin_version.patch) {
            spdlog::warn("[PluginLoader] Plugin {} desires a newer patch version", name);
            m_plugin_load_warnings.emplace(name, "Desires a newer patch version");
        }

        ++it;
    }

    // Call UEVR_plugin_initialize on any dlls that export it.
    for (auto it = m_plugins.begin(); it != m_plugins.end();) {
        auto name = it->first;
        auto mod = it->second;
        auto init_fn = (UEVR_PluginInitializeFn)GetProcAddress(mod, "uevr_plugin_initialize");

        if (init_fn == nullptr) {
            ++it;
            continue;
        }

        spdlog::info("[PluginLoader] Initializing {}...", name);
        try {
            if (!init_fn(&g_plugin_initialize_param)) {
                spdlog::error("[PluginLoader] Failed to initialize {}", name);
                m_plugin_load_errors.emplace(name, "Failed to initialize");
                FreeLibrary(mod);
                it = m_plugins.erase(it);
                continue;
            }
        } catch(...) {
            spdlog::error("[PluginLoader] {} has an exception in uevr_plugin_initialize, skipping...", name);
            m_plugin_load_errors.emplace(name, "Exception occurred in uevr_plugin_initialize");
            FreeLibrary(mod);
            it = m_plugins.erase(it);
            continue;
        }

        ++it;
    }

    return std::nullopt;
}

void PluginLoader::on_draw_ui() {
    ImGui::SetNextItemOpen(false, ImGuiCond_Once);

    if (ImGui::CollapsingHeader(get_name().data())) {
        std::scoped_lock _{m_mux};

        if (!m_plugins.empty()) {
            ImGui::Text("Loaded plugins:");

            for (auto&& [name, _] : m_plugins) {
                ImGui::Text(name.c_str());
            }
        } else {
            ImGui::Text("No plugins loaded.");
        }

        if (!m_plugin_load_errors.empty()) {
            ImGui::Spacing();
            ImGui::Text("Errors:");
            for (auto&& [name, error] : m_plugin_load_errors) {
                ImGui::Text("%s - %s", name.c_str(), error.c_str());
            }
        }

        if (!m_plugin_load_warnings.empty()) {
            ImGui::Spacing();
            ImGui::Text("Warnings:");
            for (auto&& [name, warning] : m_plugin_load_warnings) {
                ImGui::Text("%s - %s", name.c_str(), warning.c_str());
            }
        }
    }
}

void PluginLoader::on_present() {
    std::shared_lock _{m_api_cb_mtx};

    uevr::g_renderer_data.renderer_type = (int)g_framework->get_renderer_type();
    
    if (uevr::g_renderer_data.renderer_type == UEVR_RENDERER_D3D11) {
        auto& d3d11 = g_framework->get_d3d11_hook();

        uevr::g_renderer_data.device = d3d11->get_device();
        uevr::g_renderer_data.swapchain = d3d11->get_swap_chain();
    } else if (uevr::g_renderer_data.renderer_type == UEVR_RENDERER_D3D12) {
        auto& d3d12 = g_framework->get_d3d12_hook();

        uevr::g_renderer_data.device = d3d12->get_device();
        uevr::g_renderer_data.swapchain = d3d12->get_swap_chain();
        uevr::g_renderer_data.command_queue = d3d12->get_command_queue();
    }

    for (auto&& cb : m_on_present_cbs) {
        try {
            cb();
        } catch(...) {
            spdlog::error("[PluginLoader] Exception occurred in on_present callback; one of the plugins has an error.");
        }
    }
}

void PluginLoader::on_device_reset() {
    std::shared_lock _{m_api_cb_mtx};

    for (auto&& cb : m_on_device_reset_cbs) {
        try {
            cb();
        } catch(...) {
            spdlog::error("[APIProxy] Exception occurred in on_device_reset callback; one of the plugins has an error.");
        }
    }
}

bool PluginLoader::on_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    std::shared_lock _{m_api_cb_mtx};

    for (auto&& cb : m_on_message_cbs) {
        try {
            if (!cb(hwnd, msg, wparam, lparam)) {
                return false;
            }
        } catch(...) {
            spdlog::error("[APIProxy] Exception occurred in on_message callback; one of the plugins has an error.");
            continue;
        }
    }

    return true;
}

void PluginLoader::on_engine_tick(sdk::UGameEngine* engine, float delta) {
    std::shared_lock _{m_api_cb_mtx};

    for (auto&& cb : m_on_engine_tick_cbs) {
        try {
            cb((UEVR_UGameEngineHandle)engine, delta);
        } catch(...) {
            spdlog::error("[APIProxy] Exception occurred in on_engine_tick callback; one of the plugins has an error.");
        }
    }
}

void PluginLoader::on_slate_draw_window(void* renderer, void* command_list, sdk::FViewportInfo* viewport_info) {
    std::shared_lock _{m_api_cb_mtx};

    for (auto&& cb : m_on_slate_draw_window_render_thread_cbs) {
        try {
            cb((UEVR_FSlateRHIRendererHandle)renderer, (UEVR_FViewportInfoHandle)viewport_info);
        } catch(...) {
            spdlog::error("[APIProxy] Exception occurred in on_slate_draw_window callback; one of the plugins has an error.");
        }
    }
}


bool PluginLoader::add_on_present(UEVR_OnPresentCb cb) {
    std::unique_lock _{m_api_cb_mtx};

    m_on_present_cbs.push_back(cb);
    return true;
}

bool PluginLoader::add_on_device_reset(UEVR_OnDeviceResetCb cb) {
    std::unique_lock _{m_api_cb_mtx};

    m_on_device_reset_cbs.push_back(cb);
    return true;
}

bool PluginLoader::add_on_message(UEVR_OnMessageCb cb) {
    std::unique_lock _{m_api_cb_mtx};

    m_on_message_cbs.push_back(cb);
    return true;
}

bool PluginLoader::add_on_engine_tick(UEVR_Engine_TickCb cb) {
    std::unique_lock _{m_api_cb_mtx};

    m_on_engine_tick_cbs.push_back(cb);
    return true;
}

bool PluginLoader::add_on_slate_draw_window_render_thread(UEVR_Slate_DrawWindow_RenderThreadCb cb) {
    std::unique_lock _{m_api_cb_mtx};

    m_on_slate_draw_window_render_thread_cbs.push_back(cb);
    return true;
}

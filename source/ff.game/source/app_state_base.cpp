#include "pch.h"
#include "app_state_base.h"
#include "debug_state.h"

static ff::game::app_state_base* global_app_state;

const size_t ff::game::app_state_base::ID_DEBUG_HIDE_UI = ff::stable_hash_func("ff::game::app_state_base::ID_DEBUG_HIDE_UI"sv);
const size_t ff::game::app_state_base::ID_DEBUG_SHOW_UI = ff::stable_hash_func("ff::game::app_state_base::ID_DEBUG_SHOW_UI"sv);
const size_t ff::game::app_state_base::ID_DEBUG_RESTART_GAME = ff::stable_hash_func("ff::game::app_state_base::ID_DEBUG_RESTART_GAME"sv);
const size_t ff::game::app_state_base::ID_DEBUG_REBUILD_RESOURCES = ff::stable_hash_func("ff::game::app_state_base::ID_DEBUG_REBUILD_RESOURCES"sv);

static const size_t ID_DEBUG_STEP_ONE_FRAME = ff::stable_hash_func("ff.game.step_one_frame"sv);
static const size_t ID_DEBUG_CANCEL_STEP_ONE_FRAME = ff::stable_hash_func("ff.game.cancel_step_one_frame"sv);
static const size_t ID_DEBUG_SPEED_SLOW = ff::stable_hash_func("ff.game.speed_slow"sv);
static const size_t ID_DEBUG_SPEED_FAST = ff::stable_hash_func("ff.game.speed_fast"sv);
static const size_t ID_SHOW_CUSTOM_DEBUG = ff::stable_hash_func("ff.game.show_custom_debug"sv);
static const std::string_view ID_APP_STATE = "ff::game::ID_APP_STATE";
static const std::string_view ID_SYSTEM_OPTIONS = "ff::game::ID_SYSTEM_OPTIONS";

ff::game::app_state_base::app_state_base()
{
    assert(!::global_app_state);
    ::global_app_state = this;

    this->connections.emplace_front(ff::request_save_settings_sink().connect(std::bind(&ff::game::app_state_base::on_save_settings, this)));
    this->connections.emplace_front(ff::custom_debug_sink().connect(std::bind(&ff::game::app_state_base::on_custom_debug, this)));
    this->connections.emplace_front(ff::global_resources::rebuilt_sink().connect(std::bind(&ff::game::app_state_base::on_resources_rebuilt, this)));
}

ff::game::app_state_base::~app_state_base()
{
    assert(::global_app_state == this);
    ::global_app_state = nullptr;
}

ff::game::app_state_base* ff::game::app_state_base::get()
{
    return ::global_app_state;
}

void ff::game::app_state_base::init()
{
    this->load_settings();
    this->init_resources();
    this->init_game_state();
    this->apply_system_options();
}

const ff::game::system_options& ff::game::app_state_base::system_options() const
{
    return this->system_options_;
}

void ff::game::app_state_base::system_options(const ff::game::system_options& options)
{
    this->system_options_ = options;
    this->apply_system_options();
}

ff::signal_sink<>& ff::game::app_state_base::reload_resources_sink()
{
    return this->reload_resources_signal;
}

double ff::game::app_state_base::time_scale()
{
    return this->debug_time_scale;
}

ff::state::advance_t ff::game::app_state_base::advance_type()
{
    if (this->debug_step_one_frame)
    {
        return ff::state::advance_t::single_step;
    }

    if (this->debug_stepping_frames || this->rebuilding_resources)
    {
        return ff::state::advance_t::stopped;
    }

    return ff::state::advance_t::running;
}

ff::dxgi::palette_base* ff::game::app_state_base::palette()
{
    return nullptr;
}

bool ff::game::app_state_base::allow_debug()
{
    return DEBUG != 0;
}

void ff::game::app_state_base::debug_command(size_t command_id)
{
    if (command_id == ff::game::app_state_base::ID_DEBUG_HIDE_UI)
    {
        this->pending_debug_state = 1;
    }
    else if (command_id == ff::game::app_state_base::ID_DEBUG_SHOW_UI)
    {
        this->pending_debug_state = 2;
    }
    else if (command_id == ff::game::app_state_base::ID_DEBUG_REBUILD_RESOURCES)
    {
        if (!this->rebuilding_resources)
        {
            this->rebuilding_resources = true;
            ff::global_resources::rebuild_async();
        }
    }
    else if (command_id == ff::game::app_state_base::ID_DEBUG_RESTART_GAME)
    {
        this->init_game_state();
    }
}

bool ff::game::app_state_base::clear_color(DirectX::XMFLOAT4&)
{
    return false;
}

std::shared_ptr<ff::state> ff::game::app_state_base::advance_time()
{
    switch (this->pending_debug_state)
    {
        case 2:
            this->pending_debug_state = 0;

            if (!this->debug_state->visible())
            {
                this->on_custom_debug();
            }
            break;

        case 1:
            this->pending_debug_state = 0;
            this->debug_state->hide();
            break;
    }

    return ff::state::advance_time();
}

void ff::game::app_state_base::advance_input()
{
    if (this->allow_debug())
    {
        if (this->debug_input_events->advance())
        {
            if (this->debug_input_events->event_hit(::ID_DEBUG_CANCEL_STEP_ONE_FRAME))
            {
                this->debug_step_one_frame = false;
                this->debug_stepping_frames = false;
            }

            if (this->debug_input_events->event_hit(::ID_DEBUG_STEP_ONE_FRAME))
            {
                this->debug_step_one_frame = this->debug_stepping_frames;
                this->debug_stepping_frames = true;
            }
        }

        if (this->debug_input_events->digital_value(::ID_DEBUG_SPEED_FAST))
        {
            this->debug_time_scale = 4.0;
        }
        else if (this->debug_input_events->digital_value(::ID_DEBUG_SPEED_SLOW))
        {
            this->debug_time_scale = 0.25;
        }
        else
        {
            this->debug_time_scale = 1.0;
        }
    }
    else
    {
        this->debug_step_one_frame = false;
        this->debug_stepping_frames = false;
        this->debug_time_scale = 1.0;
    }

    ff::state::advance_input();
}

size_t ff::game::app_state_base::child_state_count()
{
    return this->child_state(0) ? 1 : 0;
}

ff::state* ff::game::app_state_base::child_state(size_t index)
{
    return this->debug_state->visible()
        ? static_cast<ff::state*>(this->debug_state.get())
        : static_cast<ff::state*>(this->game_state.get());
}

std::shared_ptr<ff::state> ff::game::app_state_base::create_debug_overlay_state()
{
    return {};
}

std::shared_ptr<ff::state> ff::game::app_state_base::create_initial_game_state()
{
    return {};
}

void ff::game::app_state_base::save_settings(ff::dict& dict)
{}

void ff::game::app_state_base::load_resources()
{}

void ff::game::app_state_base::load_settings(const ff::dict& dict)
{}

void ff::game::app_state_base::load_settings()
{
    ff::dict dict = ff::settings(::ID_APP_STATE);

    if (!dict.get_struct(::ID_SYSTEM_OPTIONS, this->system_options_) || this->system_options_.version != ff::game::system_options::CURRENT_VERSION)
    {
        this->system_options_ = ff::game::system_options();
    }

    this->load_settings(dict);
}

void ff::game::app_state_base::init_resources()
{
    std::vector<const ff::input_vk*> debug_input_devices{ &ff::input::keyboard(), &ff::input::pointer() };

    this->debug_input_mapping = "ff.game.debug_controls";
    this->debug_input_events = std::make_unique<ff::input_event_provider>(*this->debug_input_mapping.object(), std::move(debug_input_devices));

    this->load_resources();
}

void ff::game::app_state_base::init_game_state()
{
    std::shared_ptr<ff::state> state = this->create_initial_game_state();
    this->game_state = state ? state->wrap() : nullptr;
}

void ff::game::app_state_base::apply_system_options()
{
    ff::app_render_target().full_screen(this->system_options_.full_screen);
}

void ff::game::app_state_base::on_save_settings()
{
    this->system_options_.full_screen = ff::app_render_target().full_screen();

    ff::dict dict = ff::settings(::ID_APP_STATE);
    dict.set_struct(::ID_SYSTEM_OPTIONS, this->system_options_);

    this->save_settings(dict);
    ff::settings(::ID_APP_STATE, dict);
}

void ff::game::app_state_base::on_custom_debug()
{
    if (this->debug_state->visible())
    {
        this->pending_debug_state = 1;
    }
    else if (this->allow_debug())
    {
        this->pending_debug_state = 0;
        this->debug_state->visible(this->create_debug_overlay_state(), this->game_state);
    }
}

void ff::game::app_state_base::on_resources_rebuilt()
{
    this->rebuilding_resources = false;
    this->init_resources();
    this->reload_resources_signal.notify();
}

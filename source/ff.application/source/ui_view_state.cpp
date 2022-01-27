#include "pch.h"
#include "ui_view_state.h"

ff::ui_view_state::ui_view_state(std::shared_ptr<ff::ui_view> view)
    : view_(view)
{}

const std::shared_ptr<ff::ui_view>& ff::ui_view_state::view()
{
    return this->view_;
}

std::shared_ptr<ff::state> ff::ui_view_state::advance_time()
{
    this->view_->advance();
    return nullptr;
}

void ff::ui_view_state::render(ff::dxgi::command_context_base& context, ff::render_targets& targets)
{
    this->view_->render(context, targets.target(context, ff::render_target_type::rgba_pma), targets.depth(context));
}

ff::state::cursor_t ff::ui_view_state::cursor()
{
    switch (this->view_->cursor())
    {
    default:
        return ff::state::cursor_t::default;

    case Noesis::CursorType_Hand:
        return ff::state::cursor_t::hand;
    }
}

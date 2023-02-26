#include "pch.h"
#include "assert.h"
#include "co_task.h"
#include "thread_dispatch.h"
#include "win_handle.h"

static std::vector<HANDLE> event_pool;
static std::unordered_map<HANDLE, std::shared_ptr<int>> dupe_event_count;
static std::mutex event_mutex;

void ff::win_handle::close(HANDLE& handle)
{
    if (handle && handle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(handle);
        handle = nullptr;
    }
}

ff::win_handle ff::win_handle::duplicate(HANDLE handle)
{
    HANDLE new_handle = nullptr;
    handle_type new_type = handle_type::unknown;

    if (handle && !::DuplicateHandle(::GetCurrentProcess(), handle, ::GetCurrentProcess(), &new_handle, 0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        assert(false);
    }

    if (new_handle)
    {
        std::scoped_lock lock(::event_mutex);
        auto i = ::dupe_event_count.find(handle);
        if (i != ::dupe_event_count.end())
        {
            ++(*i->second);
            ::dupe_event_count.try_emplace(new_handle, i->second);
            new_type = handle_type::event;
        }
    }

    return win_handle(new_handle, new_type);
}

ff::win_handle::win_handle(HANDLE handle)
    : handle(handle != INVALID_HANDLE_VALUE ? handle : nullptr)
    , type(handle_type::unknown)
{}

ff::win_handle::win_handle(HANDLE handle, handle_type type)
    : handle(handle != INVALID_HANDLE_VALUE ? handle : nullptr)
    , type(type)
{}

ff::win_handle::win_handle(win_handle&& other) noexcept
    : handle(other.handle)
    , type(other.type)
{
    other.handle = nullptr;
    other.type = handle_type::unknown;
}

ff::win_handle::~win_handle()
{
    this->close();
}

ff::win_handle& ff::win_handle::operator=(win_handle&& other) noexcept
{
    this->close();
    std::swap(this->handle, other.handle);
    std::swap(this->type, other.type);
    return *this;
}

ff::internal::co_handle_awaiter ff::win_handle::operator co_await()
{
    return ff::task::wait_handle(this->handle);
}

bool ff::win_handle::operator!() const
{
    return !this->handle;
}

ff::win_handle::operator bool() const
{
    return this->handle != nullptr;
}

ff::win_handle::operator HANDLE() const
{
    return this->handle;
}

ff::win_handle ff::win_handle::duplicate() const
{
    return win_handle::duplicate(this->handle);
}

void ff::win_handle::close()
{
    if (this->handle && this->type == handle_type::event)
    {
        HANDLE handle = this->handle;
        this->handle = nullptr;
        this->type = handle_type::unknown;

        // Event pool
        {
            std::scoped_lock lock(::event_mutex);
            auto i = ::dupe_event_count.find(handle);
            if (!--(*i->second))
            {
                ::event_pool.push_back(handle);
                handle = nullptr;
            }
            else
            {
                ::dupe_event_count.erase(i);
            }
        }

        if (handle)
        {
            win_handle::close(handle);
            handle = nullptr;
        }
    }
    else
    {
        win_handle::close(this->handle);
        this->type = handle_type::unknown;
    }
}

bool ff::win_handle::wait(size_t timeout_ms)
{
    return ff::wait_for_handle(this->handle, timeout_ms);
}

bool ff::win_handle::is_set() const
{
    return this->handle && ::WaitForSingleObjectEx(this->handle, 0, FALSE) == WAIT_OBJECT_0;
}

#if !UWP_APP

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
HINSTANCE ff::get_hinstance()
{
    HINSTANCE instance = reinterpret_cast<HINSTANCE>(&__ImageBase);
    return instance ? instance : ::GetModuleHandle(nullptr);
}

#endif

ff::win_handle ff::win_handle::create_event(bool initial_set)
{
    HANDLE handle{};

    if (::event_pool.size())
    {
        std::scoped_lock lock(::event_mutex);
        if (::event_pool.size())
        {
            handle = ::event_pool.back();
            ::event_pool.pop_back();

            auto i = ::dupe_event_count.find(handle);
            ++(*i->second);
        }
    }

    if (handle)
    {
        if (initial_set)
        {
            ::SetEvent(handle);
        }
        else
        {
            ::ResetEvent(handle);
        }
    }
    else
    {
        handle = ::CreateEventEx(nullptr, nullptr,
            (initial_set ? CREATE_EVENT_INITIAL_SET : 0) | CREATE_EVENT_MANUAL_RESET,
            EVENT_ALL_ACCESS);
    }

    return ff::win_handle(handle, handle_type::event);
}

const ff::win_handle& ff::win_handle::never_complete_event()
{
    static ff::win_handle handle = ff::win_handle::create_event();
    return handle;
}

const ff::win_handle& ff::win_handle::always_complete_event()
{
    static ff::win_handle handle = ff::win_handle::create_event(true);
    return handle;
}

bool ff::wait_for_event_and_reset(HANDLE handle, size_t timeout_ms)
{
    return ff::wait_for_handle(handle, timeout_ms) && ::ResetEvent(handle);
}

bool ff::wait_for_handle(HANDLE handle, size_t timeout_ms)
{
    size_t completed_index;
    return ff::wait_for_any_handle(&handle, 1, completed_index, timeout_ms) && completed_index == 0;
}

bool ff::wait_for_any_handle(const HANDLE* handles, size_t count, size_t& completed_index, size_t timeout_ms)
{
    ff::thread_dispatch* dispatch = ff::thread_dispatch::get();
    if (dispatch)
    {
        return dispatch->wait_for_any_handle(handles, count, completed_index, timeout_ms);
    }

    while (true)
    {
        DWORD result = ::WaitForMultipleObjectsEx(static_cast<DWORD>(count), handles, FALSE, static_cast<DWORD>(timeout_ms), TRUE);
        switch (result)
        {
        default:
            {
                size_t size_result = static_cast<size_t>(result);
                if (size_result < count)
                {
                    completed_index = size_result;
                    return true;
                }
                else if (result >= WAIT_ABANDONED_0 && result < WAIT_ABANDONED_0 + count)
                {
                    return false;
                }
            }
            break;

        case WAIT_TIMEOUT:
        case WAIT_FAILED:
            return false;

        case WAIT_IO_COMPLETION:
            break;
        }
    }
}

bool ff::wait_for_all_handles(const HANDLE* handles, size_t count, size_t timeout_ms)
{
    ff::thread_dispatch* dispatch = ff::thread_dispatch::get();
    if (dispatch)
    {
        return dispatch->wait_for_all_handles(handles, count, timeout_ms);
    }

    while (true)
    {
        DWORD result = ::WaitForMultipleObjectsEx(static_cast<DWORD>(count), handles, TRUE, static_cast<DWORD>(timeout_ms), TRUE);
        switch (result)
        {
            default:
                if (static_cast<size_t>(result) < count)
                {
                    return true;
                }
                else if (result >= WAIT_ABANDONED_0 && result < WAIT_ABANDONED_0 + count)
                {
                    return false;
                }
                break;

            case WAIT_TIMEOUT:
            case WAIT_FAILED:
                return false;

            case WAIT_IO_COMPLETION:
                break;
        }
    }
}

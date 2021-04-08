#include "pch.h"
#include "signal.h"

ff::signal_connection::signal_connection()
    : entry(nullptr)
{}

ff::signal_connection::signal_connection(entry_t* entry)
    : entry(nullptr)
{
    this->connect(entry);
}

ff::signal_connection::signal_connection(signal_connection&& other) noexcept
    : entry(nullptr)
{
    *this = std::move(other);
}

ff::signal_connection::~signal_connection()
{
    this->disconnect();
}

ff::signal_connection& ff::signal_connection::operator=(signal_connection&& other) noexcept
{
    if (this != &other)
    {
        this->connect(other.entry);
        other.entry = nullptr;
    }

    return *this;
}

ff::signal_connection::operator bool() const
{
    return this->entry != nullptr;
}

void ff::signal_connection::disconnect()
{
    this->connect(nullptr);
}

void ff::signal_connection::connect(entry_t* entry)
{
    if (this->entry)
    {
        this->entry->connection = nullptr;
    }

    this->entry = entry;

    if (this->entry)
    {
        this->entry->connection = this;
    }
}

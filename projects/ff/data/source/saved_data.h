#pragma once

namespace ff::data
{
    class data_base;
    class reader_base;

    enum class saved_data_type
    {
        none,
        zlib_compressed,
    };

    class saved_data_base
    {
    public:
        virtual ~saved_data_base() = 0;

        virtual std::shared_ptr<reader_base> saved_reader() const = 0;
        virtual std::shared_ptr<reader_base> loaded_reader() const = 0;
        virtual std::shared_ptr<data_base> saved_data() const = 0;
        virtual std::shared_ptr<data_base> loaded_data() const = 0;

        virtual size_t saved_size() const = 0;
        virtual size_t loaded_size() const = 0;
        virtual saved_data_type type() const = 0;
    };

    class saved_data_static : public saved_data_base
    {
    public:
        saved_data_static(const std::shared_ptr<data_base>& data, size_t loaded_size, saved_data_type type);
        saved_data_static(const saved_data_static& other);
        saved_data_static(saved_data_static&& other) noexcept;

        saved_data_static& operator=(const saved_data_static& other);
        saved_data_static& operator=(saved_data_static&& other);
        void swap(saved_data_static& other);

        virtual std::shared_ptr<reader_base> saved_reader() const override;
        virtual std::shared_ptr<reader_base> loaded_reader() const override;
        virtual std::shared_ptr<data_base> saved_data() const override;
        virtual std::shared_ptr<data_base> loaded_data() const override;

        virtual size_t saved_size() const  override;
        virtual size_t loaded_size() const  override;
        virtual saved_data_type type() const  override;

    private:
        std::shared_ptr<data_base> data;
        size_t data_loaded_size;
        saved_data_type data_type;
    };

    class saved_data_file : public saved_data_base
    {
    public:
        saved_data_file(const std::filesystem::path& path, size_t offset, size_t saved_size, size_t loaded_size, saved_data_type type);
        saved_data_file(const saved_data_file& other);
        saved_data_file(saved_data_file&& other) noexcept;

        saved_data_file& operator=(const saved_data_file& other);
        saved_data_file& operator=(saved_data_file&& other);
        void swap(saved_data_file& other);

        virtual std::shared_ptr<reader_base> saved_reader() const override;
        virtual std::shared_ptr<reader_base> loaded_reader() const override;
        virtual std::shared_ptr<data_base> saved_data() const override;
        virtual std::shared_ptr<data_base> loaded_data() const override;

        virtual size_t saved_size() const  override;
        virtual size_t loaded_size() const  override;
        virtual saved_data_type type() const  override;

    private:
        std::filesystem::path path;
        size_t data_offset;
        size_t data_saved_size;
        size_t data_loaded_size;
        saved_data_type data_type;
    };
}

namespace std
{
    void swap(const ff::data::saved_data_static& value1, const ff::data::saved_data_static& value2);
    void swap(const ff::data::saved_data_file& value1, const ff::data::saved_data_file& value2);
}

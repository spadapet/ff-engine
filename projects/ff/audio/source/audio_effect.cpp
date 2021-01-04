#include "pch.h"
#include "audio.h"
#include "audio_effect.h"
#include "audio_effect_playing.h"
#include "wav_file.h"

static ff::pool_allocator<ff::audio_effect_playing> audio_effect_pool;

static void delete_audio_effect(ff::audio_playing_base* value)
{
    ::audio_effect_pool.delete_obj(static_cast<ff::audio_effect_playing*>(value));
}

ff::audio_effect_o::audio_effect_o(
    const std::shared_ptr<ff::resource>& file_resource,
    const std::shared_ptr<ff::data_base>& data,
    const WAVEFORMATEX& format,
    size_t start,
    size_t length,
    size_t loop_start,
    size_t loop_length,
    size_t loop_count,
    float volume,
    float speed)
    : file(file_resource)
    , data_(data)
    , format_(format)
    , start(start)
    , length(length)
    , loop_start(loop_start)
    , loop_length(loop_length)
    , loop_count(loop_count)
    , volume(volume)
    , speed(speed)
{
    ff::audio::internal::add_child(this);
}

ff::audio_effect_o::~audio_effect_o()
{
    this->stop();

    ff::audio::internal::remove_child(this);
}

void ff::audio_effect_o::reset()
{
}

std::shared_ptr<ff::audio_playing_base> ff::audio_effect_o::play(bool start_now, float volume, float speed)
{
    IXAudio2* xaudio = ff::audio::internal::xaudio();
    IXAudio2Voice* xaudio_voice = ff::audio::internal::xaudio_voice(ff::audio::voice_type::effects);
    if (!xaudio || !xaudio_voice)
    {
        return nullptr;
    }

    std::shared_ptr<ff::audio_effect_playing> effect_ptr;
    {
        ff::audio_effect_playing* effect = ::audio_effect_pool.new_obj(this);
        effect_ptr = std::shared_ptr<ff::audio_effect_playing>(effect, ::delete_audio_effect);
    }

    XAUDIO2_SEND_DESCRIPTOR send{};
    send.pOutputVoice = xaudio_voice;

    XAUDIO2_VOICE_SENDS sends{};
    sends.SendCount = 1;
    sends.pSends = &send;

    IXAudio2SourceVoice* source = nullptr;
    if (FAILED(xaudio->CreateSourceVoice(&source, &this->format_, 0, XAUDIO2_DEFAULT_FREQ_RATIO, effect_ptr.get(), &sends)))
    {
        assert(false);
        return nullptr;
    }

    XAUDIO2_BUFFER buffer{};
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.AudioBytes = static_cast<DWORD>(this->data_->size());
    buffer.pAudioData = this->data_->data();
    buffer.PlayBegin = static_cast<DWORD>(this->start);
    buffer.PlayLength = static_cast<DWORD>(this->length);
    buffer.LoopBegin = static_cast<DWORD>(this->loop_start);
    buffer.LoopLength = static_cast<DWORD>(this->loop_length);
    buffer.LoopCount = static_cast<DWORD>(this->loop_count);
    buffer.pContext = nullptr;

    if (FAILED(source->SubmitSourceBuffer(&buffer)) ||
        FAILED(source->SetVolume(this->volume * volume)) ||
        FAILED(source->SetFrequencyRatio(this->speed * speed)))
    {
        assert(false);
        source->DestroyVoice();
        return nullptr;
    }

    this->playing_.push_back(effect_ptr);
    effect_ptr->init(source, start_now);
    return effect_ptr;
}

bool ff::audio_effect_o::playing() const
{
    for (const auto& i : this->playing_)
    {
        if (i->playing())
        {
            return true;
        }
    }

    return false;
}

void ff::audio_effect_o::stop()
{
    std::vector<std::shared_ptr<audio_effect_playing>> playing;
    std::swap(playing, this->playing_);

    for (const auto& i : playing)
    {
        i->clear_owner();
        i->stop();
    }
}

const WAVEFORMATEX& ff::audio_effect_o::format() const
{
    return this->format_;
}

const std::shared_ptr<ff::data_base> ff::audio_effect_o::data() const
{
    return this->data_;
}

std::shared_ptr<ff::audio_effect_playing> ff::audio_effect_o::remove_playing(audio_effect_playing* playing)
{
    for (auto i = this->playing_.cbegin(); i != this->playing_.cend(); ++i)
    {
        if ((*i).get() == playing)
        {
            std::shared_ptr<ff::audio_effect_playing> ret = *i;
            this->playing_.erase(i);
            return ret;
        }
    }

    assert(false);
    return nullptr;
}

bool ff::audio_effect_o::save_to_cache(ff::dict& dict, bool& allow_compress) const
{
    if (this->file.valid())
    {
        dict.set<ff::resource>("file", this->file.resource());
    }
    else
    {
        dict.set<ff::data_base>("data", this->data_, ff::saved_data_type::zlib_compressed);
        dict.set_struct("format", this->format_);
    }

    dict.set<size_t>("start", this->start);
    dict.set<size_t>("length", this->length);
    dict.set<size_t>("loop_start", this->loop_start);
    dict.set<size_t>("loop_length", this->loop_length);
    dict.set<size_t>("loop_count", this->loop_count);
    dict.set<float>("volume", this->volume);
    dict.set<float>("speed", this->speed);

    return true;
}

std::shared_ptr<ff::resource_object_base> ff::internal::audio_effect_factory::load_from_source(const ff::dict& dict, resource_load_context& context) const
{
    return audio_effect_factory::load_from_cache(dict);
}

std::shared_ptr<ff::resource_object_base> ff::internal::audio_effect_factory::load_from_cache(const ff::dict& dict) const
{
    std::shared_ptr<ff::data_base> data;
    WAVEFORMATEX format{};

    std::shared_ptr<ff::resource> file_resource = dict.get<ff::resource>("file");
    if (file_resource)
    {
        ff::auto_resource<ff::file_o> file(file_resource);
        std::shared_ptr<ff::saved_data_base> file_saved_data = file.object() ? file->saved_data() : nullptr;
        std::shared_ptr<ff::reader_base> reader = file_saved_data ? file_saved_data->loaded_reader() : nullptr;
        std::shared_ptr<ff::saved_data_base> wav_saved_data = reader ? ff::internal::read_wav_file(*reader, format) : nullptr;

        data = wav_saved_data ? wav_saved_data->loaded_data() : nullptr;
        if (!data)
        {
            return nullptr;
        }
    }
    else
    {
        data = dict.get<ff::data_base>("data");
        if (!data || !dict.get_struct("format", format))
        {
            return nullptr;
        }
    }

    size_t start = dict.get<size_t>("start");
    size_t length = dict.get<size_t>("length");
    size_t loop_start = dict.get<size_t>("loop_start");
    size_t loop_length = dict.get<size_t>("loop_length");
    size_t loop_count = std::min<size_t>(dict.get<int>("loop_count") >= 0 ? dict.get<size_t>("loop_count") : XAUDIO2_LOOP_INFINITE, XAUDIO2_LOOP_INFINITE);
    float volume = dict.get<float>("volume", 1.0f);
    float speed = dict.get<float>("speed", 1.0f);

    return std::make_shared<audio_effect_o>(file_resource, data, format, start, length, loop_start, loop_length, loop_count, volume, speed);
}

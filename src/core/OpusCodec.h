#pragma once

#include <QByteArray>
#include <cstdint>

struct OpusEncoder;
struct OpusDecoder;

namespace sound {

// RAII wrapper around an Opus encoder configured for VOIP, mono, 48 kHz.
class Encoder
{
public:
    explicit Encoder(int bitrate = 24000);
    ~Encoder();

    Encoder(const Encoder&) = delete;
    Encoder& operator=(const Encoder&) = delete;

    bool isValid() const { return enc_ != nullptr; }

    // Encodes exactly `frameSamples` (e.g. kFrameSamples) of mono int16 PCM.
    // Returns the compressed packet, or an empty array on failure.
    QByteArray encode(const int16_t* pcm, int frameSamples);

private:
    OpusEncoder* enc_ = nullptr;
};

// RAII wrapper around an Opus decoder (mono, 48 kHz).
class Decoder
{
public:
    Decoder();
    ~Decoder();

    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;

    bool isValid() const { return dec_ != nullptr; }

    // Decodes a packet into `pcmOut` (capacity `maxSamples`). Pass an empty
    // packet to trigger packet-loss concealment for a dropped frame.
    // Returns the number of samples written, or a negative value on error.
    int decode(const QByteArray& packet, int16_t* pcmOut, int maxSamples);

private:
    OpusDecoder* dec_ = nullptr;
};

} // namespace sound

#pragma once

#include <QAudioFormat>

namespace sound {

// Fixed audio parameters shared across capture, codec, transport and playback.
constexpr int kSampleRate     = 48000;          // Hz
constexpr int kChannels       = 1;              // mono
constexpr int kSampleBits     = 16;             // signed PCM
constexpr int kFrameSamples   = 960;            // 20 ms @ 48 kHz (per channel)
constexpr int kBytesPerSample = kSampleBits / 8;
constexpr int kFrameBytes     = kFrameSamples * kChannels * kBytesPerSample;

// Maximum samples a decoder may emit for a single packet (60 ms safety margin).
constexpr int kMaxDecodeSamples = kSampleRate * 60 / 1000;

// Returns the canonical QAudioFormat used by capture and playback.
inline QAudioFormat canonicalFormat()
{
    QAudioFormat fmt;
    fmt.setSampleRate(kSampleRate);
    fmt.setChannelCount(kChannels);
    fmt.setSampleSize(kSampleBits);
    fmt.setCodec(QStringLiteral("audio/pcm"));
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setSampleType(QAudioFormat::SignedInt);
    return fmt;
}

} // namespace sound

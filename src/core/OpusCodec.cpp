#include "OpusCodec.h"
#include "AudioFormat.h"

#include <opus.h>

namespace sound {

Encoder::Encoder(int bitrate)
{
    int err = OPUS_OK;
    enc_ = opus_encoder_create(kSampleRate, kChannels, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK || enc_ == nullptr) {
        if (enc_) {
            opus_encoder_destroy(enc_);
            enc_ = nullptr;
        }
        return;
    }
    opus_encoder_ctl(enc_, OPUS_SET_BITRATE(bitrate));
    opus_encoder_ctl(enc_, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    // Enable in-band forward error correction; helps over lossy UDP links.
    opus_encoder_ctl(enc_, OPUS_SET_INBAND_FEC(1));
    opus_encoder_ctl(enc_, OPUS_SET_PACKET_LOSS_PERC(10));
}

Encoder::~Encoder()
{
    if (enc_)
        opus_encoder_destroy(enc_);
}

QByteArray Encoder::encode(const int16_t* pcm, int frameSamples)
{
    if (!enc_ || !pcm || frameSamples <= 0)
        return {};

    // Worst-case Opus packet size for a single frame.
    QByteArray out(4000, Qt::Uninitialized);
    const int n = opus_encode(enc_, pcm, frameSamples,
                              reinterpret_cast<unsigned char*>(out.data()),
                              out.size());
    if (n < 0)
        return {};
    out.resize(n);
    return out;
}

Decoder::Decoder()
{
    int err = OPUS_OK;
    dec_ = opus_decoder_create(kSampleRate, kChannels, &err);
    if (err != OPUS_OK || dec_ == nullptr) {
        if (dec_) {
            opus_decoder_destroy(dec_);
            dec_ = nullptr;
        }
    }
}

Decoder::~Decoder()
{
    if (dec_)
        opus_decoder_destroy(dec_);
}

int Decoder::decode(const QByteArray& packet, int16_t* pcmOut, int maxSamples)
{
    if (!dec_ || !pcmOut || maxSamples <= 0)
        return -1;

    if (packet.isEmpty()) {
        // Packet-loss concealment: synthesize one standard frame.
        return opus_decode(dec_, nullptr, 0, pcmOut, kFrameSamples, 0);
    }

    return opus_decode(dec_,
                       reinterpret_cast<const unsigned char*>(packet.constData()),
                       packet.size(), pcmOut, maxSamples, 0);
}

} // namespace sound

#include "AudioEngine.h"

#include "AudioCapture.h"
#include "AudioFormat.h"
#include "AudioPlayback.h"
#include "OpusCodec.h"
#include "UdpTransport.h"

#include <vector>

namespace sound {

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
{
}

AudioEngine::~AudioEngine()
{
    stop();
}

bool AudioEngine::start(quint16 localPort,
                        const QHostAddress& peerAddr, quint16 peerPort,
                        const QAudioDeviceInfo& inputDevice,
                        const QAudioDeviceInfo& outputDevice)
{
    stop();

    encoder_   = std::make_unique<Encoder>();
    decoder_   = std::make_unique<Decoder>();
    if (!encoder_->isValid() || !decoder_->isValid()) {
        emit errorOccurred(tr("Failed to initialize Opus codec."));
        stop();
        return false;
    }

    transport_ = std::make_unique<UdpTransport>();
    if (!transport_->bind(localPort)) {
        emit errorOccurred(tr("Failed to bind UDP port %1.").arg(localPort));
        stop();
        return false;
    }
    transport_->setPeer(peerAddr, peerPort);
    connect(transport_.get(), &UdpTransport::packetReceived,
            this, &AudioEngine::onPacketReceived);

    playback_ = std::make_unique<AudioPlayback>();
    if (!playback_->start(outputDevice)) {
        emit errorOccurred(tr("Failed to open output device."));
        stop();
        return false;
    }
    connect(playback_.get(), &AudioPlayback::level, this, &AudioEngine::rxLevel);

    capture_ = std::make_unique<AudioCapture>();
    connect(capture_.get(), &AudioCapture::frameReady,
            this, &AudioEngine::onFrameCaptured);
    connect(capture_.get(), &AudioCapture::level, this, &AudioEngine::txLevel);
    if (!capture_->start(inputDevice)) {
        emit errorOccurred(tr("Failed to open input device."));
        stop();
        return false;
    }

    running_ = true;
    return true;
}

void AudioEngine::stop()
{
    running_ = false;
    capture_.reset();
    playback_.reset();
    transport_.reset();
    encoder_.reset();
    decoder_.reset();
}

void AudioEngine::onFrameCaptured(const QByteArray& pcm)
{
    if (!encoder_ || !transport_ || pcm.size() < kFrameBytes)
        return;

    const auto* samples = reinterpret_cast<const int16_t*>(pcm.constData());
    QByteArray packet = encoder_->encode(samples, kFrameSamples);
    if (!packet.isEmpty())
        transport_->send(packet);
}

void AudioEngine::onPacketReceived(quint32 /*seq*/, const QByteArray& payload)
{
    if (!decoder_ || !playback_)
        return;

    std::vector<int16_t> pcm(kMaxDecodeSamples);
    const int n = decoder_->decode(payload, pcm.data(), kMaxDecodeSamples);
    if (n <= 0)
        return;

    QByteArray frame(reinterpret_cast<const char*>(pcm.data()),
                     n * kBytesPerSample);
    playback_->writeFrame(frame);
}

} // namespace sound

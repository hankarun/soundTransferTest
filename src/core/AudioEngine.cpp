#include "AudioEngine.h"

#include "AudioCapture.h"
#include "AudioFormat.h"
#include "AudioPlayback.h"
#include "OpusCodec.h"
#include "UdpTransport.h"

#include <QTimer>
#include <vector>

namespace {
// UDP (8) + IPv4 (20) header bytes added per packet on the wire.
constexpr int kPerPacketOverhead = 28;
}

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

    // In Opus mode the receive path needs a decoder; raw mode plays PCM directly.
    if (mode_ == Mode::Opus) {
        decoder_ = std::make_unique<Decoder>();
        if (!decoder_->isValid()) {
            emit errorOccurred(tr("Failed to initialize Opus decoder."));
            stop();
            return false;
        }
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

    // The send path (capture + encode) is optional: with no microphone the
    // engine runs receive-only and still plays incoming audio.
    const QAudioDeviceInfo resolvedInput =
        inputDevice.isNull() ? QAudioDeviceInfo::defaultInputDevice() : inputDevice;

    if (resolvedInput.isNull()) {
        emit receiveOnly();
    } else {
        capture_ = std::make_unique<AudioCapture>();
        connect(capture_.get(), &AudioCapture::frameReady,
                this, &AudioEngine::onFrameCaptured);
        connect(capture_.get(), &AudioCapture::level, this, &AudioEngine::txLevel);

        bool encoderReady = true;
        if (mode_ == Mode::Opus) {
            encoder_ = std::make_unique<Encoder>();
            encoderReady = encoder_->isValid();
        }

        if (!encoderReady || !capture_->start(resolvedInput)) {
            // A microphone exists but could not be opened: degrade gracefully
            // to receive-only rather than failing the whole session.
            capture_.reset();
            encoder_.reset();
            emit errorOccurred(tr("Could not open the microphone; "
                                  "continuing in receive-only mode."));
            emit receiveOnly();
        }
    }

    // Sample traffic counters once per second to report bandwidth.
    lastBytesSent_ = lastBytesReceived_ = 0;
    lastPacketsSent_ = lastPacketsReceived_ = 0;
    if (!statsTimer_) {
        statsTimer_ = new QTimer(this);
        connect(statsTimer_, &QTimer::timeout, this, &AudioEngine::sampleBandwidth);
    }
    statsClock_.start();
    statsTimer_->start(1000);

    running_ = true;
    return true;
}

void AudioEngine::sampleBandwidth()
{
    if (!transport_)
        return;

    const qint64 elapsedMs = statsClock_.restart();
    if (elapsedMs <= 0)
        return;
    const double seconds = elapsedMs / 1000.0;

    const quint64 sent = transport_->bytesSent();
    const quint64 recv = transport_->bytesReceived();
    const quint64 psent = transport_->packetsSent();
    const quint64 precv = transport_->packetsReceived();

    // On-wire bytes = datagram bytes + per-packet UDP/IP header overhead.
    const quint64 txWire = (sent - lastBytesSent_) +
                           (psent - lastPacketsSent_) * kPerPacketOverhead;
    const quint64 rxWire = (recv - lastBytesReceived_) +
                           (precv - lastPacketsReceived_) * kPerPacketOverhead;

    lastBytesSent_ = sent;
    lastBytesReceived_ = recv;
    lastPacketsSent_ = psent;
    lastPacketsReceived_ = precv;

    // bytes/interval -> kbit/s
    const double txKbps = (txWire * 8.0) / seconds / 1000.0;
    const double rxKbps = (rxWire * 8.0) / seconds / 1000.0;
    emit bandwidth(txKbps, rxKbps);
}

void AudioEngine::stop()
{
    running_ = false;
    if (statsTimer_)
        statsTimer_->stop();
    capture_.reset();
    playback_.reset();
    transport_.reset();
    encoder_.reset();
    decoder_.reset();
}

void AudioEngine::onFrameCaptured(const QByteArray& pcm)
{
    if (!transport_ || pcm.size() < kFrameBytes)
        return;

    if (mode_ == Mode::Raw) {
        // Send uncompressed PCM straight to the peer.
        transport_->send(pcm.left(kFrameBytes));
        return;
    }

    if (!encoder_)
        return;
    const auto* samples = reinterpret_cast<const int16_t*>(pcm.constData());
    QByteArray packet = encoder_->encode(samples, kFrameSamples);
    if (!packet.isEmpty())
        transport_->send(packet);
}

void AudioEngine::onPacketReceived(quint32 /*seq*/, const QByteArray& payload)
{
    if (!playback_)
        return;

    if (mode_ == Mode::Raw) {
        // Payload is raw PCM; play it directly.
        playback_->writeFrame(payload);
        return;
    }

    if (!decoder_)
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

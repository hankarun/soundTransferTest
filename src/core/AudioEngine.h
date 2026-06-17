#pragma once

#include <QAudioDeviceInfo>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QObject>
#include <memory>

class QTimer;

namespace sound {

class AudioCapture;
class AudioPlayback;
class UdpTransport;
class Encoder;
class Decoder;

// Ties the full duplex voice path together:
//   capture -> encode -> UDP send
//   UDP recv -> decode -> playback
class AudioEngine : public QObject
{
    Q_OBJECT
public:
    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine() override;

    // Transport payload format. Both ends must use the same mode — it is not
    // negotiated on the wire.
    //   Opus - Opus-compressed frames.
    //   Raw  - uncompressed PCM frames.
    //   Zip  - PCM frames compressed with Qt's zlib (qCompress/qUncompress).
    enum class Mode { Opus, Raw, Zip };

    // Selects the mode for the next start(). Has no effect while running.
    void setMode(Mode mode) { mode_ = mode; }
    Mode mode() const { return mode_; }

    // Opens devices, binds the local UDP port and targets the peer.
    bool start(quint16 localPort,
               const QHostAddress& peerAddr, quint16 peerPort,
               const QAudioDeviceInfo& inputDevice = QAudioDeviceInfo::defaultInputDevice(),
               const QAudioDeviceInfo& outputDevice = QAudioDeviceInfo::defaultOutputDevice());
    void stop();
    bool isRunning() const { return running_; }
    bool isCapturing() const { return capture_ != nullptr; }

signals:
    void txLevel(qreal rms);   // microphone level
    void rxLevel(qreal rms);   // playback level
    void receiveOnly();        // no usable microphone; running output-only
    // Estimated on-wire bandwidth in kbit/s (incl. UDP/IP header overhead).
    void bandwidth(double txKbps, double rxKbps);
    void errorOccurred(const QString& message);

private slots:
    void onFrameCaptured(const QByteArray& pcm);
    void onPacketReceived(quint32 seq, const QByteArray& payload);
    void sampleBandwidth();

private:
    std::unique_ptr<AudioCapture>  capture_;
    std::unique_ptr<AudioPlayback> playback_;
    std::unique_ptr<UdpTransport>  transport_;
    std::unique_ptr<Encoder>       encoder_;
    std::unique_ptr<Decoder>       decoder_;
    Mode mode_    = Mode::Opus;
    bool running_ = false;

    QTimer*       statsTimer_ = nullptr;
    QElapsedTimer statsClock_;
    quint64       lastBytesSent_       = 0;
    quint64       lastBytesReceived_   = 0;
    quint64       lastPacketsSent_     = 0;
    quint64       lastPacketsReceived_ = 0;
};

} // namespace sound

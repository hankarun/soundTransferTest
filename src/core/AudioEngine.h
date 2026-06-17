#pragma once

#include <QAudioDeviceInfo>
#include <QHostAddress>
#include <QObject>
#include <memory>

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

    // Opens devices, binds the local UDP port and targets the peer.
    bool start(quint16 localPort,
               const QHostAddress& peerAddr, quint16 peerPort,
               const QAudioDeviceInfo& inputDevice = QAudioDeviceInfo::defaultInputDevice(),
               const QAudioDeviceInfo& outputDevice = QAudioDeviceInfo::defaultOutputDevice());
    void stop();
    bool isRunning() const { return running_; }

signals:
    void txLevel(qreal rms);   // microphone level
    void rxLevel(qreal rms);   // playback level
    void errorOccurred(const QString& message);

private slots:
    void onFrameCaptured(const QByteArray& pcm);
    void onPacketReceived(quint32 seq, const QByteArray& payload);

private:
    std::unique_ptr<AudioCapture>  capture_;
    std::unique_ptr<AudioPlayback> playback_;
    std::unique_ptr<UdpTransport>  transport_;
    std::unique_ptr<Encoder>       encoder_;
    std::unique_ptr<Decoder>       decoder_;
    bool running_ = false;
};

} // namespace sound

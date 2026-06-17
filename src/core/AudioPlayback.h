#pragma once

#include <QAudioDeviceInfo>
#include <QByteArray>
#include <QObject>
#include <memory>

class QAudioOutput;
class QIODevice;

namespace sound {

// Plays mono 16-bit PCM frames through an output device.
class AudioPlayback : public QObject
{
    Q_OBJECT
public:
    explicit AudioPlayback(QObject* parent = nullptr);
    ~AudioPlayback() override;

    static QList<QAudioDeviceInfo> availableDevices();

    bool start(const QAudioDeviceInfo& device = QAudioDeviceInfo::defaultOutputDevice());
    void stop();
    bool isActive() const;

public slots:
    // Queues a PCM frame for playback.
    void writeFrame(const QByteArray& pcm);

signals:
    void level(qreal rms);

private:
    std::unique_ptr<QAudioOutput> output_;
    QIODevice* io_ = nullptr;   // owned by output_
};

} // namespace sound

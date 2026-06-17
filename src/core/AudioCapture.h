#pragma once

#include <QAudioDeviceInfo>
#include <QByteArray>
#include <QObject>
#include <memory>

class QAudioInput;
class QIODevice;

namespace sound {

// Captures mono 16-bit PCM from an input device and emits fixed-size frames
// (kFrameBytes each) suitable for Opus encoding.
class AudioCapture : public QObject
{
    Q_OBJECT
public:
    explicit AudioCapture(QObject* parent = nullptr);
    ~AudioCapture() override;

    static QList<QAudioDeviceInfo> availableDevices();

    // Starts capturing from `device` (default device if not specified).
    bool start(const QAudioDeviceInfo& device = QAudioDeviceInfo::defaultInputDevice());
    void stop();
    bool isActive() const;

signals:
    // Emitted once per complete kFrameSamples frame of PCM.
    void frameReady(const QByteArray& pcm);
    // RMS level in [0,1] for UI metering.
    void level(qreal rms);

private slots:
    void onReadyRead();

private:
    std::unique_ptr<QAudioInput> input_;
    QIODevice*  io_ = nullptr;   // owned by input_
    QByteArray  buffer_;          // accumulates partial frames
};

} // namespace sound

#include "AudioPlayback.h"
#include "AudioFormat.h"

#include <QAudioOutput>
#include <QtMath>

namespace sound {

AudioPlayback::AudioPlayback(QObject* parent)
    : QObject(parent)
{
}

AudioPlayback::~AudioPlayback()
{
    stop();
}

QList<QAudioDeviceInfo> AudioPlayback::availableDevices()
{
    return QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
}

bool AudioPlayback::start(const QAudioDeviceInfo& device)
{
    stop();

    QAudioFormat fmt = canonicalFormat();
    QAudioDeviceInfo dev = device.isNull() ? QAudioDeviceInfo::defaultOutputDevice() : device;
    if (!dev.isFormatSupported(fmt))
        fmt = dev.nearestFormat(fmt);

    output_ = std::make_unique<QAudioOutput>(dev, fmt);
    // A few frames of buffer to absorb network jitter.
    output_->setBufferSize(kFrameBytes * 8);
    io_ = output_->start();
    if (!io_) {
        output_.reset();
        return false;
    }
    return true;
}

void AudioPlayback::stop()
{
    if (output_) {
        output_->stop();
        output_.reset();
    }
    io_ = nullptr;
}

bool AudioPlayback::isActive() const
{
    return output_ && output_->state() != QAudio::StoppedState;
}

void AudioPlayback::writeFrame(const QByteArray& pcm)
{
    if (!io_ || pcm.isEmpty())
        return;

    io_->write(pcm);

    const int n = pcm.size() / kBytesPerSample;
    const auto* samples = reinterpret_cast<const int16_t*>(pcm.constData());
    double acc = 0.0;
    for (int i = 0; i < n; ++i) {
        const double s = samples[i] / 32768.0;
        acc += s * s;
    }
    if (n > 0)
        emit level(std::sqrt(acc / n));
}

} // namespace sound

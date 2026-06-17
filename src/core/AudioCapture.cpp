#include "AudioCapture.h"
#include "AudioFormat.h"

#include <QAudioInput>
#include <QtMath>

namespace sound {

AudioCapture::AudioCapture(QObject* parent)
    : QObject(parent)
{
}

AudioCapture::~AudioCapture()
{
    stop();
}

QList<QAudioDeviceInfo> AudioCapture::availableDevices()
{
    return QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
}

bool AudioCapture::start(const QAudioDeviceInfo& device)
{
    stop();

    QAudioFormat fmt = canonicalFormat();
    QAudioDeviceInfo dev = device.isNull() ? QAudioDeviceInfo::defaultInputDevice() : device;
    if (!dev.isFormatSupported(fmt))
        fmt = dev.nearestFormat(fmt);

    input_ = std::make_unique<QAudioInput>(dev, fmt);
    io_ = input_->start();
    if (!io_) {
        input_.reset();
        return false;
    }
    buffer_.clear();
    connect(io_, &QIODevice::readyRead, this, &AudioCapture::onReadyRead);
    return true;
}

void AudioCapture::stop()
{
    if (input_) {
        input_->stop();
        input_.reset();
    }
    io_ = nullptr;
    buffer_.clear();
}

bool AudioCapture::isActive() const
{
    return input_ && input_->state() == QAudio::ActiveState;
}

void AudioCapture::onReadyRead()
{
    if (!io_)
        return;

    buffer_.append(io_->readAll());

    while (buffer_.size() >= kFrameBytes) {
        QByteArray frame = buffer_.left(kFrameBytes);
        buffer_.remove(0, kFrameBytes);

        // Compute RMS for metering.
        const auto* samples = reinterpret_cast<const int16_t*>(frame.constData());
        double acc = 0.0;
        for (int i = 0; i < kFrameSamples; ++i) {
            const double s = samples[i] / 32768.0;
            acc += s * s;
        }
        emit level(std::sqrt(acc / kFrameSamples));
        emit frameReady(frame);
    }
}

} // namespace sound

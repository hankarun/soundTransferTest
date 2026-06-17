#include "OpusCodec.h"
#include "UdpTransport.h"
#include "AudioFormat.h"
#include "TestSignal.h"

#include <QtTest>
#include <vector>

using namespace sound;

// Exercises the full transmission path WITHOUT physical audio devices:
//   PCM -> Encoder -> UdpTransport (loopback) -> Decoder -> PCM
class TestTransmission : public QObject
{
    Q_OBJECT
private slots:
    void endToEndOverLoopback();
    void recoversFromDroppedPacket();
    void rawPcmLoopbackIsLossless();
    void zipPcmLoopbackIsLossless();
};

void TestTransmission::endToEndOverLoopback()
{
    Encoder enc;
    Decoder dec;
    QVERIFY(enc.isValid());
    QVERIFY(dec.isValid());

    UdpTransport sender;
    UdpTransport receiver;
    QVERIFY(sender.bind(0));
    QVERIFY(receiver.bind(0));
    sender.setPeer(QHostAddress::LocalHost, receiver.localPort());

    // 1 second of voice-band tone => 50 frames of 20 ms.
    const int totalFrames = sound::kSampleRate / sound::kFrameSamples;
    const auto frame = testutil::sine(sound::kFrameSamples, 440.0);

    std::vector<int16_t> output;
    int decodedFrames = 0;
    connect(&receiver, &UdpTransport::packetReceived,
            [&](quint32, const QByteArray& payload) {
                std::vector<int16_t> pcm(sound::kMaxDecodeSamples);
                const int n = dec.decode(payload, pcm.data(), sound::kMaxDecodeSamples);
                if (n > 0) {
                    output.insert(output.end(), pcm.begin(), pcm.begin() + n);
                    ++decodedFrames;
                }
            });

    for (int f = 0; f < totalFrames; ++f) {
        const QByteArray packet = enc.encode(frame.data(), sound::kFrameSamples);
        QVERIFY(!packet.isEmpty());
        QVERIFY(sender.send(packet) > 0);
    }

    // Wait for all datagrams to arrive over the loopback interface.
    QTRY_COMPARE(decodedFrames, totalFrames);
    QCOMPARE(int(output.size()), totalFrames * sound::kFrameSamples);

    // Reference signal of equal length.
    std::vector<int16_t> ref;
    for (int f = 0; f < totalFrames; ++f)
        ref.insert(ref.end(), frame.begin(), frame.end());

    const double rIn  = testutil::rms(ref);
    const double rOut = testutil::rms(output);
    QVERIFY2(std::abs(rIn - rOut) < 0.1,
             qPrintable(QString("RMS in=%1 out=%2").arg(rIn).arg(rOut)));

    const double corr = testutil::bestCorrelation(ref, output);
    QVERIFY2(corr > 0.9,
             qPrintable(QString("end-to-end correlation=%1").arg(corr)));
}

void TestTransmission::recoversFromDroppedPacket()
{
    Encoder enc;
    Decoder dec;

    const int totalFrames = 20;
    const auto frame = testutil::sine(sound::kFrameSamples, 660.0);

    int decodedFrames = 0;
    for (int f = 0; f < totalFrames; ++f) {
        const QByteArray packet = enc.encode(frame.data(), sound::kFrameSamples);
        QVERIFY(!packet.isEmpty());

        std::vector<int16_t> pcm(sound::kMaxDecodeSamples);

        // Simulate a lost datagram in the middle of the stream: feed the
        // decoder an empty packet (packet-loss concealment) instead.
        const QByteArray delivered = (f == 10) ? QByteArray() : packet;
        const int n = dec.decode(delivered, pcm.data(), sound::kMaxDecodeSamples);
        QVERIFY(n > 0); // a frame is always produced, including the concealed one
        ++decodedFrames;
    }

    QCOMPARE(decodedFrames, totalFrames); // stream survived the drop
}

void TestTransmission::rawPcmLoopbackIsLossless()
{
    // Raw mode sends uncompressed PCM frames directly; over the loopback the
    // received bytes must match the source exactly (no codec, no loss).
    UdpTransport sender;
    UdpTransport receiver;
    QVERIFY(sender.bind(0));
    QVERIFY(receiver.bind(0));
    sender.setPeer(QHostAddress::LocalHost, receiver.localPort());

    const int totalFrames = 20;
    const auto pcm = testutil::sine(sound::kFrameSamples, 440.0);
    const QByteArray frame = testutil::toBytes(pcm);
    QCOMPARE(frame.size(), sound::kFrameBytes);

    QVector<QByteArray> received;
    connect(&receiver, &UdpTransport::packetReceived,
            [&](quint32, const QByteArray& payload) { received.append(payload); });

    for (int f = 0; f < totalFrames; ++f)
        QVERIFY(sender.send(frame) > 0);

    QTRY_COMPARE(received.size(), totalFrames);
    for (const auto& got : received)
        QCOMPARE(got, frame); // byte-identical, lossless
}

void TestTransmission::zipPcmLoopbackIsLossless()
{
    // Zip mode sends qCompress'd PCM; after qUncompress on the far end the
    // bytes must match the source exactly. (Note: zlib does not necessarily
    // shrink high-entropy 16-bit PCM — the point of this mode is lossless
    // transport, not guaranteed size reduction.)
    UdpTransport sender;
    UdpTransport receiver;
    QVERIFY(sender.bind(0));
    QVERIFY(receiver.bind(0));
    sender.setPeer(QHostAddress::LocalHost, receiver.localPort());

    const int totalFrames = 20;
    const auto pcm = testutil::sine(sound::kFrameSamples, 440.0);
    const QByteArray frame = testutil::toBytes(pcm);
    const QByteArray compressed = qCompress(frame);

    QVector<QByteArray> received;
    connect(&receiver, &UdpTransport::packetReceived,
            [&](quint32, const QByteArray& payload) {
                received.append(qUncompress(payload));
            });

    for (int f = 0; f < totalFrames; ++f)
        QVERIFY(sender.send(compressed) > 0);

    QTRY_COMPARE(received.size(), totalFrames);
    for (const auto& got : received)
        QCOMPARE(got, frame); // byte-identical after decompression
}

QTEST_MAIN(TestTransmission)
#include "tst_transmission.moc"

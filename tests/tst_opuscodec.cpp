#include "OpusCodec.h"
#include "AudioFormat.h"
#include "TestSignal.h"

#include <QtTest>
#include <vector>

using namespace sound;

class TestOpusCodec : public QObject
{
    Q_OBJECT
private slots:
    void encodeProducesCompactPacket();
    void roundTripPreservesSignal();
    void silenceRoundTrip();
    void packetLossConcealment();
};

void TestOpusCodec::encodeProducesCompactPacket()
{
    Encoder enc;
    QVERIFY(enc.isValid());

    const auto pcm = testutil::sine(kFrameSamples);
    const QByteArray packet = enc.encode(pcm.data(), kFrameSamples);

    QVERIFY(!packet.isEmpty());
    QVERIFY(packet.size() < kFrameBytes); // compression actually happened
}

void TestOpusCodec::roundTripPreservesSignal()
{
    Encoder enc;
    Decoder dec;
    QVERIFY(enc.isValid());
    QVERIFY(dec.isValid());

    const auto input = testutil::sine(kFrameSamples);

    // Push several frames so the decoder reaches steady state.
    std::vector<int16_t> output;
    for (int f = 0; f < 25; ++f) {
        const QByteArray packet = enc.encode(input.data(), kFrameSamples);
        QVERIFY(!packet.isEmpty());

        std::vector<int16_t> frame(kMaxDecodeSamples);
        const int n = dec.decode(packet, frame.data(), kMaxDecodeSamples);
        QCOMPARE(n, kFrameSamples);
        output.insert(output.end(), frame.begin(), frame.begin() + n);
    }

    // Reference is the repeated input of equal length.
    std::vector<int16_t> ref;
    for (int f = 0; f < 25; ++f)
        ref.insert(ref.end(), input.begin(), input.end());

    const double rIn  = testutil::rms(ref);
    const double rOut = testutil::rms(output);
    QVERIFY2(std::abs(rIn - rOut) < 0.1,
             qPrintable(QString("RMS in=%1 out=%2").arg(rIn).arg(rOut)));

    const double corr = testutil::bestCorrelation(ref, output);
    QVERIFY2(corr > 0.9, qPrintable(QString("correlation=%1").arg(corr)));
}

void TestOpusCodec::silenceRoundTrip()
{
    Encoder enc;
    Decoder dec;

    std::vector<int16_t> silence(kFrameSamples, 0);
    std::vector<int16_t> out(kMaxDecodeSamples);

    for (int f = 0; f < 5; ++f) {
        const QByteArray packet = enc.encode(silence.data(), kFrameSamples);
        QVERIFY(!packet.isEmpty());
        const int n = dec.decode(packet, out.data(), kMaxDecodeSamples);
        QCOMPARE(n, kFrameSamples);
    }
    QVERIFY(testutil::rms(out.data(), kFrameSamples) < 0.01);
}

void TestOpusCodec::packetLossConcealment()
{
    Encoder enc;
    Decoder dec;

    const auto pcm = testutil::sine(kFrameSamples);
    std::vector<int16_t> out(kMaxDecodeSamples);

    // Prime the decoder with one real frame.
    QByteArray packet = enc.encode(pcm.data(), kFrameSamples);
    dec.decode(packet, out.data(), kMaxDecodeSamples);

    // Now simulate a lost packet: empty input triggers concealment.
    const int n = dec.decode(QByteArray(), out.data(), kMaxDecodeSamples);
    QCOMPARE(n, kFrameSamples); // a full frame is synthesized, no crash
}

QTEST_MAIN(TestOpusCodec)
#include "tst_opuscodec.moc"

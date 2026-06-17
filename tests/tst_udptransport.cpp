#include "UdpTransport.h"

#include <QtTest>
#include <QSignalSpy>
#include <vector>

using namespace sound;

class TestUdpTransport : public QObject
{
    Q_OBJECT
private slots:
    void loopbackDeliversPayloadsInOrder();
};

void TestUdpTransport::loopbackDeliversPayloadsInOrder()
{
    UdpTransport a;
    UdpTransport b;

    QVERIFY(a.bind(0)); // ephemeral ports
    QVERIFY(b.bind(0));
    QVERIFY(a.localPort() != 0);
    QVERIFY(b.localPort() != 0);

    a.setPeer(QHostAddress::LocalHost, b.localPort());

    QVector<QByteArray>  received;
    QVector<quint32>     seqs;
    connect(&b, &UdpTransport::packetReceived,
            [&](quint32 seq, const QByteArray& payload) {
                seqs.append(seq);
                received.append(payload);
            });

    const QVector<QByteArray> payloads = {
        QByteArrayLiteral("hello"),
        QByteArrayLiteral("opus-packet-2"),
        QByteArray(200, '\x7f'),
        QByteArrayLiteral("last"),
    };
    for (const auto& p : payloads)
        QCOMPARE(a.send(p), qint64(p.size()));

    QTRY_COMPARE(received.size(), payloads.size());

    for (int i = 0; i < payloads.size(); ++i) {
        QCOMPARE(received[i], payloads[i]); // byte-identical
        QCOMPARE(seqs[i], quint32(i));      // monotonically increasing
    }
}

QTEST_MAIN(TestUdpTransport)
#include "tst_udptransport.moc"

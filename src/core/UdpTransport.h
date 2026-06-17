#pragma once

#include <QHostAddress>
#include <QObject>
#include <QtGlobal>

class QUdpSocket;

namespace sound {

// Sends and receives Opus packets over UDP. Each datagram carries a 4-byte
// big-endian sequence header followed by the payload.
class UdpTransport : public QObject
{
    Q_OBJECT
public:
    explicit UdpTransport(QObject* parent = nullptr);
    ~UdpTransport() override;

    // Binds the local receive port. Returns false on failure.
    bool bind(quint16 localPort);

    // Sets the remote peer that send() targets.
    void setPeer(const QHostAddress& addr, quint16 port);

    quint16 localPort() const;

    // Cumulative traffic counters (datagram bytes = 4-byte header + payload).
    quint64 bytesSent() const { return bytesSent_; }
    quint64 bytesReceived() const { return bytesReceived_; }
    quint64 packetsSent() const { return packetsSent_; }
    quint64 packetsReceived() const { return packetsReceived_; }

public slots:
    // Prepends the sequence header and sends the payload to the peer.
    // Returns the number of payload bytes sent, or -1 on error.
    qint64 send(const QByteArray& payload);

signals:
    void packetReceived(quint32 seq, const QByteArray& payload);

private slots:
    void onReadyRead();

private:
    QUdpSocket*  socket_   = nullptr;
    QHostAddress peerAddr_;
    quint16      peerPort_ = 0;
    quint32      txSeq_    = 0;

    quint64 bytesSent_       = 0;
    quint64 bytesReceived_   = 0;
    quint64 packetsSent_     = 0;
    quint64 packetsReceived_ = 0;
};

} // namespace sound

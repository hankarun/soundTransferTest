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
};

} // namespace sound

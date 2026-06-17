#include "UdpTransport.h"

#include <QDataStream>
#include <QUdpSocket>

namespace sound {

UdpTransport::UdpTransport(QObject* parent)
    : QObject(parent)
    , socket_(new QUdpSocket(this))
{
    connect(socket_, &QUdpSocket::readyRead, this, &UdpTransport::onReadyRead);
}

UdpTransport::~UdpTransport() = default;

bool UdpTransport::bind(quint16 localPort)
{
    return socket_->bind(QHostAddress::AnyIPv4, localPort,
                         QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
}

void UdpTransport::setPeer(const QHostAddress& addr, quint16 port)
{
    peerAddr_ = addr;
    peerPort_ = port;
}

quint16 UdpTransport::localPort() const
{
    return socket_->localPort();
}

qint64 UdpTransport::send(const QByteArray& payload)
{
    if (peerAddr_.isNull() || peerPort_ == 0)
        return -1;

    QByteArray datagram;
    datagram.reserve(payload.size() + 4);
    {
        QDataStream out(&datagram, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::BigEndian);
        out << txSeq_++;
    }
    datagram.append(payload);

    const qint64 sent = socket_->writeDatagram(datagram, peerAddr_, peerPort_);
    if (sent < 0)
        return -1;
    bytesSent_ += quint64(sent);
    ++packetsSent_;
    return sent - 4;
}

void UdpTransport::onReadyRead()
{
    while (socket_->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(socket_->pendingDatagramSize()));
        socket_->readDatagram(datagram.data(), datagram.size());

        bytesReceived_ += quint64(datagram.size());
        ++packetsReceived_;

        if (datagram.size() < 4)
            continue; // malformed

        quint32 seq = 0;
        {
            QDataStream in(datagram);
            in.setByteOrder(QDataStream::BigEndian);
            in >> seq;
        }
        emit packetReceived(seq, datagram.mid(4));
    }
}

} // namespace sound

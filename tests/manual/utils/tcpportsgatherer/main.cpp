#include <QCoreApplication>
#include <utils/tcpportsgatherer.h>
#include <QDebug>
#include <QStringList>

using namespace Utils;
int main()
{
    qDebug() << "Used TCP Ports (IP4):";

    TcpPortsGatherer portsGatherer;
    portsGatherer.update(QAbstractSocket::IPv4Protocol);
    qDebug() << portsGatherer.usedPorts();

    qDebug() << "Used TCP Ports (IP6):";
    portsGatherer.update(QAbstractSocket::IPv6Protocol);
    qDebug() << portsGatherer.usedPorts();

    qDebug() << "All Used TCP Ports:";
    portsGatherer.update(QAbstractSocket::UnknownNetworkLayerProtocol);
    qDebug() << portsGatherer.usedPorts();

    qDebug() << "Getting a few ports ...";
    PortList portList = PortList::fromString(QLatin1String("10000-10100"));
    QStringList ports;
    for (int i = 0; i < 10; ++i) {
        quint16 port = portsGatherer.getNextFreePort(&portList);
        Q_ASSERT(!portsGatherer.usedPorts().contains(port));
        Q_ASSERT(port >= 10000);
        Q_ASSERT(port < 10100);
        QString portStr = QString::number(port);
        Q_ASSERT(!ports.contains(portStr));
        ports.append(QString::number(port));
    }
    qDebug() << ports.join(QLatin1String(", "));
    return 0;
}

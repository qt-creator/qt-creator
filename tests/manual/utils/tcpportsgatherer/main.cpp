#include <QCoreApplication>
#include <utils/tcpportsgatherer.h>
#include <QDebug>
#include <QStringList>

using namespace Utils;
int main()
{
    qDebug() << "Used TCP Ports (IP4):";

    TcpPortsGatherer ip4Ports(TcpPortsGatherer::IPv4Protocol);
    qDebug() << ip4Ports.usedPorts().toString();

    qDebug() << "Used TCP Ports (IP6):";
    TcpPortsGatherer ip6Ports(TcpPortsGatherer::IPv6Protocol);
    qDebug() << ip6Ports.usedPorts().toString();

    qDebug() << "All Used TCP Ports:";
    TcpPortsGatherer ipPorts(TcpPortsGatherer::AnyIPProcol);
    qDebug() << ipPorts.usedPorts().toString();

    qDebug() << "Getting a few ports ...";
    PortList portList = PortList::fromString(QLatin1String("10000-10100"));
    QStringList ports;
    for (int i = 0; i < 10; ++i) {
        quint16 port = ipPorts.getNextFreePort(&portList);
        Q_ASSERT(!ipPorts.usedPorts().contains(port));
        Q_ASSERT(port >= 10000);
        Q_ASSERT(port < 10100);
        QString portStr = QString::number(port);
        Q_ASSERT(!ports.contains(portStr));
        ports.append(QString::number(port));
    }
    qDebug() << ports.join(QLatin1String(", "));
    return 0;
}

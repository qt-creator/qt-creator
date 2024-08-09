#include "%{FeatureLowerCase}backend.h"

#include <QLoggingCategory>
#include <QtNumeric>
#include "%{ProjectNameLowerCase}module.h"

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(qLc%{Feature}, "%{ProjectNameCap}Module.%{Feature}Backend", QtDebugMsg);

%{Feature}Backend::%{Feature}Backend(QObject *parent)
    : %{Feature}BackendInterface(parent)
{
    %{ProjectNameCap}Module::registerTypes();
    @if %{SampleCode}
    @if %{Zoned}
    m_zoneMap.emplace(QString());
    @endif
    @endif
}

void %{Feature}Backend::initialize()
{
    @if %{SampleCode}
    @if %{Zoned}
    QString zone1 = "Foreground";
    QString zone2 = "Background";
    m_zoneMap.emplace(zone1);
    m_zoneMap.emplace(zone2);
    m_zones << zone1 << zone2;
    Q_EMIT availableZonesChanged(m_zones);
    Q_EMIT processedCountChanged(m_zoneMap[QString()].m_processedCount, QString());
    Q_EMIT gammaChanged(m_zoneMap[QString()].m_gamma, QString());
    Q_EMIT processedCountChanged(m_zoneMap[zone1].m_processedCount, zone1);
    Q_EMIT gammaChanged(m_zoneMap[zone1].m_gamma, zone1);
    Q_EMIT processedCountChanged(m_zoneMap[zone2].m_processedCount, zone2);
    Q_EMIT gammaChanged(m_zoneMap[zone2].m_gamma, zone2);
    @else
    Q_EMIT processedCountChanged(m_data.m_processedCount);
    Q_EMIT gammaChanged(m_data.m_gamma);
    @endif
    @endif
    Q_EMIT initializationDone();
    qCDebug(qLc%{Feature}) << "%{Feature} Production Backend initialized";
}
@if %{Zoned}

QStringList %{Feature}Backend::availableZones() const
{
    @if %{SampleCode}
    return m_zones;
    @else
    qCDebug(qLc%{Feature}) << "Zones not implemented";
    return QStringList();
    @endif
}
@endif
@if %{SampleCode}
@if %{Zoned}

void %{Feature}Backend::setGamma(qreal gamma, const QString &zone)
{
    if (!m_zoneMap.contains(zone)) {
        qCDebug(qLc%{Feature}) << "setGamma() called with invalid zone: " << zone;
        return;
    }
    if (m_zoneMap[zone].m_gamma != gamma) {
        m_zoneMap[zone].m_gamma = gamma;
        Q_EMIT gammaChanged(gamma, zone);
    }
}
@else

void %{Feature}Backend::setGamma(qreal gamma)
{
    if (m_data.m_gamma != gamma) {
        m_data.m_gamma = gamma;
        Q_EMIT gammaChanged(gamma);
    }
}
@endif
@if %{Zoned}

QIfPendingReply<%{ProjectNameCap}Module::Status> %{Feature}Backend::processColor(%{ProjectNameCap}Module::ColorBitss egaColor, const QString &zone)
{
    if (!m_zoneMap.contains(zone)) {
        qCDebug(qLc%{Feature}) << "processColor() called with invalid zone: " << zone;
        return QIfPendingReply<%{ProjectNameCap}Module::Status>::createFailedReply();
    }
    qCDebug(qLc%{Feature}) << "%{Feature} Production Backend processColor() called on zone " << zone;
    qreal adjust = m_zoneMap[zone].m_gamma;
@else

QIfPendingReply<%{ProjectNameCap}Module::Status> %{Feature}Backend::processColor(%{ProjectNameCap}Module::ColorBitss egaColor)
{
    qCDebug(qLc%{Feature}) << "%{Feature} Production Backend processColor() called";
    qreal adjust = m_data.m_gamma;
@endif
    QIfPendingReply<%{ProjectNameCap}Module::Status> ifReply = %{ProjectNameCap}Module::Ok;
    if (adjust < 0.0)
        return QIfPendingReply<%{ProjectNameCap}Module::Status>::createFailedReply();

    Color color;
    int highValue = egaColor & %{ProjectNameCap}Module::Intensity ? 255 : 170;
    int lowValue = egaColor & %{ProjectNameCap}Module::Intensity ? 85 : 0;
    color.setRed(egaColor & %{ProjectNameCap}Module::Red ? highValue : lowValue);
    color.setGreen(egaColor & %{ProjectNameCap}Module::Green ? highValue : lowValue);
    color.setBlue(egaColor & %{ProjectNameCap}Module::Blue ? highValue : lowValue);
    if (egaColor == 6)
        color.setGreen(85);

    color.setRed(qMin(255, qRound(color.red() * adjust)));
    color.setGreen(qMin(255, qRound(color.green() * adjust)));
    color.setBlue(qMin(255, qRound(color.blue() * adjust)));
    color.setHtmlCode(QString("#%1%2%3")
        .arg(color.red(), 2, 16, QChar('0'))
        .arg(color.green(), 2, 16, QChar('0'))
        .arg(color.blue(), 2, 16, QChar('0')));

    @if %{Zoned}
    Q_EMIT colorProcessed(color, zone);
    m_zoneMap[zone].m_processedCount += 1;
    Q_EMIT processedCountChanged(m_zoneMap[zone].m_processedCount, zone);
    @else
    Q_EMIT colorProcessed(color);
    m_data.m_processedCount += 1;
    Q_EMIT processedCountChanged(m_data.m_processedCount);
    @endif
    return ifReply;
}
@endif

#include "moc_%{FeatureLowerCase}backend.cpp"

#include "%{FeatureLowerCase}adapter.h"

using namespace Qt::StringLiterals;

%{Feature}Adapter::%{Feature}Adapter(%{Feature}Backend *parent) :
%{Feature}Source(parent),
m_backend(parent)
{
    @if %{SampleCode}
    @if %{Zoned}
    m_zoneMap.emplace(QString());
    connect(m_backend, &%{Feature}Backend::availableZonesChanged, this, [this](const QStringList &zones) {
        for (QString zone : zones)
            m_zoneMap.emplace(zone);
    });

    @endif
    @if %{Zoned}
    connect(m_backend, &%{Feature}Backend::processedCountChanged, this, [this](int processedCount, const QString &zone) {
        m_zoneMap[zone].m_processedCount = processedCount;
        Q_EMIT processedCountChanged(processedCount, zone);
    });
    connect(m_backend, &%{Feature}Backend::gammaChanged, this, [this](qreal gamma, const QString &zone) {
        m_zoneMap[zone].m_gamma = gamma;
        Q_EMIT gammaChanged(gamma, zone);
    });
    @else
    connect(m_backend, &%{Feature}Backend::processedCountChanged, this, [this](int processedCount) {
        m_data.m_processedCount = processedCount;
        Q_EMIT processedCountChanged(processedCount);
    });
    connect(m_backend, &%{Feature}Backend::gammaChanged, this, [this](qreal gamma) {
        m_data.m_gamma = gamma;
        Q_EMIT gammaChanged(gamma);
    });
    @endif
    connect(m_backend, &%{Feature}Backend::colorProcessed, this, &%{Feature}Adapter::colorProcessed);
    @endif
    m_backend->initialize();
}
@if %{SampleCode}

@if %{Zoned}
int %{Feature}Adapter::processedCount(const QString & zone)
{
    auto it = m_zoneMap.find(zone);
    return it != m_zoneMap.end() ? it->m_processedCount : -1;
}

qreal %{Feature}Adapter::gamma(const QString & zone)
{
    auto it = m_zoneMap.find(zone);
    return it != m_zoneMap.end() ? it->m_gamma : -1.0;
}

void %{Feature}Adapter::setGamma(qreal gamma, const QString & zone)
{
    m_backend->setGamma(gamma, zone);
}

QVariant %{Feature}Adapter::processColor(%{ProjectNameCap}Module::ColorBitss egaColor, const QString & zone)
{
    return m_backend->processColor(egaColor, zone).value(); // always returned immediately
}
@else
int %{Feature}Adapter::processedCount() const
{
    return m_data.m_processedCount;
}

qreal %{Feature}Adapter::gamma() const
{
    return m_data.m_gamma;
}

void %{Feature}Adapter::setGamma(qreal gamma)
{
    m_backend->setGamma(gamma);
}

QVariant %{Feature}Adapter::processColor(%{ProjectNameCap}Module::ColorBitss egaColor)
{
    return m_backend->processColor(egaColor).value(); // always returned immediately
}
@endif
@endif
@if %{Zoned}

QStringList %{Feature}Adapter::availableZones()
{
    return m_backend->availableZones();
}
@endif

#include "moc_%{FeatureLowerCase}adapter.cpp"

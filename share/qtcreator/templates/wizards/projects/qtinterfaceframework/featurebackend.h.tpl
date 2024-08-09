#ifndef %{ProjectNameUpperCase}_%{FeatureUpperCase}BACKEND_H_
#define %{ProjectNameUpperCase}_%{FeatureUpperCase}BACKEND_H_

#include "%{FeatureLowerCase}backendinterface.h"

using namespace Qt::StringLiterals;
@if %{SampleCode}

struct %{Feature}Data
{
    %{Feature}Data()
    : m_processedCount(int(0))
    , m_gamma(qreal(1.0))
    {}

    int m_processedCount;
    qreal m_gamma;
};
@endif

class %{Feature}Backend : public %{Feature}BackendInterface
{
    Q_OBJECT

public:
    explicit %{Feature}Backend(QObject *parent = nullptr);
    void initialize() override;
    @if %{Zoned}
    QStringList availableZones() const override;
    @endif
@if %{SampleCode}

public Q_SLOTS:
    @if %{Zoned}
    void setGamma(qreal gamma, const QString &zone) override;
    QIfPendingReply<%{ProjectNameCap}Module::Status> processColor(%{ProjectNameCap}Module::ColorBitss egaColor, const QString &zone) override;
    @else
    void setGamma(qreal gamma) override;
    QIfPendingReply<%{ProjectNameCap}Module::Status> processColor(%{ProjectNameCap}Module::ColorBitss egaColor) override;
    @endif

private:
    @if %{Zoned}
    QHash<QString, %{Feature}Data> m_zoneMap;
    QStringList m_zones;
    @else
    %{Feature}Data m_data;
    @endif
@endif
};

#endif // %{ProjectNameUpperCase}_%{FeatureUpperCase}BACKEND_H_

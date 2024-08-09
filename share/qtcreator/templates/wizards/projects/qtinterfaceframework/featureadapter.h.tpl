#ifndef %{ProjectNameUpperCase}_%{FeatureUpperCase}ADAPTER_H_
#define %{ProjectNameUpperCase}_%{FeatureUpperCase}ADAPTER_H_

#include "%{FeatureLowerCase}backend.h"
#include "rep_%{FeatureLowerCase}_source.h"

using namespace Qt::StringLiterals;

class %{Feature}Adapter : public %{Feature}Source
{
    Q_OBJECT

public:
    %{Feature}Adapter(%{Feature}Backend *parent);
@if %{SampleCode}

public Q_SLOTS:
    @if %{Zoned}
    virtual int processedCount(const QString & zone) override;
    virtual qreal gamma(const QString & zone) override;
    virtual void setGamma(qreal gamma, const QString & zone) override;
    virtual QVariant processColor(%{ProjectNameCap}Module::ColorBitss egaColor, const QString & zone) override;
    @else
    virtual int processedCount() const override;
    virtual qreal gamma() const override;
    virtual void setGamma(qreal gamma) override;
    virtual QVariant processColor(%{ProjectNameCap}Module::ColorBitss egaColor) override;
    @endif
@endif
@if %{Zoned}
    virtual QStringList availableZones() override;
@endif

private:
    %{Feature}Backend *m_backend;
    @if %{SampleCode}
    @if %{Zoned}
    QHash<QString, %{Feature}Data> m_zoneMap;
    @else
    %{Feature}Data m_data;
    @endif
    @endif
};

#endif // %{ProjectNameUpperCase}_%{FeatureUpperCase}ADAPTER_H_

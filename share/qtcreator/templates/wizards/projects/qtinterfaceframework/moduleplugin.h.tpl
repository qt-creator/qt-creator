#ifndef %{ProjectNameUpperCase}_%{ProjectNameUpperCase}PLUGIN_H_
#define %{ProjectNameUpperCase}_%{ProjectNameUpperCase}PLUGIN_H_

#include <QtInterfaceFramework/QIfServiceInterface>
#include "%{FeatureLowerCase}backend.h"

class %{ProjectNameCap}Plugin : public QObject, QIfServiceInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QIfServiceInterface_iid FILE "%{ProjectNameLowerCase}plugin.json")
    Q_INTERFACES(QIfServiceInterface)

public:
    explicit %{ProjectNameCap}Plugin(QObject *parent = nullptr);

    QStringList interfaces() const override;
    QIfFeatureInterface* interfaceInstance(const QString& interface) const override;

    QString id() const override;
    QString configurationId() const override;
    void updateServiceSettings(const QVariantMap &settings) override;

private:
    %{Feature}Backend *m_%{Feature}Backend;
};

#endif // %{ProjectNameUpperCase}_%{ProjectNameUpperCase}PLUGIN_H_

#ifndef S60DEVICES_H
#define S60DEVICES_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>

namespace Qt4ProjectManager {
namespace Internal {

class S60Devices : public QObject
{
    Q_OBJECT
public:
    struct Device {
        QString id;
        QString name;
        bool isDefault;
        QString epocRoot;
        QString toolsRoot;
        QString qt;
    };

    S60Devices(QObject *parent = 0);
    bool read();
    QString errorString() const;
    QList<Device> devices() const;
    bool detectQtForDevices();
    Device deviceForId(const QString &id) const;

    static QString cleanedRootPath(const QString &deviceRoot);
signals:
    void qtVersionsChanged();

private:
    QString m_errorString;
    QList<Device> m_devices;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60DEVICES_H

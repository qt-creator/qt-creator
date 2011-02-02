#ifndef MAEMODEVICECONFIGWIZARD_H
#define MAEMODEVICECONFIGWIZARD_H

#include <QtCore/QScopedPointer>
#include <QtCore/QSharedPointer>
#include <QtGui/QWizard>

namespace Qt4ProjectManager {
namespace Internal {
class MaemoDeviceConfig;
struct MaemoDeviceConfigWizardPrivate;

class MaemoDeviceConfigWizard : public QWizard
{
    Q_OBJECT
public:
    explicit MaemoDeviceConfigWizard(QWidget *parent = 0);
    ~MaemoDeviceConfigWizard();
    QSharedPointer<MaemoDeviceConfig> deviceConfig() const;
    virtual int nextId() const;

private:
    const QScopedPointer<MaemoDeviceConfigWizardPrivate> d;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEVICECONFIGWIZARD_H

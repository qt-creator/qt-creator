#ifndef S60DEVICESPREFERENCEPANE_H
#define S60DEVICESPREFERENCEPANE_H

#include "s60devices.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QtCore/QPointer>
#include <QtGui/QWidget>

namespace Qt4ProjectManager {
namespace Internal {

namespace Ui {
    class S60DevicesPreferencePane;
}

class S60DevicesWidget : public QWidget
{
    Q_OBJECT
public:
    S60DevicesWidget(QWidget *parent, S60Devices *devices);
    ~S60DevicesWidget();

private slots:
    void updateDevices();

private:
    void updateDevicesList();
    Ui::S60DevicesPreferencePane *m_ui;
    S60Devices *m_devices;
};

class S60DevicesPreferencePane : public Core::IOptionsPage {
    Q_OBJECT
public:
    S60DevicesPreferencePane(S60Devices *devices, QObject *parent = 0);
    ~S60DevicesPreferencePane();

    QString id() const;
    QString trName() const;
    QString category() const;
    QString trCategory() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();

private:
    QPointer<S60DevicesWidget> m_widget;
    S60Devices *m_devices;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60DEVICESPREFERENCEPANE_H

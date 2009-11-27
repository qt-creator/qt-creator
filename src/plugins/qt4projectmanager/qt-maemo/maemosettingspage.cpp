/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Assistant of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include "maemodeviceconfigurations.h"

#include "ui_maemosettingswidget.h"
#include "maemosettingspage.h"

#include <algorithm>

namespace Qt4ProjectManager {
namespace Internal {

#define PAGE_ID "Maemo Device Configurations"

class DevConfMatcher
{
public:
    DevConfMatcher(const QString &name) : m_name(name) {}
    bool operator()(const MaemoDeviceConfigurations::DeviceConfig &devConfig)
    {
        return devConfig.name == m_name;
    }
private:
    const QString m_name;
};


class MaemoSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    MaemoSettingsWidget(QWidget *parent);
    void saveSettings();
private:
    void initGui();
    void addConfig();
    void display(const MaemoDeviceConfigurations::DeviceConfig &devConfig);

    Ui_maemoSettingsWidget *m_ui;
    QList<MaemoDeviceConfigurations::DeviceConfig> m_devConfs;
};

MaemoSettingsPage::MaemoSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent), m_widget(0)
{

}

MaemoSettingsPage::~MaemoSettingsPage()
{
}

QString MaemoSettingsPage::id() const
{
    return QLatin1String(PAGE_ID);
}

QString MaemoSettingsPage::trName() const
{
    return tr(PAGE_ID);
}

QString MaemoSettingsPage::category() const
{
    return Constants::QT_CATEGORY;
}

QString MaemoSettingsPage::trCategory() const
{
    return tr(Constants::QT_CATEGORY);
}

QWidget *MaemoSettingsPage::createPage(QWidget *parent)
{
    if (m_widget != 0)
        delete m_widget;
    m_widget = new MaemoSettingsWidget(parent);
    return m_widget;
}

void MaemoSettingsPage::apply()
{
    m_widget->saveSettings();
}

void MaemoSettingsPage::finish()
{
    apply();
}

MaemoSettingsWidget::MaemoSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_ui(new Ui_maemoSettingsWidget),
      m_devConfs(MaemoDeviceConfigurations::instance().devConfigs())

{
    initGui();
}

void MaemoSettingsWidget::initGui()
{
    m_ui->setupUi(this);
    foreach(const MaemoDeviceConfigurations::DeviceConfig &devConf, m_devConfs)
        m_ui->configListWidget->addItem(devConf.name);
}

void MaemoSettingsWidget::addConfig()
{
    QLatin1String prefix("New Device Configuration ");
    int suffix = 1;
    QString newName;
    bool isUnique = false;
    do {
        newName = prefix + QString::number(suffix++);
        isUnique = std::find_if(m_devConfs.constBegin(), m_devConfs.constEnd(),
                                DevConfMatcher(newName)) == m_devConfs.constEnd();
    } while (!isUnique);

    m_devConfs.append(MaemoDeviceConfigurations::DeviceConfig(newName));
    m_ui->configListWidget->addItem(newName);
    // TODO: select and display new item (selection should automatically lead to display)
    // also mark configuration name to suggest overwriting
}

void MaemoSettingsWidget::display(const MaemoDeviceConfigurations::DeviceConfig &devConfig)
{
    m_ui->nameLineEdit->setText(devConfig.name);
    if (devConfig.type == MaemoDeviceConfigurations::DeviceConfig::Physical)
        m_ui->deviceButton->setEnabled(true);
    else
        m_ui->simulatorButton->setEnabled(true);
    m_ui->hostLineEdit->setText(devConfig.host);
    m_ui->portLineEdit->setText(QString::number(devConfig.port));
    m_ui->timeoutLineEdit->setText(QString::number(devConfig.timeout));
    m_ui->userLineEdit->setText(devConfig.uname);
    m_ui->pwdLineEdit->setText(devConfig.pwd);
    m_ui->detailsWidget->setEnabled(true);
}

void MaemoSettingsWidget::saveSettings()
{
    MaemoDeviceConfigurations::instance().setDevConfigs(m_devConfs);
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "maemosettingspage.moc"

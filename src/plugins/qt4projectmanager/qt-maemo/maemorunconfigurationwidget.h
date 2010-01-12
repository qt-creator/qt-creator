/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAEMORUNCONFIGURATIONWIDGET_H
#define MAEMORUNCONFIGURATIONWIDGET_H

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QLineEdit;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class MaemoDeviceConfig;
class MaemoRunConfiguration;

class MaemoRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    MaemoRunConfigurationWidget(MaemoRunConfiguration *runConfiguration,
                                QWidget *parent = 0);

private slots:
    void configNameEdited(const QString &text);
    void argumentsEdited(const QString &args);
    void deviceConfigurationChanged(const QString &name);
    void resetDeviceConfigurations();
    void showSettingsDialog();

    void updateSimulatorPath();
    void updateTargetInformation();

private:
    void setSimInfoVisible(const MaemoDeviceConfig &devConf);

    QLineEdit *m_configNameLineEdit;
    QLineEdit *m_argsLineEdit;
    QLabel *m_executableLabel;
    QLabel *m_debuggerLabel;
    QComboBox *m_devConfBox;
    QLabel *m_simPathNameLabel;
    QLabel *m_simPathValueLabel;
    MaemoRunConfiguration *m_runConfiguration;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMORUNCONFIGURATIONWIDGET_H

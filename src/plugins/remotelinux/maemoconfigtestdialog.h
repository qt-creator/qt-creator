/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef MAEMOCONFIGTESTDIALOG_H
#define MAEMOCONFIGTESTDIALOG_H

#include <QtCore/QSharedPointer>
#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
class QPushButton;
class Ui_MaemoConfigTestDialog;
QT_END_NAMESPACE

namespace Utils {
    class SshRemoteProcessRunner;
} // namespace Utils

namespace RemoteLinux {
class LinuxDeviceConfiguration;

namespace Internal {
class MaemoUsedPortsGatherer;

/**
 * A dialog that runs a test of a device configuration.
 */
class MaemoConfigTestDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MaemoConfigTestDialog(const QSharedPointer<const LinuxDeviceConfiguration> &config,
        QWidget *parent = 0);
    ~MaemoConfigTestDialog();

private slots:
    void stopConfigTest();
    void processSshOutput(const QByteArray &output);
    void handleConnectionError();
    void handleTestProcessFinished(int exitStatus);
    void handlePortListReady();
    void handlePortListFailure(const QString &errMsg);

private:
    void startConfigTest();
    QString parseTestOutput();
    void handleGeneralTestResult(int exitStatus);
    void handleMadDeveloperTestResult(int exitStatus);
    void handleQmlToolingTestResult(int exitStatus);
    void testPorts();
    void finish();

    Ui_MaemoConfigTestDialog *m_ui;
    QPushButton *m_closeButton;

    const QSharedPointer<const LinuxDeviceConfiguration> m_config;
    QSharedPointer<Utils::SshRemoteProcessRunner> m_testProcessRunner;
    QString m_deviceTestOutput;
    bool m_qtVersionOk;
    MaemoUsedPortsGatherer *const m_portsGatherer;

    enum DeviceTest { GeneralTest, MadDeveloperTest, QmlToolingTest };
    DeviceTest m_currentTest;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMOCONFIGTESTDIALOG_H

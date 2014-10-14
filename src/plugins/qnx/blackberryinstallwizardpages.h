/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNX_INTERNAL_BLACKBERRYINSTALLWIZARDPAGES_H
#define QNX_INTERNAL_BLACKBERRYINSTALLWIZARDPAGES_H

#include <QWizardPage>
#include <QListWidgetItem>

#include "blackberryinstallwizard.h"

#include <utils/pathchooser.h>

QT_BEGIN_NAMESPACE
class QProcess;
class QButtonGroup;
QT_END_NAMESPACE

namespace Qnx {
namespace Internal {

class Ui_BlackBerryInstallWizardOptionPage;
class Ui_BlackBerryInstallWizardNdkPage;
class Ui_BlackBerryInstallWizardTargetPage;
class Ui_BlackBerryInstallWizardProcessPage;

class NdkPathChooser : public Utils::PathChooser
{
    Q_OBJECT
public:
    enum Mode {
        InstallMode,   // Select a valid 10.2 NDK path
        ManualMode     // Select a target bbnk-env file path
    };

    NdkPathChooser(Mode mode, QWidget *parent = 0);
    virtual bool validatePath(const QString &path, QString *errorMessage);

private:
    Mode m_mode;
};

class BlackBerryInstallWizardOptionPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit BlackBerryInstallWizardOptionPage(BlackBerryInstallerDataHandler &data, QWidget *parent = 0);
    ~BlackBerryInstallWizardOptionPage();
    void initializePage();
    bool isComplete() const;
    int nextId() const;

protected slots:
    void handleApiLevelOptionChanged();
    void handlePathChanged(const QString &envFilePath);
    void handleTargetChanged();

signals:
    void installModeChanged();

private:
    Ui_BlackBerryInstallWizardOptionPage *m_ui;
    QButtonGroup *m_buttonGroup;

    NdkPathChooser *m_envFileChooser;
    BlackBerryInstallerDataHandler &m_data;
};

class BlackBerryInstallWizardNdkPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit BlackBerryInstallWizardNdkPage(BlackBerryInstallerDataHandler &data, QWidget *parent = 0);
    ~BlackBerryInstallWizardNdkPage();
    void initializePage();
    bool isComplete() const;

protected slots:
    void setNdkPath();
    void setManualNdkPath();

private:
    Ui_BlackBerryInstallWizardNdkPage *m_ui;
    BlackBerryInstallerDataHandler &m_data;
    NdkPathChooser* m_ndkPathChooser;
    QListWidgetItem* m_manual;
    bool m_validNdkPath;
};

class BlackBerryInstallWizardTargetPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit BlackBerryInstallWizardTargetPage(BlackBerryInstallerDataHandler &data, QWidget *parent = 0);
    ~BlackBerryInstallWizardTargetPage();

    void initializePage();
    bool isComplete() const;
    bool isProcessRunning() const;

protected slots:
    void targetsListProcessFinished();
    void setTarget();

private:
    BlackBerryInstallerDataHandler &m_data;
    Ui_BlackBerryInstallWizardTargetPage *m_ui;
    bool m_isTargetValid;
    QProcess *m_targetListProcess;

    void initTargetsTreeWidget();
    void updateAvailableTargetsList();

};

class BlackBerryInstallWizardProcessPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit BlackBerryInstallWizardProcessPage(BlackBerryInstallerDataHandler &data,
                                                QWidget *parent = 0);
    ~BlackBerryInstallWizardProcessPage();

    void initializePage();
    bool isComplete() const;
    bool isProcessRunning() const;

protected slots:
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    Ui_BlackBerryInstallWizardProcessPage *m_ui;
    BlackBerryInstallerDataHandler &m_data;
    QProcess *m_targetProcess;

    void processTarget();
};

class BlackBerryInstallWizardFinalPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit BlackBerryInstallWizardFinalPage(BlackBerryInstallerDataHandler &data, QWidget *parent = 0);
    void initializePage();

signals:
    void done();

private:
    BlackBerryInstallerDataHandler &m_data;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYINSTALLWIZARDPAGES_H

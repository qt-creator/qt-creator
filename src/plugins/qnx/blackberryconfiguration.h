/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BLACKBERRYCONFIGURATIONS_H
#define BLACKBERRYCONFIGURATIONS_H

#include <qnxconstants.h>

#include <utils/environment.h>
#include <utils/fileutils.h>

#include <qtsupport/baseqtversion.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/gcctoolchain.h>

#include <QSettings>
#include <QObject>

namespace Qnx {
namespace Internal {

class BlackBerryConfig
{
    QString ndkPath;
    QString targetName;
    Utils::FileName qmakeBinaryFile;
    Utils::FileName gccCompiler;
    Utils::FileName deviceDebuger;
    Utils::FileName simulatorDebuger;
    Utils::FileName sysRoot;
    QMultiMap<QString, QString> qnxEnv;

    friend class BlackBerryConfiguration;
};

class BlackBerryConfiguration: public QObject
{
    Q_OBJECT
public:
    static BlackBerryConfiguration &instance();
    BlackBerryConfig config() const;
    Utils::FileName qmakePath() const;
    Utils::FileName gccPath() const;
    Utils::FileName deviceGdbPath() const;
    Utils::FileName simulatorGdbPath() const;
    Utils::FileName sysRoot() const;
    QMultiMap<QString, QString> qnxEnv() const;
    void setupConfiguration(const QString &ndkPath);
    QString ndkPath() const;
    QString targetName() const;
    void loadSetting();
    void clearSetting();
    void cleanConfiguration();

public slots:
    void saveSetting();

private:
    BlackBerryConfiguration(QObject *parent = 0);
    static BlackBerryConfiguration *m_instance;
    BlackBerryConfig m_config;

    bool setConfig(const QString &ndkPath);
    QtSupport::BaseQtVersion* createQtVersion();
    ProjectExplorer::GccToolChain* createGccToolChain();
    ProjectExplorer::Kit* createKit(QnxArchitecture arch, QtSupport::BaseQtVersion* qtVersion, ProjectExplorer::GccToolChain* tc);

signals:
    void updated();
};

} // namespace Internal
} // namespace Qnx

#endif // BLACKBERRYCONFIGURATIONS_H

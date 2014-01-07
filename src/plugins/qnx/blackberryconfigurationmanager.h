/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
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

#ifndef BLACKBERRYCONFIGURATIONMANAGER_H
#define BLACKBERRYCONFIGURATIONMANAGER_H

#include <utils/environment.h>
#include <utils/fileutils.h>

#include <QSettings>
#include <QObject>

namespace Qnx {
namespace Internal {

class BlackBerryConfiguration;

class BlackBerryConfigurationManager : public QObject
{
    Q_OBJECT
public:
    static BlackBerryConfigurationManager &instance();
    ~BlackBerryConfigurationManager();
    bool addConfiguration(BlackBerryConfiguration *config);
    void removeConfiguration(BlackBerryConfiguration *config);
    QList<BlackBerryConfiguration*> configurations() const;
    QList<BlackBerryConfiguration*> manualConfigurations() const;
    QList<BlackBerryConfiguration *> activeConfigurations() const;
    BlackBerryConfiguration *configurationFromEnvFile(const Utils::FileName &envFile) const;

    QString barsignerCskPath() const;
    QString idTokenPath() const;
    QString barsignerDbPath() const;
    QString defaultKeystorePath() const;
    QString defaultDebugTokenPath() const;
    void clearConfigurationSettings(BlackBerryConfiguration *config);

    QList<Utils::EnvironmentItem> defaultQnxEnv();

    void loadAutoDetectedConfigurations();

public slots:
    void loadSettings();
    void saveSettings();
    void checkToolChainConfiguration();

signals:
    void settingsLoaded();

private:
    BlackBerryConfigurationManager(QObject *parent = 0);
    static BlackBerryConfigurationManager *m_instance;
    QList<BlackBerryConfiguration*> m_configs;

    void loadManualConfigurations();
    void saveManualConfigurations();
    void saveActiveConfigurationNdkEnvPath();
    void clearInvalidConfigurations();

    QStringList activeConfigurationNdkEnvPaths();
};

} // namespace Internal
} // namespace Qnx

#endif // BLACKBERRYCONFIGURATIONMANAGER_H

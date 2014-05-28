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

namespace Utils { class PersistentSettingsWriter; }

namespace Qnx {
namespace Internal {

class BlackBerryApiLevelConfiguration;
class BlackBerryRuntimeConfiguration;
class QnxPlugin;

class BlackBerryConfigurationManager : public QObject
{
    Q_OBJECT
public:
    enum ConfigurationType {
        ApiLevel = 0x01,
        Runtime = 0x02
    };
    Q_DECLARE_FLAGS(ConfigurationTypes, ConfigurationType)

    static BlackBerryConfigurationManager *instance();
    ~BlackBerryConfigurationManager();
    bool addApiLevel(BlackBerryApiLevelConfiguration *config);
    void removeApiLevel(BlackBerryApiLevelConfiguration *config);
    bool addRuntime(BlackBerryRuntimeConfiguration *runtime);
    void removeRuntime(BlackBerryRuntimeConfiguration *runtime);
    QList<BlackBerryApiLevelConfiguration*> apiLevels() const;
    QList<BlackBerryRuntimeConfiguration *> runtimes() const;
    QList<BlackBerryApiLevelConfiguration*> manualApiLevels() const;
    QList<BlackBerryApiLevelConfiguration *> activeApiLevels() const;
    BlackBerryApiLevelConfiguration *apiLevelFromEnvFile(const Utils::FileName &envFile) const;
    BlackBerryRuntimeConfiguration *runtimeFromFilePath(const QString &path);
    BlackBerryApiLevelConfiguration *defaultApiLevel() const;

    QString barsignerCskPath() const;
    QString idTokenPath() const;
    QString barsignerDbPath() const;
    QString defaultKeystorePath() const;
    QString defaultDebugTokenPath() const;

    // returns the environment for the default API level
    QList<Utils::EnvironmentItem> defaultConfigurationEnv() const;

    void loadAutoDetectedConfigurations(QFlags<ConfigurationType> types);
    void setDefaultConfiguration(BlackBerryApiLevelConfiguration *config);

    bool newestApiLevelEnabled() const;

    void emitSettingsChanged();

#ifdef WITH_TESTS
    void initUnitTest();
#endif

public slots:
    void loadSettings();
    void saveSettings();
    void checkToolChainConfiguration();

signals:
    void settingsLoaded();
    void settingsChanged();

private:
    BlackBerryConfigurationManager(QObject *parent = 0);

    static BlackBerryConfigurationManager *m_instance;

    QList<BlackBerryApiLevelConfiguration*> m_apiLevels;
    QList<BlackBerryRuntimeConfiguration*> m_runtimes;

    BlackBerryApiLevelConfiguration *m_defaultConfiguration;

    Utils::PersistentSettingsWriter *m_writer;

    void saveConfigurations();
    void restoreConfigurations();

    void loadAutoDetectedApiLevels();
    void loadAutoDetectedRuntimes();

    void loadManualConfigurations();
    void setKitsAutoDetectionSource();

    void insertApiLevelByVersion(BlackBerryApiLevelConfiguration* apiLevel);
    void insertRuntimeByVersion(BlackBerryRuntimeConfiguration* runtime);

    friend class QnxPlugin;
};

} // namespace Internal
} // namespace Qnx

Q_DECLARE_OPERATORS_FOR_FLAGS(Qnx::Internal::BlackBerryConfigurationManager::ConfigurationTypes)

#endif // BLACKBERRYCONFIGURATIONMANAGER_H

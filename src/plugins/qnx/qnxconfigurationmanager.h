// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>

namespace Utils { class PersistentSettingsWriter; }

namespace Qnx::Internal {

class QnxConfiguration;
class QnxPlugin;

class QnxConfigurationManager: public QObject
{
    Q_OBJECT
public:
    QnxConfigurationManager();
    ~QnxConfigurationManager() override;

    static QnxConfigurationManager *instance();
    QList<QnxConfiguration*> configurations() const;
    void removeConfiguration(QnxConfiguration *config);
    bool addConfiguration(QnxConfiguration *config);
    QnxConfiguration* configurationFromEnvFile(const Utils::FilePath &envFile) const;

protected slots:
    void saveConfigs();

signals:
    void configurationsListUpdated();

private:
    QList<QnxConfiguration*> m_configurations;
    Utils::PersistentSettingsWriter *m_writer;
    void restoreConfigurations();
};

} // Qnx::Internal

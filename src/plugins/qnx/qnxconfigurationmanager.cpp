// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxconfigurationmanager.h"

#include "qnxconfiguration.h"

#include <coreplugin/icore.h>

#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace Qnx::Internal {

const QLatin1String QNXConfigDataKey("QNXConfiguration.");
const QLatin1String QNXConfigCountKey("QNXConfiguration.Count");
const QLatin1String QNXConfigsFileVersionKey("Version");

static FilePath qnxConfigSettingsFileName()
{
    return Core::ICore::userResourcePath("qnx/qnxconfigurations.xml");
}

static QnxConfigurationManager *m_instance = nullptr;

QnxConfigurationManager::QnxConfigurationManager()
{
    m_instance = this;
    m_writer = new PersistentSettingsWriter(qnxConfigSettingsFileName(), "QnxConfigurations");
    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &QnxConfigurationManager::saveConfigs);
}

QnxConfigurationManager *QnxConfigurationManager::instance()
{
    return m_instance;
}

QnxConfigurationManager::~QnxConfigurationManager()
{
    m_instance = nullptr;
    qDeleteAll(m_configurations);
    delete m_writer;
}

QList<QnxConfiguration *> QnxConfigurationManager::configurations() const
{
    return m_configurations;
}

void QnxConfigurationManager::removeConfiguration(QnxConfiguration *config)
{
    if (m_configurations.removeAll(config)) {
        delete config;
        emit configurationsListUpdated();
    }
}

bool QnxConfigurationManager::addConfiguration(QnxConfiguration *config)
{
    if (!config || !config->isValid())
        return false;

    for (QnxConfiguration *c : std::as_const(m_configurations)) {
        if (c->envFile() == config->envFile())
            return false;
    }

    m_configurations.append(config);
    emit configurationsListUpdated();
    return true;
}

QnxConfiguration *QnxConfigurationManager::configurationFromEnvFile(const FilePath &envFile) const
{
    for (QnxConfiguration *c : m_configurations) {
        if (c->envFile() == envFile)
            return c;
    }

    return nullptr;
}

void QnxConfigurationManager::saveConfigs()
{
    QTC_ASSERT(m_writer, return);
    QVariantMap data;
    data.insert(QLatin1String(QNXConfigsFileVersionKey), 1);
    int count = 0;
    for (QnxConfiguration *config : std::as_const(m_configurations)) {
        QVariantMap tmp = config->toMap();
        if (tmp.isEmpty())
            continue;

        data.insert(QNXConfigDataKey + QString::number(count), tmp);
        ++count;
    }

    data.insert(QLatin1String(QNXConfigCountKey), count);
    m_writer->save(data, Core::ICore::dialogParent());
}

void QnxConfigurationManager::restoreConfigurations()
{
    PersistentSettingsReader reader;
    if (!reader.load(qnxConfigSettingsFileName()))
        return;

    QVariantMap data = reader.restoreValues();
    int count = data.value(QNXConfigCountKey, 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QNXConfigDataKey + QString::number(i);
        if (!data.contains(key))
            continue;

        const QVariantMap dMap = data.value(key).toMap();
        auto configuration = new QnxConfiguration(dMap);
        addConfiguration(configuration);
    }
}

} // Qnx::Internal

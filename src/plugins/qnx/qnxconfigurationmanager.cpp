/**************************************************************************
**
** Copyright (C) 2015 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qnxconfigurationmanager.h"
#include "qnxconfiguration.h"

#include <coreplugin/icore.h>
#include <utils/persistentsettings.h>

namespace Qnx {
namespace Internal {


const QLatin1String QNXConfigDataKey("QNXConfiguration.");
const QLatin1String QNXConfigCountKey("QNXConfiguration.Count");
const QLatin1String QNXConfigsFileVersionKey("Version");

static Utils::FileName qnxConfigSettingsFileName()
{
    return Utils::FileName::fromString(Core::ICore::userResourcePath() + QLatin1String("/qnx/")
                                + QLatin1String(Constants::QNX_CONFIGS_FILENAME));
}

QnxConfigurationManager *QnxConfigurationManager::m_instance = 0;

QnxConfigurationManager::QnxConfigurationManager(QObject *parent)
    : QObject(parent)
{
    m_instance = this;
    m_writer = new Utils::PersistentSettingsWriter(qnxConfigSettingsFileName(),
                                                   QLatin1String("QnxConfigurations"));
    restoreConfigurations();
    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()),
            this, SLOT(saveConfigs()));
}

QnxConfigurationManager *QnxConfigurationManager::instance()
{
    return m_instance;
}

QnxConfigurationManager::~QnxConfigurationManager()
{
    m_instance = 0;
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

    foreach (QnxConfiguration *c, m_configurations) {
        if (c->envFile() == config->envFile())
            return false;
    }

    m_configurations.append(config);
    emit configurationsListUpdated();
    return true;
}

QnxConfiguration *QnxConfigurationManager::configurationFromEnvFile(const Utils::FileName &envFile) const
{
    foreach (QnxConfiguration *c, m_configurations) {
        if (c->envFile() == envFile)
            return c;
    }

    return 0;
}

void QnxConfigurationManager::saveConfigs()
{
    QTC_ASSERT(m_writer, return);
    QVariantMap data;
    data.insert(QLatin1String(QNXConfigsFileVersionKey), 1);
    int count = 0;
    foreach (QnxConfiguration *config, m_configurations) {
        QVariantMap tmp = config->toMap();
        if (tmp.isEmpty())
            continue;

        data.insert(QNXConfigDataKey + QString::number(count), tmp);
        ++count;
    }


    data.insert(QLatin1String(QNXConfigCountKey), count);
    m_writer->save(data, Core::ICore::mainWindow());
}

void QnxConfigurationManager::restoreConfigurations()
{
    Utils::PersistentSettingsReader reader;
    if (!reader.load(qnxConfigSettingsFileName()))
        return;

    QVariantMap data = reader.restoreValues();
    int count = data.value(QNXConfigCountKey, 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QNXConfigDataKey + QString::number(i);
        if (!data.contains(key))
            continue;

        const QVariantMap dMap = data.value(key).toMap();
        QnxConfiguration *configuration = new QnxConfiguration(dMap);
        addConfiguration(configuration);
    }
}

}
}

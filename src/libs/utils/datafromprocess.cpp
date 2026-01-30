// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "datafromprocess.h"

#include "settingsdatabase.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace Utils {

const char settingsGroup[] = "DataFromProcessCache";
const char stdOutKey[] = "stdout";
const char stdErrKey[] = "stderr";

SynchronizedValue<QHash<QString, QVariant>> DataFromProcessSettingsCache::m_newValuesCache = {};

static QString replaceSlashes(const QString &str)
{
    QString result = str;
    result.replace(QLatin1Char('/'), QLatin1Char('\\'));
    return result;
}

void DataFromProcessSettingsCache::writeToSettings()
{
    QTC_ASSERT(Utils::isMainThread(), return);
    SettingsDatabase::beginGroup(settingsGroup);
    const auto cache = m_newValuesCache.readLocked();
    for (auto it = cache->begin(); it != cache->end(); ++it)
        SettingsDatabase::setValue(it.key(), it.value());
    SettingsDatabase::endGroup();
}

void DataFromProcessSettingsCache::setValue(const QString &key, const ProcessOutput &value)
{
    m_newValuesCache.writeLocked()->insert(replaceSlashes(key), value.toVariant());
}

std::optional<DataFromProcessSettingsCache::ProcessOutput> DataFromProcessSettingsCache::value(
    const QString &key)
{
    const QString fixedKey = replaceSlashes(key);
    auto result = m_newValuesCache.get([fixedKey](const QHash<QString, QVariant> &cache) {
        return ProcessOutput::fromVariant(cache.value(fixedKey));
    });
    if (!result) {
        QTC_ASSERT(Utils::isMainThread(), return result);
        SettingsDatabase::beginGroup(settingsGroup);
        const auto settingsValue = SettingsDatabase::value(fixedKey);
        SettingsDatabase::endGroup();
        result = ProcessOutput::fromVariant(settingsValue);
        if (result)
            m_newValuesCache.writeLocked()->insert(fixedKey, settingsValue);
    }
    return result;
}

std::optional<DataFromProcessSettingsCache::ProcessOutput>
DataFromProcessSettingsCache::ProcessOutput::fromVariant(const QVariant &var)
{
    if (!var.isValid())
        return {};
    const QJsonObject obj = QJsonDocument::fromJson(var.toByteArray()).object();
    const QJsonValue output = obj[stdOutKey];
    const QJsonValue error = obj[stdErrKey];
    if (!output.isString() || !error.isString())
        return {};
    return ProcessOutput{output.toString(), error.toString()};
}

QVariant DataFromProcessSettingsCache::ProcessOutput::toVariant() const
{
    QJsonObject obj;
    obj[stdOutKey] = stdOut;
    obj[stdErrKey] = stdErr;
    return QJsonDocument(obj).toJson();
}

} // namespace Utils

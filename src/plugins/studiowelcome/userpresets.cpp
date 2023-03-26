// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "userpresets.h"
#include "algorithm.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <coreplugin/icore.h>
#include <memory>
#include <utils/qtcassert.h>

using namespace StudioWelcome;

FileStoreIo::FileStoreIo(const QString &fileName)
    : m_file{std::make_unique<QFile>(fullFilePath(fileName))}
{}

QByteArray FileStoreIo::read() const
{
    if (!m_file->exists())
        return {};

    if (!m_file->open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << "Cannot load User Preset(s)";
        return {};
    }

    QByteArray data = m_file->readAll();
    m_file->close();

    return data;
}

void FileStoreIo::write(const QByteArray &data)
{
    if (!m_file->open(QFile::WriteOnly | QFile::Text)) {
        qWarning() << "Cannot save User Preset(s)";
        return;
    }

    m_file->write(data);
    m_file->close();
}

QString FileStoreIo::fullFilePath(const QString &fileName) const
{
    return Core::ICore::userResourcePath(fileName).toString();
}

UserPresetsStore::UserPresetsStore(const QString &fileName, StorePolicy policy)
    : m_store{std::make_unique<FileStoreIo>(fileName)}
    , m_policy{policy}
{}

UserPresetsStore::UserPresetsStore(std::unique_ptr<StoreIo> &&fileStore,
                                   StorePolicy policy)
    : m_store{std::move(fileStore)}
    , m_policy{policy}
{}

void UserPresetsStore::savePresets(const std::vector<UserPresetData> &presetItems)
{
    QJsonArray jsonArray;

    for (const auto &preset : presetItems) {
        QJsonObject obj({{"categoryId", preset.categoryId},
                         {"wizardName", preset.wizardName},
                         {"name", preset.name},
                         {"screenSize", preset.screenSize},
                         {"useQtVirtualKeyboard", preset.useQtVirtualKeyboard},
                         {"qtVersion", preset.qtVersion},
                         {"styleName", preset.styleName}});

        jsonArray.append(QJsonValue{obj});
    }

    QJsonDocument doc(jsonArray);
    QByteArray data = doc.toJson();

    m_store->write(data);
}

bool UserPresetsStore::save(const UserPresetData &newPreset)
{
    QTC_ASSERT(newPreset.isValid(), return false);

    std::vector<UserPresetData> presetItems = fetchAll();

    if (m_policy == StorePolicy::UniqueNames) {
        if (Utils::anyOf(presetItems, [&newPreset](const UserPresetData &p) {
                return p.name == newPreset.name;
            })) {
            return false;
        }
    } else if (m_policy == StorePolicy::UniqueValues) {
        if (Utils::containsItem(presetItems, newPreset))
            return false;
    }

    if (m_reverse)
        Utils::prepend(presetItems, newPreset);
    else
        presetItems.push_back(newPreset);

    if (m_maximum > -1 && static_cast<int>(presetItems.size()) > m_maximum) {
        if (m_reverse)
            presetItems.pop_back();
        else
            presetItems.erase(std::cbegin(presetItems));
    }

    savePresets(presetItems);

    return true;
}

void UserPresetsStore::remove(const QString &category, const QString &name)
{
    std::vector<UserPresetData> presetItems = fetchAll();
    auto item = Utils::take(presetItems, [&](const UserPresetData &p) {
        return p.categoryId == category && p.name == name;
    });

    if (!item)
        return;

    savePresets(presetItems);
}

std::vector<UserPresetData> UserPresetsStore::remove(const UserPresetData &preset)
{
    std::vector<UserPresetData> presetItems = fetchAll();
    bool erased = Utils::erase_one(presetItems, preset);
    if (erased)
        savePresets(presetItems);

    return presetItems;
}

std::vector<UserPresetData> UserPresetsStore::fetchAll() const
{
    QByteArray data = m_store->read();

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray())
        return {};

    std::vector<UserPresetData> result;
    const QJsonArray jsonArray = doc.array();

    for (const QJsonValue &value: jsonArray) {
        if (!value.isObject())
            continue;

        const QJsonObject obj = value.toObject();
        UserPresetData preset;

        preset.categoryId = obj["categoryId"].toString();
        preset.wizardName = obj["wizardName"].toString();
        preset.name = obj["name"].toString();
        preset.screenSize = obj["screenSize"].toString();
        preset.useQtVirtualKeyboard = obj["useQtVirtualKeyboard"].toBool();
        preset.qtVersion = obj["qtVersion"].toString();
        preset.styleName = obj["styleName"].toString();

        if (preset.isValid())
            result.push_back(preset);
    }

    return result;
}

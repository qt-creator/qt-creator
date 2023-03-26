// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gradientpresetcustomlistmodel.h"
#include "gradientpresetitem.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QHash>
#include <QByteArray>
#include <QDebug>
#include <QSettings>
#include <QFile>

namespace Internal {

const char settingsKey[] = "GradientPresetCustomList";
const char settingsFileName[] = "GradientPresets.ini";

QString settingsFullFilePath(const QSettings::Scope &scope)
{
    if (scope == QSettings::SystemScope)
        return Core::ICore::installerResourcePath(settingsFileName).toString();

    return Core::ICore::userResourcePath(settingsFileName).toString();
}

} // namespace Internal

GradientPresetCustomListModel::GradientPresetCustomListModel(QObject *parent)
    : GradientPresetListModel(parent)
    , m_filename(getFilename())
{
    readPresets();
}

GradientPresetCustomListModel::~GradientPresetCustomListModel() {}

void GradientPresetCustomListModel::registerDeclarativeType()
{
    qmlRegisterType<GradientPresetCustomListModel>("HelperWidgets",
                                                   2,
                                                   0,
                                                   "GradientPresetCustomListModel");
}

QString GradientPresetCustomListModel::getFilename()
{
    return Internal::settingsFullFilePath(QSettings::UserScope);
}

void GradientPresetCustomListModel::storePresets(const QString &filename,
                                                 const QList<GradientPresetItem> &items)
{
    const QList<QVariant> presets
        = Utils::transform<QList<QVariant>>(items, [](const GradientPresetItem &item) {
              return QVariant::fromValue(item);
          });

    QSettings settings(filename, QSettings::IniFormat);
    settings.clear();
    settings.setValue(Internal::settingsKey, QVariant::fromValue(presets));
}

QList<GradientPresetItem> GradientPresetCustomListModel::storedPresets(const QString &filename)
{
    const QSettings settings(filename, QSettings::IniFormat);
    const QVariant presetSettings = settings.value(Internal::settingsKey);

    if (!presetSettings.isValid())
        return {};

    const QList<QVariant> presets = presetSettings.toList();

    QList<GradientPresetItem> out;
    for (const QVariant &preset : presets) {
        if (preset.isValid()) {
            out.append(preset.value<GradientPresetItem>());
        }
    }

    return out;
}

void GradientPresetCustomListModel::addGradient(const QList<qreal> &stopsPositions,
                                                const QList<QString> &stopsColors,
                                                int stopsCount)
{
    QGradient tempGradient;
    QGradientStops gradientStops;
    QGradientStop gradientStop;
    for (int i = 0; i < stopsCount; i++) {
        gradientStop.first = stopsPositions.at(i);
        gradientStop.second = stopsColors.at(i);
        gradientStops.push_back(gradientStop);
    }

    tempGradient.setStops(gradientStops);

    addItem(GradientPresetItem(tempGradient));
}

void GradientPresetCustomListModel::changePresetName(int id, const QString &newName)
{
    QTC_ASSERT(id >= 0, return);
    QTC_ASSERT(id < m_items.size(), return);
    m_items[id].setPresetName(newName);
    writePresets();
}

void GradientPresetCustomListModel::deletePreset(int id)
{
    QTC_ASSERT(id >= 0, return);
    QTC_ASSERT(id < m_items.size(), return);
    beginResetModel();
    m_items.removeAt(id);
    writePresets();
    endResetModel();
}

void GradientPresetCustomListModel::writePresets()
{
    storePresets(m_filename, m_items);
}

void GradientPresetCustomListModel::readPresets()
{
    const QList<GradientPresetItem> presets = storedPresets(m_filename);
    beginResetModel();
    m_items.clear();

    for (const GradientPresetItem &preset : presets) {
        addItem(preset);
    }
    endResetModel();
}

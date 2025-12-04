// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gradientpresetcustomlistmodel.h"
#include "gradientpresetitem.h"
#include "propertyeditortracing.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QHash>
#include <QByteArray>
#include <QDebug>
#include <QSettings>
#include <QFile>

namespace {

using QmlDesigner::PropertyEditorTracing::category;

const char settingsKey[] = "GradientPresetCustomList";
const char settingsFileName[] = "GradientPresets.ini";

QString settingsFullFilePath(const QSettings::Scope &scope)
{
    if (scope == QSettings::SystemScope)
        return Core::ICore::installerResourcePath(settingsFileName).toFSPathString();

    return Core::ICore::userResourcePath(settingsFileName).toFSPathString();
}

} // namespace

GradientPresetCustomListModel::GradientPresetCustomListModel(QObject *parent)
    : GradientPresetListModel(parent)
    , m_filename(getFilename())
{
    NanotraceHR::Tracer tracer{"gradient preset custom list model constructor", category()};

    readPresets();
}

GradientPresetCustomListModel::~GradientPresetCustomListModel()
{
    NanotraceHR::Tracer tracer{"gradient preset custom list model destructor", category()};
}

void GradientPresetCustomListModel::registerDeclarativeType()
{
    NanotraceHR::Tracer tracer{"gradient preset custom list model register declarative type",
                               category()};

    qmlRegisterType<GradientPresetCustomListModel>("HelperWidgets", 2, 0, "GradientPresetCustomListModel");
}

QString GradientPresetCustomListModel::getFilename()
{
    NanotraceHR::Tracer tracer{"gradient preset custom list model get filename", category()};

    return settingsFullFilePath(QSettings::UserScope);
}

void GradientPresetCustomListModel::storePresets(const QString &filename,
                                                 const QList<GradientPresetItem> &items)
{
    NanotraceHR::Tracer tracer{"gradient preset custom list model store presets", category()};

    const QList<QVariant> presets = Utils::transform<QList<QVariant>>(
        items, [](const GradientPresetItem &item) { return QVariant::fromValue(item); });

    QSettings settings(filename, QSettings::IniFormat);
    settings.clear();
    settings.setValue(settingsKey, QVariant::fromValue(presets));
}

QList<GradientPresetItem> GradientPresetCustomListModel::storedPresets(const QString &filename)
{
    NanotraceHR::Tracer tracer{"gradient preset custom list model stored presets", category()};

    const QSettings settings(filename, QSettings::IniFormat);
    const QVariant presetSettings = settings.value(settingsKey);

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
                                                const QStringList &stopsColors,
                                                int stopsCount)
{
    NanotraceHR::Tracer tracer{"gradient preset custom list model add gradient", category()};

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
    NanotraceHR::Tracer tracer{"gradient preset custom list model change preset name", category()};

    QTC_ASSERT(id >= 0, return);
    QTC_ASSERT(id < m_items.size(), return);
    m_items[id].setPresetName(newName);
    writePresets();
}

void GradientPresetCustomListModel::deletePreset(int id)
{
    NanotraceHR::Tracer tracer{"gradient preset custom list model delete preset", category()};

    QTC_ASSERT(id >= 0, return);
    QTC_ASSERT(id < m_items.size(), return);
    beginResetModel();
    m_items.removeAt(id);
    writePresets();
    endResetModel();
}

void GradientPresetCustomListModel::writePresets()
{
    NanotraceHR::Tracer tracer{"gradient preset custom list model write presets", category()};

    storePresets(m_filename, m_items);
}

void GradientPresetCustomListModel::readPresets()
{
    NanotraceHR::Tracer tracer{"gradient preset custom list model read presets", category()};

    const QList<GradientPresetItem> presets = storedPresets(m_filename);
    beginResetModel();
    m_items.clear();

    for (const GradientPresetItem &preset : presets) {
        addItem(preset);
    }
    endResetModel();
}

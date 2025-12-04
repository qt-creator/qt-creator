// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gradientpresetdefaultlistmodel.h"
#include "gradientpresetitem.h"
#include "propertyeditortracing.h"

#include <QHash>
#include <QByteArray>
#include <QDebug>
#include <QFile>

using QmlDesigner::PropertyEditorTracing::category;

GradientPresetDefaultListModel::GradientPresetDefaultListModel(QObject *parent)
    : GradientPresetListModel(parent)
{
    NanotraceHR::Tracer tracer{"gradient preset default list model constructor", category()};

    addAllPresets();
}

GradientPresetDefaultListModel::~GradientPresetDefaultListModel()
{
    NanotraceHR::Tracer tracer{"gradient preset default list model destructor", category()};
}

void GradientPresetDefaultListModel::registerDeclarativeType()
{
    NanotraceHR::Tracer tracer{"gradient preset default list model register declarative type",
                               category()};

    qmlRegisterType<GradientPresetDefaultListModel>("HelperWidgets",
                                                    2,
                                                    0,
                                                    "GradientPresetDefaultListModel");
}

void GradientPresetDefaultListModel::addAllPresets()
{
    NanotraceHR::Tracer tracer{"gradient preset default list model add all presets", category()};

    const QMetaObject &metaObj = QGradient::staticMetaObject;
    const QMetaEnum metaEnum = metaObj.enumerator(metaObj.indexOfEnumerator("Preset"));

    if (!metaEnum.isValid())
        return;

    for (int i = 0; i < metaEnum.keyCount(); i++) {
        auto preset = GradientPresetItem::Preset(metaEnum.value(i));
        if (preset < GradientPresetItem::numPresets)
            addItem(GradientPresetItem(preset));
    }
}

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gradientpresetdefaultlistmodel.h"
#include "gradientpresetitem.h"

#include <QHash>
#include <QByteArray>
#include <QDebug>
#include <QFile>

GradientPresetDefaultListModel::GradientPresetDefaultListModel(QObject *parent)
    : GradientPresetListModel(parent)
{
    addAllPresets();
}

GradientPresetDefaultListModel::~GradientPresetDefaultListModel() {}

void GradientPresetDefaultListModel::registerDeclarativeType()
{
    qmlRegisterType<GradientPresetDefaultListModel>("HelperWidgets",
                                                    2,
                                                    0,
                                                    "GradientPresetDefaultListModel");
}

void GradientPresetDefaultListModel::addAllPresets()
{
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

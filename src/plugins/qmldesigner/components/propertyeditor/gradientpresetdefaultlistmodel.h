// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gradientpresetlistmodel.h"

#include <QObject>
#include <QAbstractListModel>
#include <QtQml/qqml.h>

class GradientPresetDefaultListModel : public GradientPresetListModel
{
    Q_OBJECT

public:
    explicit GradientPresetDefaultListModel(QObject *parent = nullptr);
    ~GradientPresetDefaultListModel() override;

    static void registerDeclarativeType();

private:
    void addAllPresets();
};

QML_DECLARE_TYPE(GradientPresetDefaultListModel)

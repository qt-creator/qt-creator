// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gradientpresetlistmodel.h"

#include <QObject>
#include <QAbstractListModel>
#include <QtQml/qqml.h>

class GradientPresetCustomListModel : public GradientPresetListModel
{
    Q_OBJECT

public:
    explicit GradientPresetCustomListModel(QObject *parent = nullptr);
    ~GradientPresetCustomListModel() override;

    static void registerDeclarativeType();

    static QString getFilename();
    static void storePresets(const QString &filename, const QList<GradientPresetItem> &items);
    static QList<GradientPresetItem> storedPresets(const QString &filename);

    Q_INVOKABLE void addGradient(const QList<qreal> &stopsPositions,
                                 const QList<QString> &stopsColors,
                                 int stopsCount);

    Q_INVOKABLE void changePresetName(int id, const QString &newName);
    Q_INVOKABLE void deletePreset(int id);

    Q_INVOKABLE void writePresets();
    Q_INVOKABLE void readPresets();

private:
    QString m_filename;
};

QML_DECLARE_TYPE(GradientPresetCustomListModel)

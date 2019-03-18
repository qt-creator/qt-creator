/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

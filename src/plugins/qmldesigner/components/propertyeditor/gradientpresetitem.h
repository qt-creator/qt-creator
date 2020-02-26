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

#include <QObject>
#include <QGradient>

class GradientPresetItem
{
    Q_GADGET

    Q_PROPERTY(QList<qreal> stopsPosList READ stopsPosList FINAL)
    Q_PROPERTY(QList<QString> stopsColorList READ stopsColorList FINAL)
    Q_PROPERTY(int stopListSize READ stopListSize FINAL)
    Q_PROPERTY(QString presetName READ presetName FINAL)
    Q_PROPERTY(int presetID READ presetID FINAL)

public:
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    using Preset = QGradient::Preset;
#else
    enum Preset {};
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    static const int numPresets = Preset::NumPresets;
#else
    static const int numPresets = 181;
#endif

    explicit GradientPresetItem();
    explicit GradientPresetItem(const QGradient &value, const QString &name = QString());
    explicit GradientPresetItem(const Preset number);
    ~GradientPresetItem();

    enum Property {
        objectNameRole = 0,
        stopsPosListRole = 1,
        stopsColorListRole = 2,
        stopListSizeRole = 3,
        presetNameRole = 4,
        presetIDRole = 5
    };

    QVariant getProperty(Property id) const;

    QGradient gradientVal() const;

    void setGradient(const QGradient &value);
    void setGradient(const Preset value);

    QList<qreal> stopsPosList() const;
    QList<QString> stopsColorList() const;
    int stopListSize() const;

    void setPresetName(const QString &value);
    QString presetName() const;
    int presetID() const;

    static QString getNameByPreset(Preset value);

    friend QDebug &operator<<(QDebug &stream, const GradientPresetItem &gradient);

    friend QDataStream &operator<<(QDataStream &stream, const GradientPresetItem &gradient);
    friend QDataStream &operator>>(QDataStream &stream, GradientPresetItem &gradient);

    static QGradient createGradientFromPreset(Preset value);

private:
    QGradient m_gradientVal;
    Preset m_gradientID;
    QString m_presetName;
};

Q_DECLARE_METATYPE(GradientPresetItem)

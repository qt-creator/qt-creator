// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    using Preset = QGradient::Preset;
    static const int numPresets = Preset::NumPresets;

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

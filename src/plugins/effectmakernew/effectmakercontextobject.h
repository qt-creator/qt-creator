// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <model.h>
#include <modelnode.h>

#include <QObject>
#include <QUrl>
#include <QQmlPropertyMap>
#include <QQmlComponent>
#include <QColor>
#include <QPoint>
#include <QMouseEvent>

namespace EffectMaker {

class EffectMakerContextObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString stateName READ stateName WRITE setStateName NOTIFY stateNameChanged)
    Q_PROPERTY(QStringList allStateNames READ allStateNames WRITE setAllStateNames NOTIFY allStateNamesChanged)
    Q_PROPERTY(int possibleTypeIndex READ possibleTypeIndex NOTIFY possibleTypeIndexChanged)

    Q_PROPERTY(bool isBaseState READ isBaseState WRITE setIsBaseState NOTIFY isBaseStateChanged)
    Q_PROPERTY(bool selectionChanged READ selectionChanged WRITE setSelectionChanged NOTIFY selectionChangedChanged)

    Q_PROPERTY(int majorVersion READ majorVersion WRITE setMajorVersion NOTIFY majorVersionChanged)

    Q_PROPERTY(QQmlPropertyMap *backendValues READ backendValues WRITE setBackendValues NOTIFY backendValuesChanged)

public:
    EffectMakerContextObject(QQmlContext *context, QObject *parent = nullptr);

    QString stateName() const { return m_stateName; }
    QStringList allStateNames() const { return m_allStateNames; }
    int possibleTypeIndex() const { return m_possibleTypeIndex; }

    bool isBaseState() const { return m_isBaseState; }
    bool selectionChanged() const { return m_selectionChanged; }

    QQmlPropertyMap *backendValues() const { return m_backendValues; }

    Q_INVOKABLE QString convertColorToString(const QVariant &color);
    Q_INVOKABLE QColor colorFromString(const QString &colorString);

    Q_INVOKABLE void hideCursor();
    Q_INVOKABLE void restoreCursor();
    Q_INVOKABLE void holdCursorInPlace();

    Q_INVOKABLE int devicePixelRatio();

    Q_INVOKABLE QStringList allStatesForId(const QString &id);

    Q_INVOKABLE bool isBlocked(const QString &propName) const;

    int majorVersion() const;
    void setMajorVersion(int majorVersion);

    void setStateName(const QString &newStateName);
    void setAllStateNames(const QStringList &allStates);
    void setIsBaseState(bool newIsBaseState);
    void setSelectionChanged(bool newSelectionChanged);
    void setBackendValues(QQmlPropertyMap *newBackendValues);
    void setModel(QmlDesigner::Model *model);

    void triggerSelectionChanged();

signals:
    void stateNameChanged();
    void allStateNamesChanged();
    void possibleTypeIndexChanged();
    void isBaseStateChanged();
    void selectionChangedChanged();
    void backendValuesChanged();
    void majorVersionChanged();

private:
    void updatePossibleTypeIndex();

    QQmlContext *m_qmlContext = nullptr;

    QString m_stateName;
    QStringList m_allStateNames;
    int m_possibleTypeIndex = -1;
    QString m_currentType;

    int m_majorVersion = 1;

    QQmlPropertyMap *m_backendValues = nullptr;
    QmlDesigner::Model *m_model = nullptr;

    QPoint m_lastPos;

    bool m_isBaseState = false;
    bool m_selectionChanged = false;
};

} // namespace EffectMaker


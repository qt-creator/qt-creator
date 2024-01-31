// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "effectcomposeruniformsmodel.h"

#include <QJsonObject>
#include <QObject>

namespace EffectComposer {

class CompositionNode : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString nodeName MEMBER m_name CONSTANT)
    Q_PROPERTY(bool nodeEnabled READ isEnabled WRITE setIsEnabled NOTIFY isEnabledChanged)
    Q_PROPERTY(bool isDependency READ isDependency NOTIFY isDepencyChanged)
    Q_PROPERTY(QObject *nodeUniformsModel READ uniformsModel NOTIFY uniformsModelChanged)

public:
    enum NodeType {
        SourceNode = 0,
        DestinationNode,
        CustomNode
    };

    CompositionNode(const QString &effectName, const QString &qenPath, const QJsonObject &json = {});

    QString fragmentCode() const;
    QString vertexCode() const;
    QString description() const;
    QString id() const;

    QObject *uniformsModel();

    QStringList requiredNodes() const;

    NodeType type() const;

    bool isEnabled() const;
    void setIsEnabled(bool newIsEnabled);

    bool isDependency() const;

    QString name() const;

    QList<Uniform *> uniforms() const;

    int incRefCount();
    int decRefCount();
    void setRefCount(int count);

signals:
    void uniformsModelChanged();
    void isEnabledChanged();
    void isDepencyChanged();

private:
    void parse(const QString &effectName, const QString &qenPath, const QJsonObject &json);

    QString m_name;
    NodeType m_type = CustomNode;
    QString m_fragmentCode;
    QString m_vertexCode;
    QString m_description;
    QStringList m_requiredNodes;
    QString m_id;
    bool m_isEnabled = true;
    int m_refCount = 0;

    QList<Uniform *> m_uniforms;

    EffectComposerUniformsModel m_unifomrsModel;
};

} // namespace EffectComposer

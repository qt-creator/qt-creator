// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "effectmakeruniformsmodel.h"

#include <QObject>

namespace QmlDesigner {

class CompositionNode : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString nodeName MEMBER m_name CONSTANT)
    Q_PROPERTY(QObject *nodeUniformsModel READ uniformsModel NOTIFY uniformsModelChanged)

public:
    CompositionNode(const QString &qenPath);

    QString fragmentCode() const;
    QString vertexCode() const;
    QString description() const;

    QObject *uniformsModel();

    QStringList requiredNodes() const;

signals:
    void uniformsModelChanged();

private:
    void parse(const QString &qenPath);

    QString m_name;
    QString m_fragmentCode;
    QString m_vertexCode;
    QString m_description;
    QStringList m_requiredNodes;

    EffectMakerUniformsModel m_unifomrsModel;
};

} // namespace QmlDesigner

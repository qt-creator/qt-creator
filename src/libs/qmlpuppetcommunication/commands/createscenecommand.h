// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QSize>
#include <QUrl>
#include <QVector>
#include <QList>
#include <QColor>
#include <qmetatype.h>

#include "instancecontainer.h"
#include "reparentcontainer.h"
#include "idcontainer.h"
#include "mockuptypecontainer.h"
#include "propertyvaluecontainer.h"
#include "propertybindingcontainer.h"
#include "addimportcontainer.h"

namespace QmlDesigner {

class CreateSceneCommand
{
public:
    CreateSceneCommand() = default;
    explicit CreateSceneCommand(const QVector<InstanceContainer> &instanceContainer,
                                const QVector<ReparentContainer> &reparentContainer,
                                const QVector<IdContainer> &idVector,
                                const QVector<PropertyValueContainer> &valueChangeVector,
                                const QVector<PropertyBindingContainer> &bindingChangeVector,
                                const QVector<PropertyValueContainer> &auxiliaryChangeVector,
                                const QVector<AddImportContainer> &importVector,
                                const QVector<MockupTypeContainer> &mockupTypeVector,
                                const QUrl &fileUrl,
                                const QUrl &resourceUrl,
                                const QHash<QString, QVariantMap> &edit3dToolStates,
                                const QString &language,
                                QSize captureImageMinimumSize,
                                QSize captureImageMaximumSize,
                                qint32 stateInstanceId)
        : instances(instanceContainer)
        , reparentInstances(reparentContainer)
        , ids(idVector)
        , valueChanges(valueChangeVector)
        , bindingChanges(bindingChangeVector)
        , auxiliaryChanges(auxiliaryChangeVector)
        , imports(importVector)
        , mockupTypes(mockupTypeVector)
        , fileUrl(fileUrl)
        , resourceUrl(resourceUrl)
        , edit3dToolStates(edit3dToolStates)
        , language(language)
        , captureImageMinimumSize(captureImageMinimumSize)
        , captureImageMaximumSize(captureImageMaximumSize)
        , stateInstanceId{stateInstanceId}
    {}

    friend QDataStream &operator<<(QDataStream &out, const CreateSceneCommand &command)
    {
        out << command.instances;
        out << command.reparentInstances;
        out << command.ids;
        out << command.valueChanges;
        out << command.bindingChanges;
        out << command.auxiliaryChanges;
        out << command.imports;
        out << command.mockupTypes;
        out << command.fileUrl;
        out << command.resourceUrl;
        out << command.edit3dToolStates;
        out << command.language;
        out << command.stateInstanceId;
        out << command.captureImageMinimumSize;
        out << command.captureImageMaximumSize;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, CreateSceneCommand &command)
    {
        in >> command.instances;
        in >> command.reparentInstances;
        in >> command.ids;
        in >> command.valueChanges;
        in >> command.bindingChanges;
        in >> command.auxiliaryChanges;
        in >> command.imports;
        in >> command.mockupTypes;
        in >> command.fileUrl;
        in >> command.resourceUrl;
        in >> command.edit3dToolStates;
        in >> command.language;
        in >> command.stateInstanceId;
        in >> command.captureImageMinimumSize;
        in >> command.captureImageMaximumSize;

        return in;
    }

public:
    QVector<InstanceContainer> instances;
    QVector<ReparentContainer> reparentInstances;
    QVector<IdContainer> ids;
    QVector<PropertyValueContainer> valueChanges;
    QVector<PropertyBindingContainer> bindingChanges;
    QVector<PropertyValueContainer> auxiliaryChanges;
    QVector<AddImportContainer> imports;
    QVector<MockupTypeContainer> mockupTypes;
    QUrl fileUrl;
    QUrl resourceUrl;
    QHash<QString, QVariantMap> edit3dToolStates;
    QString language;
    QSize captureImageMinimumSize;
    QSize captureImageMaximumSize;
    qint32 stateInstanceId = 0;
};

QDebug operator<<(QDebug debug, const CreateSceneCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::CreateSceneCommand)

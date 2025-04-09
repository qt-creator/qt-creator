// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QColor>
#include <QList>
#include <QMetaType>
#include <QSize>
#include <QUrl>

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
    explicit CreateSceneCommand(const QList<InstanceContainer> &instanceContainer,
                                const QList<ReparentContainer> &reparentContainer,
                                const QList<IdContainer> &idVector,
                                const QList<PropertyValueContainer> &valueChangeVector,
                                const QList<PropertyBindingContainer> &bindingChangeVector,
                                const QList<PropertyValueContainer> &auxiliaryChangeVector,
                                const QList<AddImportContainer> &importVector,
                                const QList<MockupTypeContainer> &mockupTypeVector,
                                const QUrl &fileUrl,
                                const QUrl &resourceUrl,
                                const QHash<QString, QVariantMap> &edit3dToolStates,
                                const QString &language,
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

        return in;
    }

public:
    QList<InstanceContainer> instances;
    QList<ReparentContainer> reparentInstances;
    QList<IdContainer> ids;
    QList<PropertyValueContainer> valueChanges;
    QList<PropertyBindingContainer> bindingChanges;
    QList<PropertyValueContainer> auxiliaryChanges;
    QList<AddImportContainer> imports;
    QList<MockupTypeContainer> mockupTypes;
    QUrl fileUrl;
    QUrl resourceUrl;
    QHash<QString, QVariantMap> edit3dToolStates;
    QString language;
    qint32 stateInstanceId = 0;
};

QDebug operator<<(QDebug debug, const CreateSceneCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::CreateSceneCommand)

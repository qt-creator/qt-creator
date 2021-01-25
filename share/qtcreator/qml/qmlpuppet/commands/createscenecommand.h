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

#include <qmetatype.h>
#include <QUrl>
#include <QVector>

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
    qint32 stateInstanceId = 0;
};

QDebug operator<<(QDebug debug, const CreateSceneCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::CreateSceneCommand)

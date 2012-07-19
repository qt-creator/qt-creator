/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "createscenecommand.h"

namespace QmlDesigner {

CreateSceneCommand::CreateSceneCommand()
{
}

CreateSceneCommand::CreateSceneCommand(const QVector<InstanceContainer> &instanceContainer,
                                       const QVector<ReparentContainer> &reparentContainer,
                                       const QVector<IdContainer> &idVector,
                                       const QVector<PropertyValueContainer> &valueChangeVector,
                                       const QVector<PropertyBindingContainer> &bindingChangeVector,
                                       const QVector<PropertyValueContainer> &auxiliaryChangeVector,
                                       const QVector<AddImportContainer> &importVector,
                                       const QUrl &fileUrl)
    : m_instanceVector(instanceContainer),
      m_reparentInstanceVector(reparentContainer),
      m_idVector(idVector),
      m_valueChangeVector(valueChangeVector),
      m_bindingChangeVector(bindingChangeVector),
      m_auxiliaryChangeVector(auxiliaryChangeVector),
      m_importVector(importVector),
      m_fileUrl(fileUrl)
{
}

QVector<InstanceContainer> CreateSceneCommand::instances() const
{
    return m_instanceVector;
}

QVector<ReparentContainer> CreateSceneCommand::reparentInstances() const
{
    return m_reparentInstanceVector;
}

QVector<IdContainer> CreateSceneCommand::ids() const
{
    return m_idVector;
}

QVector<PropertyValueContainer> CreateSceneCommand::valueChanges() const
{
    return m_valueChangeVector;
}

QVector<PropertyBindingContainer> CreateSceneCommand::bindingChanges() const
{
    return m_bindingChangeVector;
}

QVector<PropertyValueContainer> CreateSceneCommand::auxiliaryChanges() const
{
    return m_auxiliaryChangeVector;
}

QVector<AddImportContainer> CreateSceneCommand::imports() const
{
    return m_importVector;
}

QUrl CreateSceneCommand::fileUrl() const
{
    return m_fileUrl;
}

QDataStream &operator<<(QDataStream &out, const CreateSceneCommand &command)
{
    out << command.instances();
    out << command.reparentInstances();
    out << command.ids();
    out << command.valueChanges();
    out << command.bindingChanges();
    out << command.auxiliaryChanges();
    out << command.imports();
    out << command.fileUrl();

    return out;
}

QDataStream &operator>>(QDataStream &in, CreateSceneCommand &command)
{
    in >> command.m_instanceVector;
    in >> command.m_reparentInstanceVector;
    in >> command.m_idVector;
    in >> command.m_valueChangeVector;
    in >> command.m_bindingChangeVector;
    in >> command.m_auxiliaryChangeVector;
    in >> command.m_importVector;
    in >> command.m_fileUrl;

    return in;
}

}

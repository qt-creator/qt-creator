/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
                                       const QVector<AddImportContainer> &importVector,
                                       const QUrl &fileUrl)
    : m_instanceVector(instanceContainer),
      m_reparentInstanceVector(reparentContainer),
      m_idVector(idVector),
      m_valueChangeVector(valueChangeVector),
      m_bindingChangeVector(bindingChangeVector),
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
    in >> command.m_importVector;
    in >> command.m_fileUrl;

    return in;
}

}

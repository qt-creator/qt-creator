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

#include "createscenecommand.h"

#include <QDebug>

namespace QmlDesigner {

CreateSceneCommand::CreateSceneCommand() = default;

CreateSceneCommand::CreateSceneCommand(const QVector<InstanceContainer> &instanceContainer,
                                       const QVector<ReparentContainer> &reparentContainer,
                                       const QVector<IdContainer> &idVector,
                                       const QVector<PropertyValueContainer> &valueChangeVector,
                                       const QVector<PropertyBindingContainer> &bindingChangeVector,
                                       const QVector<PropertyValueContainer> &auxiliaryChangeVector,
                                       const QVector<AddImportContainer> &importVector,
                                       const QVector<MockupTypeContainer> &mockupTypeVector,
                                       const QUrl &fileUrl)
    : m_instanceVector(instanceContainer),
      m_reparentInstanceVector(reparentContainer),
      m_idVector(idVector),
      m_valueChangeVector(valueChangeVector),
      m_bindingChangeVector(bindingChangeVector),
      m_auxiliaryChangeVector(auxiliaryChangeVector),
      m_importVector(importVector),
      m_mockupTypeVector(mockupTypeVector),
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

QVector<MockupTypeContainer> CreateSceneCommand::mockupTypes() const
{
    return m_mockupTypeVector;
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
    out << command.mockupTypes();
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
    in >> command.m_mockupTypeVector;
    in >> command.m_fileUrl;

    return in;
}

QDebug operator <<(QDebug debug, const CreateSceneCommand &command)
{
    return debug.nospace() << "CreateSceneCommand("
                    << "instances: " << command.instances() << ", "
                    << "reparentInstances: " << command.reparentInstances() << ", "
                    << "ids: " << command.ids() << ", "
                    << "valueChanges: " << command.valueChanges() << ", "
                    << "bindingChanges: " << command.bindingChanges() << ", "
                    << "auxiliaryChanges: " << command.auxiliaryChanges() << ", "
                    << "imports: " << command.imports() << ", "
                    << "mockupTypes: " << command.mockupTypes() << ", "
                    << "fileUrl: " << command.fileUrl() << ")";
}

}

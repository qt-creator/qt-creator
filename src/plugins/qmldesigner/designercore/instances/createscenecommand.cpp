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

#ifndef CREATESCENECOMMAND_H
#define CREATESCENECOMMAND_H

#include <qmetatype.h>
#include <QUrl>
#include <QVector>

#include "instancecontainer.h"
#include "reparentcontainer.h"
#include "idcontainer.h"
#include "propertyvaluecontainer.h"
#include "propertybindingcontainer.h"
#include "addimportcontainer.h"

namespace QmlDesigner {

class CreateSceneCommand
{
    friend QDataStream &operator>>(QDataStream &in, CreateSceneCommand &command);

public:
    CreateSceneCommand();
    CreateSceneCommand(const QVector<InstanceContainer> &instanceContainer,
                       const QVector<ReparentContainer> &reparentContainer,
                       const QVector<IdContainer> &idVector,
                       const QVector<PropertyValueContainer> &valueChangeVector,
                       const QVector<PropertyBindingContainer> &bindingChangeVector,
                       const QVector<AddImportContainer> &importVector,
                       const QUrl &fileUrl);

    QVector<InstanceContainer> instances() const;
    QVector<ReparentContainer> reparentInstances() const;
    QVector<IdContainer> ids() const;
    QVector<PropertyValueContainer> valueChanges() const;
    QVector<PropertyBindingContainer> bindingChanges() const;
    QVector<AddImportContainer> imports() const;
    QUrl fileUrl() const;

private:
    QVector<InstanceContainer> m_instanceVector;
    QVector<ReparentContainer> m_reparentInstanceVector;
    QVector<IdContainer> m_idVector;
    QVector<PropertyValueContainer> m_valueChangeVector;
    QVector<PropertyBindingContainer> m_bindingChangeVector;
    QVector<AddImportContainer> m_importVector;
    QUrl m_fileUrl;
};

QDataStream &operator<<(QDataStream &out, const CreateSceneCommand &command);
QDataStream &operator>>(QDataStream &in, CreateSceneCommand &command);

}

Q_DECLARE_METATYPE(QmlDesigner::CreateSceneCommand)

#endif // CREATESCENECOMMAND_H

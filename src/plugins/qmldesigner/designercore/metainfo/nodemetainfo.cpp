/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "nodemetainfo.h"
#include "model.h"
#include "widgetqueryview.h"
#include "nodeinstance.h"
#include "invalidargumentexception.h"

#include "metainfo.h"
#include "propertymetainfo.h"

#include <QSharedData>
#include <QtDebug>
#include <QIcon>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeComponent>

#include <private/qdeclarativemetatype_p.h>

namespace QmlDesigner {

namespace Internal {

class NodeMetaInfoData : public QSharedData
{
public:
    typedef enum {
        No = -1,
        Unknown = 0,
        Yes = 1,
    } TristateBoolean;

public:
    NodeMetaInfoData(const MetaInfo &metaInfo) :
            metaInfo(metaInfo),
            isContainer(false),
            isFXItem(Unknown),
            icon(),
            category("misc")
    { }

    MetaInfo metaInfo;
    QString typeName;
    bool isContainer;
    TristateBoolean isFXItem;
    QHash<QString, PropertyMetaInfo> propertyMetaInfoHash;
    QIcon icon;
    QString category;
    QString qmlFile;
    QString defaultProperty;

    QString superClassType;
    int superClassMajorVersion;
    int superClassMinorVersion;

    int majorVersion;
    int minorVersion;
};

} // namespace Internal

/*!
\class QmlDesigner::NodeMetaInfo
\ingroup CoreModel
\brief The NodeMetaInfo class provides meta information about a qml type.

A NodeMetaInfo object can be created via ModelNode::metaInfo, or MetaInfo::nodeMetaInfo.

The object can be invalid - you can check this by calling isValid().
The object is invalid if you ask for meta information for
an non-existing qml property. Also the node meta info can become invalid
if the enclosing type is deregistered from the meta type system (e.g.
a sub component qml file is deleted). Trying to call any accessor methods on an invalid
NodeMetaInfo object will result in an InvalidMetaInfoException being thrown.

\see QmlDesigner::MetaInfo, QmlDesigner::PropertyMetaInfo, QmlDesigner::EnumeratorMetaInfo
*/

NodeMetaInfo::NodeMetaInfo()
    : m_data(0)
{
    // create invalid node
}

NodeMetaInfo::NodeMetaInfo(const MetaInfo &metaInfo)
    : m_data(new Internal::NodeMetaInfoData(metaInfo))
{
}

NodeMetaInfo::~NodeMetaInfo()
{
}

/*!
  \brief Creates a copy of the handle.
  */
NodeMetaInfo::NodeMetaInfo(const NodeMetaInfo &other)
    : m_data(other.m_data)
{
}

/*!
  \brief Copies the handle.
  */
NodeMetaInfo &NodeMetaInfo::operator=(const NodeMetaInfo &other)
{
    if (this != &other)
        this->m_data = other.m_data;

    return *this;
}

/*!
  \brief Returns whether the meta information system knows about this type.
  */
bool NodeMetaInfo::isValid() const
{
    return (m_data.data() != 0);
}

MetaInfo NodeMetaInfo::metaInfo() const
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return MetaInfo();
    }

    return m_data->metaInfo;
}

/*!
  \brief Creates an instance of the qml type in the given qml context.

  \throws InvalidArgumentException when the context argument is a null pointer
  \throws InvalidMetaInfoException if the object is not valid
  */
QObject *NodeMetaInfo::createInstance(QDeclarativeContext *context) const
{
    if (!context) {
        Q_ASSERT_X(0, Q_FUNC_INFO, "Context cannot be null");
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, "context");
    }

    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return 0; // maybe we should return a new QObject?
    }

    QObject *object = 0;
    if (isComponent()) {
        // qml component
        // TODO: This is maybe expensive ...
        QDeclarativeComponent component(context->engine(), QUrl::fromLocalFile(m_data->qmlFile));
        QDeclarativeContext *newContext =  new QDeclarativeContext(context);
        object = component.create(newContext);
        newContext->setParent(object);
    } else {
        // primitive
        QDeclarativeType *type = QDeclarativeMetaType::qmlType(typeName().toAscii(), majorVersion(), minorVersion());
        if (type)  {
            object = type->create();
        } else {
            qWarning() << "QuickDesigner: Cannot create an object of type"
                       << QString("%1 %2,%3").arg(typeName(), majorVersion(), minorVersion())
                       << "- type isn't known to declarative meta type system";
        }

        if (object && context)
            QDeclarativeEngine::setContextForObject(object, context);
    }
    return object;
}

/*!
  \brief Returns all (direct and indirect) ancestor types.

  \throws InvalidMetaInfoException if the object is not valid
  */
QList<NodeMetaInfo> NodeMetaInfo::superClasses() const
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return QList<NodeMetaInfo>();
    }

    QList<NodeMetaInfo> types;

    NodeMetaInfo superType = directSuperClass();
    if (superType.isValid()) {
        types += superType;
        types += superType.superClasses();
    }

    return types;
}

/*!
  \brief Returns direct ancestor type. An invalid type if there is none.
  */
NodeMetaInfo NodeMetaInfo::directSuperClass() const
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return NodeMetaInfo();
    }

    if (!m_data->superClassType.isEmpty()) {
        int superClassMajorVersion = m_data->superClassMajorVersion;
        int superClassMinorVersion = m_data->superClassMinorVersion;
        if (superClassMajorVersion == -1) {
            // If we don't know the version of the super type, assume that it's the same like our version.
            superClassMajorVersion = majorVersion();
            superClassMinorVersion = minorVersion();
        }
        return m_data->metaInfo.nodeMetaInfo(m_data->superClassType,
                                             superClassMajorVersion, superClassMinorVersion);
    }
    return NodeMetaInfo();
}

/*!
  \brief Returns meta information for all properties, including properties inherited from base types.

  Returns a Hash with the name of the property as key and property meta information as value. Node
  In case there are multiple properties with the same name in the hierarchy the property defined
  in the more concrete subclass is chosen.

  \throws InvalidMetaInfoException if the object is not valid
  */
QHash<QString,PropertyMetaInfo> NodeMetaInfo::properties(bool resolveDotSyntax) const
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return QHash<QString,PropertyMetaInfo>();
    }

    QHash<QString,PropertyMetaInfo> propertiesInfo;
    propertiesInfo = m_data->propertyMetaInfoHash;

    foreach (const NodeMetaInfo &nodeInfo, superClasses()) {
        QHash<QString,PropertyMetaInfo> superClassProperties = nodeInfo.properties();
        QHashIterator<QString,PropertyMetaInfo> iter(superClassProperties);
        while (iter.hasNext()) {
            iter.next();
            if (!propertiesInfo.contains(iter.key()))
                propertiesInfo.insert(iter.key(), iter.value());
        }
    }
    if (resolveDotSyntax) {
        QHashIterator<QString,PropertyMetaInfo> iter(dotProperties());
        while (iter.hasNext()) {
            iter.next();
            if (!propertiesInfo.contains(iter.key()))
                propertiesInfo.insert(iter.key(), iter.value());
        }
    }
    return propertiesInfo;
}

/*!
  \brief Returns meta information for all dot properties, including properties inherited from base types.

  */
QHash<QString,PropertyMetaInfo> NodeMetaInfo::dotProperties() const
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return QHash<QString,PropertyMetaInfo>();
    }

    QHash<QString,PropertyMetaInfo> propertiesInfo;

    foreach (const QString &propertyName, properties().keys()) {
        if (property(propertyName).hasDotSubProperties()) {
                QString propertyType = property(propertyName).type();
                if (propertyType.right(1) == "*")
                    propertyType = propertyType.left(propertyType.size() - 1).trimmed();

                NodeMetaInfo nodeInfo(m_data->metaInfo.nodeMetaInfo(m_data->metaInfo.fromQtTypes(propertyType), majorVersion(), minorVersion()));
                if (nodeInfo.isValid()) {
                    QHashIterator<QString,PropertyMetaInfo> iter(nodeInfo.properties());
                    while (iter.hasNext()) {
                        iter.next();
                        if (!propertiesInfo.contains(iter.key()) && iter.key() != "objectName")
                            propertiesInfo.insert(propertyName + '.' + iter.key(), iter.value());
                    }
            }
        }
    }
    return propertiesInfo;
}

/*!
  \brief Returns meta information for a property. An invalid PropertyMetaInfo object if the given property name is unknown.

  \throws InvalidMetaInfoException if the object is not valid
  */
PropertyMetaInfo NodeMetaInfo::property(const QString &propertyName, bool resolveDotSyntax) const
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return PropertyMetaInfo();
    }

    if (resolveDotSyntax && propertyName.contains('.')) {
        const QStringList nameParts = propertyName.split('.');
        NodeMetaInfo nodeInfo = *this;
        const int partCount = nameParts.size();
        for (int i = 0; i < partCount; ++i) {
            const QString namePart(nameParts.at(i));
            const PropertyMetaInfo propInfo = nodeInfo.property(namePart, false);

            if (!propInfo.isValid())
                break;

            if (i + 1 == partCount)
                return propInfo;

            QString propertyType = propInfo.type();
            if (propertyType.right(1) == "*")
                propertyType = propertyType.left(propertyType.size() - 1).trimmed();
            nodeInfo = m_data->metaInfo.nodeMetaInfo(m_data->metaInfo.fromQtTypes(propertyType), majorVersion(), minorVersion());
            if (!nodeInfo.isValid()) {
                qWarning() << "no type info available for" << propertyType;
                break;
            }
        }

        return PropertyMetaInfo();
    } else {
        PropertyMetaInfo propertyMetaInfo;

        if (hasLocalProperty(propertyName)) {
            propertyMetaInfo = m_data->propertyMetaInfoHash.value(propertyName, PropertyMetaInfo());
        } else {
            foreach (const NodeMetaInfo &superTypeMetaInfo, superClasses()) {
                Q_ASSERT(superTypeMetaInfo.isValid());
                propertyMetaInfo = superTypeMetaInfo.property(propertyName);
                if (propertyMetaInfo.isValid())
                    break;
            }
        }

        return propertyMetaInfo;
    }
}

/*!
  \brief Returns whether the type has a (not inherited) property.

  \throws InvalidMetaInfoException if the object is not valid
  */
bool NodeMetaInfo::hasLocalProperty(const QString &propertyName, bool resolveDotSyntax) const
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return false;
    }

    if (resolveDotSyntax && propertyName.contains('.')) {
        const QStringList nameParts = propertyName.split('.');
        NodeMetaInfo nodeInfo = *this;
        const int partCount = nameParts.size();
        for (int i = 0; i < partCount; ++i) {
            QString namePart(nameParts.at(i));
            const PropertyMetaInfo propInfo = nodeInfo.property(namePart, false);

            if (!propInfo.isValid())
                break;

            if (i + 1 == partCount)
                return true;

            QString propertyType = propInfo.type();
            if (propertyType.right(1) == "*")
                propertyType = propertyType.left(propertyType.size() - 1).trimmed();
            nodeInfo = m_data->metaInfo.nodeMetaInfo(m_data->metaInfo.fromQtTypes(propertyType), majorVersion(), minorVersion());
            if (!nodeInfo.isValid()) {
                qWarning() << "no type info available for" << propertyType;
                break;
            }
        }

        return false;
    } else {
        return m_data->propertyMetaInfoHash.contains(propertyName);
    }
}

/*!
  \brief Returns whether the type has a (inherited or not inherited) property.

  \throws InvalidMetaInfoException if the object is not valid
  */
bool NodeMetaInfo::hasProperty(const QString &propertyName, bool resolveDotSyntax) const
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return false;
    }

    if (hasLocalProperty(propertyName, resolveDotSyntax))
        return true;

    if (directSuperClass().hasProperty(propertyName, resolveDotSyntax))
        return true;

    return false;
}

void NodeMetaInfo::addProperty(const PropertyMetaInfo &property)
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return;
    }

    m_data->propertyMetaInfoHash.insert(property.name(), property);
}

/*!
  \brief Returns the name of the qml type.

  This is not necessarily the class name: E.g. the class defining "Item" is QDeclarativeItem.

  \throws InvalidMetaInfoException if the object is not valid
  */
QString NodeMetaInfo::typeName() const
{
    if (!isValid()) {
        return QString();
    }
    return m_data->typeName;
}

/*!
  \brief Returns the name of the major number of the qml type.

  \throws InvalidMetaInfoException if the object is not valid
*/
int NodeMetaInfo::majorVersion() const
{
    if (!isValid()) {
        return -1;
    }

    return m_data->majorVersion;
}


/*!
  \brief Returns the name of the minor number of the qml type to which the type is used.

  \throws InvalidMetaInfoException if the object is not valid
*/
int NodeMetaInfo::minorVersion() const
{
    if (!isValid()) {
        return -1;
    }

    return m_data->minorVersion;
}

bool NodeMetaInfo::availableInVersion(int majorVersion, int minorVersion) const
{
    return ((majorVersion > m_data->majorVersion)
            || (majorVersion == m_data->majorVersion && minorVersion >= m_data->minorVersion))
            || (majorVersion == -1 && minorVersion == -1);
}

bool NodeMetaInfo::hasDefaultProperty() const
{
    if (!isValid()) {
        return false;
    }

    return m_data->defaultProperty.isNull();
}

QString NodeMetaInfo::defaultProperty() const
{
    if (!isValid()) {
        return QString();
    }

    return m_data->defaultProperty;
}

void NodeMetaInfo::setDefaultProperty(const QString &defaultProperty)
{
     if (!isValid()) {
        return;
    }

    m_data->defaultProperty = defaultProperty;
}

void NodeMetaInfo::setSuperClass(const QString &typeName, int majorVersion, int minorVersion)
{
    if (!isValid()) {
        return;
    }
    m_data->superClassType = typeName;
    m_data->superClassMajorVersion = majorVersion;
    m_data->superClassMinorVersion = minorVersion;
}

void NodeMetaInfo::setInvalid()
{
    if (!isValid())
        return;

    m_data = 0;
}

void NodeMetaInfo::setType(const QString &typeName, int majorVersion, int minorVersion)
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return;
    }
    m_data->typeName = typeName;
    m_data->majorVersion = majorVersion;
    m_data->minorVersion = minorVersion;
}

uint qHash(const NodeMetaInfo &nodeMetaInfo)
{
    if (!nodeMetaInfo.isValid())
        return 0;
    return qHash(nodeMetaInfo.m_data->typeName);
}

bool operator==(const NodeMetaInfo &firstNodeInfo,
                const NodeMetaInfo &secondNodeInfo)
{
    if (!firstNodeInfo.isValid() || !secondNodeInfo.isValid())
        return false;
    return firstNodeInfo.m_data->typeName == secondNodeInfo.m_data->typeName;
}

/*!
  \brief Returns whether objects of these type can have children.

  \throws InvalidMetaInfoException if the object is not valid
  */
bool NodeMetaInfo::isContainer() const
{
    // TODO KAI: Is this too generic?
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return false;
    }
    return m_data->isContainer;
}

void NodeMetaInfo::setIsContainer(bool isContainer)
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return;
    }
    m_data->isContainer = isContainer;
}

QIcon NodeMetaInfo::icon() const
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return QIcon();
    }
    return m_data->icon;
}

void NodeMetaInfo::setIcon(const QIcon &icon)
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return;
    }
    m_data->icon = icon;
}

bool NodeMetaInfo::isComponent() const
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return false;
    }

    return !m_data->qmlFile.isEmpty();
}

/*!
  \brief Returns whether the type inherits from a type.

  \throws InvalidMetaInfoException if the object is not valid
  */
bool NodeMetaInfo::isSubclassOf(const QString &type, int majorVersion, int minorVersion) const
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return false;
    }

    if (typeName() == type
        && availableInVersion(majorVersion, minorVersion))
        return true;

    foreach (const NodeMetaInfo &superClass, superClasses()) {
        if (superClass.typeName() == type
            && superClass.availableInVersion(majorVersion, minorVersion))
            return true;
    }
    return false;
}

void NodeMetaInfo::setQmlFile(const QString &filePath)
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return;
    }
    m_data->qmlFile = filePath;
}

//QDataStream& operator<<(QDataStream& stream, const NodeMetaInfo& nodeMetaInfo)
//{
//    stream << nodeMetaInfo.typeName();
//    stream << nodeMetaInfo.majorVersion();
//    stream << nodeMetaInfo.minorVersionTo();
//
//    return stream;
//}
//
//QDataStream& operator>>(QDataStream& stream, NodeMetaInfo& nodeMetaInfo)
//{
//    QString typeName;
//    int minorVersion;
//    int majorVersion;
//
//    stream >> minorVersion;
//    stream >> majorVersion;
//    stream >> typeName;
//
//    nodeMetaInfo = MetaInfo::global().nodeMetaInfo(typeName/*, majorVersion ,minorVersion*/);
//
//    return stream;
//}

} // namespace QmlDesigner

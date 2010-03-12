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

#include "metainfo.h"

#include "abstractproperty.h"
#include "modelnode.h"
#include "invalidmodelnodeexception.h"
#include "invalidargumentexception.h"
#include "propertymetainfo.h"
#include "metainfoparser.h"
#include "iwidgetplugin.h"

#include "model/variantparser.h"
#include "pluginmanager/widgetpluginmanager.h"

#include <QtDebug>
#include <QPair>
#include <QtAlgorithms>
#include <QMetaProperty>
#include <QDeclarativeEngine>

#include <private/qdeclarativemetatype_p.h>
#include <private/qdeclarativeanchors_p.h>

enum {
    debug = false
};

namespace QmlDesigner {
namespace Internal {

class MetaInfoPrivate
{
    Q_DISABLE_COPY(MetaInfoPrivate)
public:
    typedef QSharedPointer<MetaInfoPrivate> Pointer;
    typedef QWeakPointer<MetaInfoPrivate> WeakPointer;


    MetaInfoPrivate(MetaInfo *q);
    void clear();

    void initialize();
    void parseQmlTypes();
    void parseNonQmlTypes();
    void parseValueTypes();
    void parseNonQmlClassRecursively(const QMetaObject *qMetaObject);
    void parseProperties(NodeMetaInfo &nodeMetaInfo, const QMetaObject *qMetaObject) const;
    void parseClassInfo(NodeMetaInfo &nodeMetaInfo, const QMetaObject *qMetaObject) const;

    QString typeName(const QMetaObject *qMetaObject) const;

    void parseXmlFiles();

    QMultiHash<QString, QString> m_superClassHash; // the list of direct superclasses
    QHash<QString, NodeMetaInfo> m_nodeMetaInfoHash;
    QHash<QString, EnumeratorMetaInfo> m_enumeratorMetaInfoHash;
    QMultiHash<NodeMetaInfo, ItemLibraryInfo> m_itemLibraryInfoHash;
    QHash<QString, ItemLibraryInfo> m_itemLibraryInfoHashAll;
    QHash<QString, QString> m_QtTypesToQmlTypes;

    MetaInfo *m_q;
    bool m_isInitialized;
};

MetaInfoPrivate::MetaInfoPrivate(MetaInfo *q) :
        m_q(q),
        m_isInitialized(false)
{
}

void MetaInfoPrivate::clear()
{
    m_superClassHash.clear();
    m_nodeMetaInfoHash.clear();
    m_enumeratorMetaInfoHash.clear();
    m_itemLibraryInfoHash.clear();
    m_itemLibraryInfoHashAll.clear();
    m_isInitialized = false;
}

void MetaInfoPrivate::initialize()
{
    // make sure QmlGraphicsItemsModule gets initialized, that is
    // QmlGraphicsItemsModule::defineModule called
    QDeclarativeEngine engine;
    Q_UNUSED(engine);

    parseQmlTypes();
    parseNonQmlTypes();
    parseValueTypes();
    parseXmlFiles();

    m_isInitialized = true;
}



void MetaInfoPrivate::parseProperties(NodeMetaInfo &nodeMetaInfo, const QMetaObject *qMetaObject) const
{
    Q_ASSERT_X(qMetaObject, Q_FUNC_INFO, "invalid QMetaObject");
    Q_ASSERT_X(nodeMetaInfo.isValid(), Q_FUNC_INFO, "invalid NodeMetaInfo");

    for (int i = qMetaObject->propertyOffset(); i < qMetaObject->propertyCount(); ++i) {
        QMetaProperty qProperty = qMetaObject->property(i);

        PropertyMetaInfo propertyInfo;

        propertyInfo.setName(QLatin1String(qProperty.name()));

        QString typeName(qProperty.typeName());
        QString noStar = typeName;
        bool star = false;
        while (noStar.contains('*')) {//strip star
            noStar.chop(1);
            star = true;
        }
        if (m_QtTypesToQmlTypes.contains(noStar)) {
            typeName = star ? m_QtTypesToQmlTypes.value(noStar) + '*' : m_QtTypesToQmlTypes.value(noStar);
            //### versions
        }
        propertyInfo.setType(typeName);
        propertyInfo.setValid(true);
        propertyInfo.setReadable(qProperty.isReadable());
        propertyInfo.setWritable(qProperty.isWritable());
        propertyInfo.setResettable(qProperty.isResettable());
        propertyInfo.setEnumType(qProperty.isEnumType());
        propertyInfo.setFlagType(qProperty.isFlagType());

        if (propertyInfo.isEnumType()) {
            EnumeratorMetaInfo enumerator;

            QMetaEnum qEnumerator = qProperty.enumerator();
            enumerator.setValid(qEnumerator.isValid());
            enumerator.setIsFlagType(qEnumerator.isFlag());
            enumerator.setScope(qEnumerator.scope());
            enumerator.setName(qEnumerator.name());
            for (int i = 0 ;i < qEnumerator.keyCount(); i++)
            {
                enumerator.addElement(qEnumerator.valueToKey(i), i);
            }

            propertyInfo.setEnumerator(enumerator);
        }

        nodeMetaInfo.addProperty(propertyInfo);
    }
}

void MetaInfoPrivate::parseClassInfo(NodeMetaInfo &nodeMetaInfo, const QMetaObject *qMetaObject) const
{
    Q_ASSERT_X(qMetaObject, Q_FUNC_INFO, "invalid QMetaObject");
    Q_ASSERT_X(nodeMetaInfo.isValid(), Q_FUNC_INFO, "invalid NodeMetaInfo");
    for (int index = qMetaObject->classInfoCount() - 1 ; index >= 0 ; --index) {
        QMetaClassInfo classInfo = qMetaObject->classInfo(index);
        if (QLatin1String(classInfo.name()) == QLatin1String("DefaultProperty")) {
            nodeMetaInfo.setDefaultProperty(classInfo.value());
            return;
        }
    }
}

void MetaInfoPrivate::parseNonQmlClassRecursively(const QMetaObject *qMetaObject)
{
    Q_ASSERT_X(qMetaObject, Q_FUNC_INFO, "invalid QMetaObject");
    const QString className = qMetaObject->className();
    if ( !m_q->hasNodeMetaInfo(className)
        && !QDeclarativeMetaType::qmlTypeNames().contains(typeName(qMetaObject).toAscii()) ) {
        NodeMetaInfo nodeMetaInfo(*m_q);
        nodeMetaInfo.setTypeName(typeName(qMetaObject));
        parseProperties(nodeMetaInfo, qMetaObject);
        parseClassInfo(nodeMetaInfo, qMetaObject);

        if (debug)
            qDebug() << "adding non qml type" << className << typeName(qMetaObject) << ", parent type" << typeName(qMetaObject->superClass());
        m_q->addNodeInfo(nodeMetaInfo, typeName(qMetaObject->superClass()));
    }

    if (const QMetaObject *superClass = qMetaObject->superClass()) {
        parseNonQmlClassRecursively(superClass);
    }
}


QString MetaInfoPrivate::typeName(const QMetaObject *qMetaObject) const
{
    if (!qMetaObject)
        return QString();
    QString className = qMetaObject->className();
    if (QDeclarativeType *qmlType = QDeclarativeMetaType::qmlType(qMetaObject)) {
        QString qmlClassName(qmlType->qmlTypeName());
        if (!qmlClassName.isEmpty())
            className = qmlType->qmlTypeName(); // Ensure that we always use the qml name,
                                            // if available.
    }
    return className;
}

void MetaInfoPrivate::parseValueTypes()
{
    QStringList valueTypes;
    //there is no global list of all supported value types
    valueTypes << "QFont"
               << "QPoint"
               << "QPointF"
               << "QRect"
               << "QRectF"
               << "QSize"
               << "QSizeF"
               << "QVector3D";

    foreach (const QString &type, valueTypes) {
        NodeMetaInfo nodeMetaInfo(*m_q);
        nodeMetaInfo.setTypeName(type);
        foreach (const QString &propertyName, VariantParser::create(type).properties()) {
            PropertyMetaInfo propertyInfo;
            propertyInfo.setName(propertyName);
            propertyInfo.setType("real");
            if (type == ("QFont")) {
                if (propertyName == "bold")
                    propertyInfo.setType("bool");
                else if (propertyName == "italic")
                    propertyInfo.setType("bool");
                else if (propertyName == "family")
                    propertyInfo.setType("string");
                else if (propertyName == "pixelSize")
                    propertyInfo.setType("int");
            } else if (type == ("QPoint")) {
                propertyInfo.setType("int");
            } else if (type == ("QSize")) {
                propertyInfo.setType("int");
            } else if (type == ("QRect")) {
                propertyInfo.setType("int");
            }
            propertyInfo.setValid(true);
            propertyInfo.setReadable(true);
            propertyInfo.setWritable(true);
            nodeMetaInfo.addProperty(propertyInfo);
        }
        if (debug)
            qDebug() << "adding value type" << nodeMetaInfo.typeName();
        m_q->addNodeInfo(nodeMetaInfo, QString());
    }
}

void MetaInfoPrivate::parseQmlTypes()
{
    foreach (QDeclarativeType *qmlType, QDeclarativeMetaType::qmlTypes()) {
        const QString qtTypeName(qmlType->typeName());
        const QString qmlTypeName(qmlType->qmlTypeName());
        m_QtTypesToQmlTypes.insert(qtTypeName, qmlTypeName);
    }
    foreach (QDeclarativeType *qmlType, QDeclarativeMetaType::qmlTypes()) {
        const QMetaObject *qMetaObject = qmlType->metaObject();

        // parseQmlTypes is called iteratively e.g. when plugins are loaded
        if (m_q->hasNodeMetaInfo(qmlType->qmlTypeName(), qmlType->majorVersion(), qmlType->minorVersion()))
            continue;

        NodeMetaInfo nodeMetaInfo(*m_q);
        nodeMetaInfo.setTypeName(qmlType->qmlTypeName());
        nodeMetaInfo.setMajorVersion(qmlType->majorVersion());
        nodeMetaInfo.setMinorVersion(qmlType->minorVersion());

        parseProperties(nodeMetaInfo, qMetaObject);
        parseClassInfo(nodeMetaInfo, qMetaObject);

        QString superTypeName = typeName(qMetaObject->superClass());
        if (qmlType->baseMetaObject() != qMetaObject) {
            // type is declared with Q_DECLARE_EXTENDED_TYPE
            // also parse properties of original type
            parseProperties(nodeMetaInfo, qmlType->baseMetaObject());
            superTypeName = typeName(qmlType->baseMetaObject()->superClass());
        }

        m_q->addNodeInfo(nodeMetaInfo, superTypeName);
    }
}

void MetaInfoPrivate::parseNonQmlTypes()
{
    foreach (QDeclarativeType *qmlType, QDeclarativeMetaType::qmlTypes()) {
        parseNonQmlClassRecursively(qmlType->metaObject());
    }

    parseNonQmlClassRecursively(&QDeclarativeAnchors::staticMetaObject);
}


void MetaInfoPrivate::parseXmlFiles()
{
    Internal::MetaInfoParser(*m_q).parseFile(":/metainfo/gui.metainfo");

    Internal::WidgetPluginManager pluginManager;
    foreach (const QString &pluginDir, m_q->s_pluginDirs)
        pluginManager.addPath(pluginDir);
    QList<IWidgetPlugin *> widgetPluginList = pluginManager.instances();
    foreach (IWidgetPlugin *plugin, widgetPluginList) {
        parseQmlTypes();
        Internal::MetaInfoParser parser(*m_q);
        parser.parseFile(plugin->metaInfo());
    }
}

} // namespace Internal

using QmlDesigner::Internal::MetaInfoPrivate;

MetaInfo MetaInfo::s_global;
QStringList MetaInfo::s_pluginDirs;


/*!
\class QmlDesigner::MetaInfo
\ingroup CoreModel
\brief The MetaInfo class provides meta information about qml types and properties.

The MetaInfo, NodeMetaInfo, PropertyMetaInfo and EnumeratorMetaInfo
classes provide information about the (static and dynamic) qml types available in
a specific model. Just like their Model, ModelNode and AbstractProperty counterparts,
objects of these classes are handles - that means, they are implicitly shared, and
should be created on the stack.

The MetaInfo object should always be accessed via the model (see Model::metaInfo()).
Otherwise types specific to a model (like sub components) might
be missed.

\see Model::metaInfo(), QmlDesigner::NodeMetaInfo, QmlDesigner::PropertyMetaInfo, QmlDesigner::EnumeratorMetaInfo
*/

/*!
  \brief Constructs a copy of the given meta info.
  */
MetaInfo::MetaInfo(const MetaInfo &metaInfo) :
        m_p(metaInfo.m_p)
{
}

/*!
  \brief Creates a meta information object with just the qml types registered statically.
  You almost always want to use Model::metaInfo() instead!

  You almost certainly want to access the meta information for the model.

  \see Model::metaInfo()
  */
MetaInfo::MetaInfo() :
        m_p(new MetaInfoPrivate(this))
{
}

MetaInfo::~MetaInfo()
{
}

/*!
  \brief Assigns other to this meta information and returns a reference to this meta information.
  */
MetaInfo& MetaInfo::operator=(const MetaInfo &other)
{
    m_p = other.m_p;
    return *this;
}

/*!
  \brief Returns whether a type with the given name is registered in the meta system.
  */
bool MetaInfo::hasNodeMetaInfo(const QString &typeName, int /*majorVersion*/, int /*minorVersion*/) const
{
    if (m_p->m_nodeMetaInfoHash.contains(typeName))
        return true;
    if (!isGlobal())
        return global().hasNodeMetaInfo(typeName);
    return false;
}

/*!
  \brief Returns meta information for a qml type. An invalid NodeMetaInfo object if the type is unknown.
  */
NodeMetaInfo MetaInfo::nodeMetaInfo(const QString &typeName, int /*majorVersion*/, int /*minorVersion*/) const
{
    if (m_p->m_nodeMetaInfoHash.contains(typeName))
        return m_p->m_nodeMetaInfoHash.value(typeName, NodeMetaInfo());
    if (!isGlobal())
        return global().nodeMetaInfo(typeName);

    return NodeMetaInfo();
}

QStringList MetaInfo::superClasses(const QString &className) const
{
    QStringList ancestorList = m_p->m_superClassHash.values(className);
    foreach (const QString &ancestor, ancestorList) {
        QStringList superClassList = superClasses(ancestor);
        if (!superClassList.isEmpty())
            ancestorList += superClassList;
    }
    if (!isGlobal())
        ancestorList += global().superClasses(className);
    return ancestorList;
}

QStringList MetaInfo::directSuperClasses(const QString &className) const
{
    QStringList directAncestorList = m_p->m_superClassHash.values(className);
    if (!isGlobal())
        directAncestorList += global().directSuperClasses(className);
    return directAncestorList;
}

QList<NodeMetaInfo> MetaInfo::superClasses(const NodeMetaInfo &nodeInfo) const
{
    if (!nodeInfo.isValid()) {
        Q_ASSERT_X(nodeInfo.isValid(), Q_FUNC_INFO, "Invalid nodeInfo argument");
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, "nodeInfo");
    }

    QList<NodeMetaInfo> superClassList;

    foreach (const QString &typeName, superClasses(nodeInfo.typeName())) {
        if (!hasNodeMetaInfo(typeName))
            continue;
        const NodeMetaInfo superClass = nodeMetaInfo(typeName);
        if (!superClassList.contains(superClass))
            superClassList.append(superClass);
    }
    return superClassList;
}

QList<NodeMetaInfo> MetaInfo::directSuperClasses(const NodeMetaInfo &nodeInfo) const
{
    if (!nodeInfo.isValid()) {
        Q_ASSERT_X(nodeInfo.isValid(), Q_FUNC_INFO, "Invalid nodeInfo argument");
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, "nodeInfo");
    }

    QList<NodeMetaInfo> superClassList;

    foreach (const QString &typeName, directSuperClasses(nodeInfo.typeName())) {
        if (!hasNodeMetaInfo(typeName))
            continue;
        const NodeMetaInfo superClass = nodeMetaInfo(typeName);
        if (!superClassList.contains(superClass))
            superClassList.append(superClass);
    }
    return superClassList;
}

QList<ItemLibraryInfo> MetaInfo::itemLibraryRepresentations(const NodeMetaInfo &nodeMetaInfo) const
{
    QList<ItemLibraryInfo> itemLibraryItems = m_p->m_itemLibraryInfoHash.values(nodeMetaInfo);
    if (!isGlobal())
        itemLibraryItems += global().itemLibraryRepresentations(nodeMetaInfo);
    return itemLibraryItems;
}

ItemLibraryInfo MetaInfo::itemLibraryRepresentation(const QString &name) const
{
    if (m_p->m_itemLibraryInfoHashAll.contains(name))
        return m_p->m_itemLibraryInfoHashAll.value(name);
    if (!isGlobal())
        return global().itemLibraryRepresentation(name);
    return ItemLibraryInfo();
}

QStringList MetaInfo::itemLibraryItems() const
{
    QStringList completeList = m_p->m_nodeMetaInfoHash.keys();
    QStringList finalList;
    foreach (const QString &name, completeList) {
        if (nodeMetaInfo(name).isVisibleToItemLibrary())
            finalList.append(name);
    }

    if (!isGlobal())
        finalList += global().itemLibraryItems();

    return finalList;
}

/*!
  \brief Returns whether className is the same type or a type derived from superClassName.
  */
bool MetaInfo::isSubclassOf(const QString &className, const QString &superClassName) const
{
    return (className == superClassName) || superClasses(className).contains(superClassName);
}

/*!
  \brief Returns whether the type of modelNode is the same type or a type derived from superClassName.
  \throws InvalidModelNode if !modelNode.isValid()
  */
bool MetaInfo::isSubclassOf(const ModelNode &modelNode, const QString &superClassName) const
{
    if (!modelNode.isValid()) {
        Q_ASSERT_X(modelNode.isValid(), Q_FUNC_INFO, "Invalid modelNode argument");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    return (modelNode.type() == superClassName) || isSubclassOf(modelNode.type(), superClassName);
}

/*!
  \brief Returns whether an enumerator is registered in the meta type system.
  */
bool MetaInfo::hasEnumerator(const QString &enumeratorName) const
{
    return m_p->m_enumeratorMetaInfoHash.contains(enumeratorName)
            || (!isGlobal() ? global().hasEnumerator(enumeratorName) : false);
}

/*!
  \brief Returns meta information about an enumerator. An invalid EnumeratorMetaInfo object if the enumerator is not known.
  */
EnumeratorMetaInfo MetaInfo::enumerator(const QString &enumeratorName) const
{
    if (m_p->m_enumeratorMetaInfoHash.contains(enumeratorName))
        return m_p->m_enumeratorMetaInfoHash.value(enumeratorName);
    if (!isGlobal())
        return global().enumerator(enumeratorName);
    return EnumeratorMetaInfo();
}

/*!
  \brief Access to the global meta information object.
  You almost always want to use Model::metaInfo() instead.

  Internally all meta information objects share this "global" object
  where static qml type information is stored.
  */
MetaInfo MetaInfo::global()
{
    if (!s_global.m_p->m_isInitialized) {
        s_global.m_p = QSharedPointer<MetaInfoPrivate>(new MetaInfoPrivate(&s_global));
        s_global.m_p->initialize();
    }
    return s_global;
}

/*!
  \brief Clears the global meta information object.

  This method should be called once on application shutdown to free static data structures.
  */
void MetaInfo::clearGlobal()
{
    MetaInfo::global().m_p->clear();
}

void MetaInfo::setPluginPaths(const QStringList &paths)
{
    s_pluginDirs = paths;
}

/*!
  This bypasses the notifications to the model that the metatype has changed.
  Use MetaInfo::addNodeInfo() instead
  */
void MetaInfo::addSuperClassRelationship(const QString &superClassName, const QString &className)
{
    m_p->m_superClassHash.insert(className, superClassName);
}

void MetaInfo::addNodeInfo(NodeMetaInfo &nodeInfo, const QString &baseType)
{
    if (nodeInfo.typeName().isEmpty() || nodeInfo.metaInfo() != *this)
        throw new InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, QLatin1String("nodeInfo"));

    if (nodeInfo.typeName() == baseType) // prevent simple recursion
        throw new InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, QLatin1String("baseType"));

    m_p->m_nodeMetaInfoHash.insert(nodeInfo.typeName(), nodeInfo);

    if (!baseType.isEmpty()) {
        m_p->m_superClassHash.insert(nodeInfo.typeName(), baseType);
    }
}

void MetaInfo::removeNodeInfo(NodeMetaInfo &info)
{
    Q_ASSERT(info.isValid());

    if (m_p->m_nodeMetaInfoHash.contains(info.typeName())) {
        m_p->m_nodeMetaInfoHash.remove(info.typeName());

        m_p->m_superClassHash.remove(info.typeName());
        // TODO: Other types might specify type as parent type
        m_p->m_itemLibraryInfoHash.remove(info);
        m_p->m_itemLibraryInfoHashAll.remove(info.typeName());

    } else if (!isGlobal()) {
        global().removeNodeInfo(info);
    } else {
        Q_ASSERT_X(0, Q_FUNC_INFO, "Node meta info not in db");
    }

    info.setInvalid();
}

void MetaInfo::replaceNodeInfo(NodeMetaInfo & /*oldInfo*/, NodeMetaInfo & /*newInfo*/, const QString & /*baseType*/)
{
    // TODO
}

EnumeratorMetaInfo MetaInfo::addEnumerator(const QString &enumeratorScope, const QString &enumeratorName)
{
    Q_ASSERT(!enumeratorName.isEmpty());

    EnumeratorMetaInfo enumeratorMetaInfo;
    enumeratorMetaInfo.setName(enumeratorName);
    enumeratorMetaInfo.setScope(enumeratorScope);
    enumeratorMetaInfo.setIsFlagType(false);
    enumeratorMetaInfo.setValid(true);

    m_p->m_enumeratorMetaInfoHash.insert(enumeratorMetaInfo.scopeAndName(), enumeratorMetaInfo);

    return enumeratorMetaInfo;
}

EnumeratorMetaInfo MetaInfo::addFlag(const QString &enumeratorScope, const QString &enumeratorName)
{
    Q_ASSERT(!enumeratorName.isEmpty());

    EnumeratorMetaInfo enumeratorMetaInfo;
    enumeratorMetaInfo.setName(enumeratorName);
    enumeratorMetaInfo.setScope(enumeratorScope);
    enumeratorMetaInfo.setIsFlagType(true);
    m_p->m_enumeratorMetaInfoHash.insert(enumeratorMetaInfo.scopeAndName(), enumeratorMetaInfo);

    return enumeratorMetaInfo;
}

ItemLibraryInfo MetaInfo::addItemLibraryInfo(const NodeMetaInfo &nodeMetaInfo, const QString &itemLibraryRepresentationName)
{
    ItemLibraryInfo itemLibraryInfo;
    itemLibraryInfo.setName(itemLibraryRepresentationName);
    itemLibraryInfo.setTypeName(nodeMetaInfo.typeName());
    itemLibraryInfo.setMajorVersion(nodeMetaInfo.majorVersion());
    itemLibraryInfo.setMinorVersion(nodeMetaInfo.minorVersion());
    m_p->m_itemLibraryInfoHash.insert(nodeMetaInfo, itemLibraryInfo);
    m_p->m_itemLibraryInfoHashAll.insert(itemLibraryRepresentationName, itemLibraryInfo);
    return itemLibraryInfo;
}

bool MetaInfo::isGlobal() const
{
    return (this->m_p == s_global.m_p);
}

bool operator==(const MetaInfo &first, const MetaInfo &second)
{
    return first.m_p == second.m_p;
}

bool operator!=(const MetaInfo &first, const MetaInfo &second)
{
    return !(first == second);
}
} //namespace QmlDesigner

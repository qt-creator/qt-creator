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
    void parseNonQmlClassRecursively(const QMetaObject *qMetaObject, int majorVersion, int minorVersion);
    void parseProperties(NodeMetaInfo &nodeMetaInfo, const QMetaObject *qMetaObject) const;
    void parseClassInfo(NodeMetaInfo &nodeMetaInfo, const QMetaObject *qMetaObject) const;

    QString typeName(const QMetaObject *qMetaObject) const;

    void parseXmlFiles();

    QMultiHash<QString, NodeMetaInfo> m_nodeMetaInfoHash;
    QHash<QString, EnumeratorMetaInfo> m_enumeratorMetaInfoHash;
    QHash<QString, QString> m_QtTypesToQmlTypes;
    QScopedPointer<ItemLibraryInfo> m_itemLibraryInfo;

    MetaInfo *m_q;
    bool m_isInitialized;
};

MetaInfoPrivate::MetaInfoPrivate(MetaInfo *q) :
        m_itemLibraryInfo(new ItemLibraryInfo()),
        m_q(q),
        m_isInitialized(false)
{
    if (!m_q->isGlobal())
        m_itemLibraryInfo->setBaseInfo(MetaInfo::global().itemLibraryInfo());
}

void MetaInfoPrivate::clear()
{
    m_nodeMetaInfoHash.clear();
    m_enumeratorMetaInfoHash.clear();
    m_itemLibraryInfo->clearEntries();
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
    parseXmlFiles();
    parseValueTypes();

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
            QMetaEnum qEnumerator = qProperty.enumerator();
            EnumeratorMetaInfo enumerator = m_q->addEnumerator(qEnumerator.scope(), qEnumerator.name());

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

void MetaInfoPrivate::parseNonQmlClassRecursively(const QMetaObject *qMetaObject, int majorVersion, int minorVersion)
{
    Q_ASSERT_X(qMetaObject, Q_FUNC_INFO, "invalid QMetaObject");
    const QString className = qMetaObject->className();

    if (className.isEmpty()) {
        qWarning() << "Meta type system: Registered class has no name.";
        return;
    }

    if (!m_q->hasNodeMetaInfo(typeName(qMetaObject), majorVersion, minorVersion)) {
        NodeMetaInfo nodeMetaInfo(*m_q);
        nodeMetaInfo.setType(typeName(qMetaObject), majorVersion, minorVersion);
        parseProperties(nodeMetaInfo, qMetaObject);
        parseClassInfo(nodeMetaInfo, qMetaObject);

        if (debug)
            qDebug() << "adding non qml type" << nodeMetaInfo.typeName() << nodeMetaInfo.majorVersion() << nodeMetaInfo.minorVersion() << ", parent type" << typeName(qMetaObject->superClass());
        if (qMetaObject->superClass())
            nodeMetaInfo.setSuperClass(typeName(qMetaObject->superClass()));

        m_q->addNodeInfo(nodeMetaInfo);
    }

    if (const QMetaObject *superClass = qMetaObject->superClass()) {
        parseNonQmlClassRecursively(superClass, majorVersion, minorVersion);
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
               << "QVector3D"
               << "QEasingCurve";

    foreach (const QString &type, valueTypes) {
        NodeMetaInfo nodeMetaInfo(*m_q);
        nodeMetaInfo.setType(type, -1, -1);
        foreach (const QString &propertyName, VariantParser::create(type).properties()) {
            PropertyMetaInfo propertyInfo;
            propertyInfo.setName(propertyName);
            propertyInfo.setType("real");
            if (type == ("QFont")) {
                if (propertyName == "bold")
                    propertyInfo.setType("bool");
                else if (propertyName == "italic")
                    propertyInfo.setType("bool");
                else if (propertyName == "underline")
                    propertyInfo.setType("bool");
                else if (propertyName == "strikeout")
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
            } else if (type == ("QEasingCurve")) {
                if (propertyName == "type") {
                    propertyInfo.setEnumType("true");
                    propertyInfo.setType("QEasingCurve::Type");
                    propertyInfo.setEnumerator(m_q->enumerator("QEasingCurve::Type"));
                }
            }
            propertyInfo.setValid(true);
            propertyInfo.setReadable(true);
            propertyInfo.setWritable(true);
            nodeMetaInfo.addProperty(propertyInfo);
        }
        if (debug)
            qDebug() << "adding value type" << nodeMetaInfo.typeName();
        m_q->addNodeInfo(nodeMetaInfo);
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
        nodeMetaInfo.setType(qmlType->qmlTypeName(), qmlType->majorVersion(), qmlType->minorVersion());

        parseProperties(nodeMetaInfo, qMetaObject);
        parseClassInfo(nodeMetaInfo, qMetaObject);

        QString superTypeName = typeName(qMetaObject->superClass());
        if (qmlType->baseMetaObject() != qMetaObject) {
            // type is declared with Q_DECLARE_EXTENDED_TYPE
            // also parse properties of original type
            parseProperties(nodeMetaInfo, qmlType->baseMetaObject());
            superTypeName = typeName(qmlType->baseMetaObject()->superClass());
        }

        nodeMetaInfo.setSuperClass(superTypeName);

        if (debug)
            qDebug() << "adding qml type" << nodeMetaInfo.typeName() << nodeMetaInfo.majorVersion() << nodeMetaInfo.minorVersion() << "super class" << superTypeName;
        m_q->addNodeInfo(nodeMetaInfo);
    }
}

void MetaInfoPrivate::parseNonQmlTypes()
{
    foreach (QDeclarativeType *qmlType, QDeclarativeMetaType::qmlTypes()) {
        if (!qmlType->qmlTypeName().contains("Bauhaus"))
            parseNonQmlClassRecursively(qmlType->metaObject(), qmlType->majorVersion(), qmlType->minorVersion());
    }

    parseNonQmlClassRecursively(&QDeclarativeAnchors::staticMetaObject, -1, -1);
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
bool MetaInfo::hasNodeMetaInfo(const QString &typeName, int majorVersion, int minorVersion) const
{
    foreach (const NodeMetaInfo &info, m_p->m_nodeMetaInfoHash.values(typeName)) {
        if (info.availableInVersion(majorVersion, minorVersion)) {
            return true;
        }
    }
    if (!isGlobal())
        return global().hasNodeMetaInfo(typeName);
    return false;
}

/*!
  \brief Returns meta information for a qml type. An invalid NodeMetaInfo object if the type is unknown.
  */
NodeMetaInfo MetaInfo::nodeMetaInfo(const QString &typeName, int majorVersion, int minorVersion) const
{
    foreach (const NodeMetaInfo &info, m_p->m_nodeMetaInfoHash.values(typeName)) {
        // todo: The order for different types for different versions is random here.
        if (info.availableInVersion(majorVersion, minorVersion)) {
            return info;
        }
    }
    if (!isGlobal())
        return global().nodeMetaInfo(typeName);

    return NodeMetaInfo();
}

QString MetaInfo::fromQtTypes(const QString &type) const
{
    if (m_p->m_QtTypesToQmlTypes.contains(type))
        return m_p->m_QtTypesToQmlTypes.value(type);

    return type;
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

ItemLibraryInfo *MetaInfo::itemLibraryInfo() const
{
    return m_p->m_itemLibraryInfo.data();
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
    if (s_global.m_p->m_isInitialized) {
        s_global.m_p->clear();
    }
}

void MetaInfo::setPluginPaths(const QStringList &paths)
{
    s_pluginDirs = paths;
}

void MetaInfo::addNodeInfo(NodeMetaInfo &nodeInfo)
{
    if (nodeInfo.typeName().isEmpty() || nodeInfo.metaInfo() != *this)
        throw new InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, QLatin1String("nodeInfo"));


    m_p->m_nodeMetaInfoHash.insertMulti(nodeInfo.typeName(), nodeInfo);
}

void MetaInfo::removeNodeInfo(NodeMetaInfo &info)
{
    if (!info.isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return;
    }

    if (m_p->m_nodeMetaInfoHash.contains(info.typeName())
        && m_p->m_nodeMetaInfoHash.remove(info.typeName(), info)) {

        foreach (const ItemLibraryEntry &entry,
                 m_p->m_itemLibraryInfo->entriesForType(info.typeName(), info.majorVersion(), info.minorVersion())) {
            m_p->m_itemLibraryInfo->removeEntry(entry.name());
        }

    } else if (!isGlobal()) {
        global().removeNodeInfo(info);
    } else {
        Q_ASSERT_X(0, Q_FUNC_INFO, "Node meta info not in db");
    }

    info.setInvalid();
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

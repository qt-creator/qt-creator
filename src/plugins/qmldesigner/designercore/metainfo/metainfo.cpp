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
    void loadPlugins(QDeclarativeEngine *engine);
    void parseQmlTypes();
    void parseNonQmlTypes();
    void parseValueTypes();
    void parseNonQmlClassRecursively(const QMetaObject *qMetaObject);
    void parseProperties(NodeMetaInfo &nodeMetaInfo, const QMetaObject *qMetaObject) const;
    void parseClassInfo(NodeMetaInfo &nodeMetaInfo, const QMetaObject *qMetaObject) const;

    QList<QDeclarativeType*> qmlTypes();
    void typeInfo(const QMetaObject *qMetaObject, QString *typeName, int *majorVersion = 0, int *minorVersion = 0) const;

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

    loadPlugins(&engine);
    parseQmlTypes();
    parseNonQmlTypes();
    parseValueTypes();
    parseXmlFiles();

    m_isInitialized = true;
}

void MetaInfoPrivate::loadPlugins(QDeclarativeEngine *engine)
{
    // hack to load plugins
    QDeclarativeComponent pluginComponent(engine, 0);

    QStringList pluginList;
    pluginList += "import Qt 4.7";
    pluginList += "import QtWebKit 1.0";

    // load maybe useful plugins
    pluginList += "import Qt.labs.folderlistmodel 1.0";
    pluginList += "import Qt.labs.gestures 1.0";
    pluginList += "import Qt.multimedia 4.7";
    pluginList += "import Qt.labs.particles 1.0";

    QString componentString = QString("%1\n Item {}\n").arg(pluginList.join("\n"));


    pluginComponent.setData(componentString.toLatin1(), QUrl());
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

    QString typeName;
    int majorVersion = -1;
    int minorVersion = -1;
    typeInfo(qMetaObject, &typeName, &majorVersion, &minorVersion);

    if (typeName.isEmpty()) {
        qWarning() << "Meta type system: Registered class has no name.";
        return;
    }

    NodeMetaInfo existingInfo = m_q->nodeMetaInfo(typeName, majorVersion, minorVersion);
    if (existingInfo.isValid()
        && existingInfo.majorVersion() == majorVersion
        && existingInfo.minorVersion() == minorVersion) {
        return;
    }

    NodeMetaInfo nodeMetaInfo(*m_q);
    nodeMetaInfo.setType(typeName, majorVersion, minorVersion);
    parseProperties(nodeMetaInfo, qMetaObject);
    parseClassInfo(nodeMetaInfo, qMetaObject);

    QString superTypeName;
    int superTypeMajorVersion = -1;
    int superTypeMinorVersion = -1;

    if (qMetaObject->superClass()) {
        typeInfo(qMetaObject->superClass(), &superTypeName, &superTypeMajorVersion, &superTypeMinorVersion);
        nodeMetaInfo.setSuperClass(superTypeName, superTypeMajorVersion, superTypeMinorVersion);
    }
    if (debug)
        qDebug() << "adding non qml type" << nodeMetaInfo.typeName() << nodeMetaInfo.majorVersion() << nodeMetaInfo.minorVersion()
                 << ", parent type" << superTypeName << superTypeMajorVersion << superTypeMinorVersion;

    m_q->addNodeInfo(nodeMetaInfo);

    if (const QMetaObject *superClass = qMetaObject->superClass())
        parseNonQmlClassRecursively(superClass);
}

QList<QDeclarativeType*> MetaInfoPrivate::qmlTypes()
{
    QList<QDeclarativeType*> list;
    foreach (QDeclarativeType *type, QDeclarativeMetaType::qmlTypes()) {
        if (!type->qmlTypeName().startsWith("Bauhaus/")
            && !type->qmlTypeName().startsWith("QmlProject/"))
            list += type;
    }
    return list;
}

void MetaInfoPrivate::typeInfo(const QMetaObject *qMetaObject, QString *typeName, int *majorVersion, int *minorVersion) const
{
    Q_ASSERT(typeName);

    if (!qMetaObject)
        return;

    *typeName = qMetaObject->className();
    int majVersion = -1;
    int minVersion = -1;
    QDeclarativeType *qmlType = QDeclarativeMetaType::qmlType(qMetaObject);
    if (qmlType) {
        if (!qmlType->qmlTypeName().isEmpty()) {
            *typeName = qmlType->qmlTypeName();
            majVersion = qmlType->majorVersion();
            minVersion = qmlType->minorVersion();
        }
    }
    if (majorVersion)
        *majorVersion = majVersion;
    if (minorVersion)
        *minorVersion = minVersion;
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
    foreach (QDeclarativeType *qmlType, qmlTypes()) {
        const QString qtTypeName(qmlType->typeName());
        const QString qmlTypeName(qmlType->qmlTypeName());
        m_QtTypesToQmlTypes.insert(qtTypeName, qmlTypeName);
    }
    foreach (QDeclarativeType *qmlType, qmlTypes()) {
        const QMetaObject *qMetaObject = qmlType->metaObject();

        // parseQmlTypes is called iteratively e.g. when plugins are loaded
        if (m_q->hasNodeMetaInfo(qmlType->qmlTypeName(), qmlType->majorVersion(), qmlType->minorVersion()))
            continue;

        NodeMetaInfo nodeMetaInfo(*m_q);
        nodeMetaInfo.setType(qmlType->qmlTypeName(), qmlType->majorVersion(), qmlType->minorVersion());

        parseProperties(nodeMetaInfo, qMetaObject);
        if (qmlType->baseMetaObject() != qMetaObject) {
            // type is declared with Q_DECLARE_EXTENDED_TYPE
            parseProperties(nodeMetaInfo, qmlType->baseMetaObject());
        }

        parseClassInfo(nodeMetaInfo, qMetaObject);

        QString superTypeName;
        int superTypeMajorVersion = -1;
        int superTypeMinorVersion = -1;
        if (const QMetaObject *superClassObject = qmlType->baseMetaObject()->superClass())
            typeInfo(superClassObject, &superTypeName, &superTypeMajorVersion, &superTypeMinorVersion);

        if (!superTypeName.isEmpty())
            nodeMetaInfo.setSuperClass(superTypeName, superTypeMajorVersion, superTypeMinorVersion);

        if (debug) {
            qDebug() << "adding qml type" << nodeMetaInfo.typeName() << nodeMetaInfo.majorVersion() << nodeMetaInfo.minorVersion()
                     << ", super class" << superTypeName << superTypeMajorVersion << superTypeMinorVersion;
        }

        m_q->addNodeInfo(nodeMetaInfo);
    }
}

void MetaInfoPrivate::parseNonQmlTypes()
{
    foreach (QDeclarativeType *qmlType, qmlTypes()) {
        if (qmlType->qmlTypeName().startsWith("Bauhaus/")
             || qmlType->qmlTypeName().startsWith("QmlProject/"))
            continue;
        if (qmlType->metaObject()->superClass())
            parseNonQmlClassRecursively(qmlType->metaObject()->superClass());
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
bool MetaInfo::hasNodeMetaInfo(const QString &typeName, int majorVersion, int minorVersion) const
{
    foreach (const NodeMetaInfo &info, m_p->m_nodeMetaInfoHash.values(typeName)) {
        if (info.availableInVersion(majorVersion, minorVersion)) { {
            return true;
        }
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
    NodeMetaInfo returnInfo;
    foreach (const NodeMetaInfo &info, m_p->m_nodeMetaInfoHash.values(typeName)) {
        if (info.availableInVersion(majorVersion, minorVersion)) {
            if (!returnInfo.isValid()
                || returnInfo.majorVersion() < info.majorVersion()
                || (returnInfo.majorVersion() == info.minorVersion()
                    && returnInfo.minorVersion() < info.minorVersion()))
            returnInfo = info;
        }
    }
    if (!returnInfo.isValid()
        && !isGlobal())
        return global().nodeMetaInfo(typeName);

    return returnInfo;
}

QString MetaInfo::fromQtTypes(const QString &type) const
{
    if (m_p->m_QtTypesToQmlTypes.contains(type))
        return m_p->m_QtTypesToQmlTypes.value(type);
    if (!isGlobal())
        return global().fromQtTypes(type);
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

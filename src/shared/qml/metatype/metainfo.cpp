#include "invalidmetainfoexception.h"
#include "metainfo.h"
#include "propertymetainfo.h"

#include <QDebug>
#include <QPair>
#include <QtAlgorithms>
#include <QMetaProperty>
#include <QmlMetaType>

enum {
    debug = false
};

namespace Qml {
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
    m_isInitialized = false;
}

void MetaInfoPrivate::initialize()
{
    parseQmlTypes();
    parseNonQmlTypes();
//    parseValueTypes();
//    parseXmlFiles();

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
        int majorVersion = -1, minorVersion = -1;
        if (QmlType *propertyType = QmlMetaType::qmlType(qMetaObject)) {
            majorVersion = propertyType->majorVersion();
            minorVersion = propertyType->minorVersion();
        }

        propertyInfo.setType(typeName, majorVersion, minorVersion);
        propertyInfo.setValid(true);
        propertyInfo.setReadable(qProperty.isReadable());
        propertyInfo.setWritable(qProperty.isWritable());
        propertyInfo.setResettable(qProperty.isResettable());
        propertyInfo.setEnumType(qProperty.isEnumType());
        propertyInfo.setFlagType(qProperty.isFlagType());

//        if (propertyInfo.isEnumType()) {
//            EnumeratorMetaInfo enumerator;
//
//            QMetaEnum qEnumerator = qProperty.enumerator();
//            enumerator.setValid(qEnumerator.isValid());
//            enumerator.setIsFlagType(qEnumerator.isFlag());
//            enumerator.setScope(qEnumerator.scope());
//            enumerator.setName(qEnumerator.name());
//            for (int i = 0 ;i < qEnumerator.keyCount(); i++)
//            {
//                enumerator.addElement(qEnumerator.valueToKey(i), i);
//            }
//
//            propertyInfo.setEnumerator(enumerator);
//        }

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
        && !QmlMetaType::qmlTypeNames().contains(typeName(qMetaObject).toAscii()) ) {
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
    if (QmlType *qmlType = QmlMetaType::qmlType(qMetaObject)) {
        QString qmlClassName(qmlType->qmlTypeName());
        if (!qmlClassName.isEmpty())
            className = qmlType->qmlTypeName(); // Ensure that we always use the qml name,
                                            // if available.
    }
    return className;
}

void MetaInfoPrivate::parseQmlTypes()
{
    foreach (QmlType *qmlType, QmlMetaType::qmlTypes()) {
        const QString qtTypeName(qmlType->typeName());
        const QString qmlTypeName(qmlType->qmlTypeName());
        m_QtTypesToQmlTypes.insert(qtTypeName, qmlTypeName);
    }
    foreach (QmlType *qmlType, QmlMetaType::qmlTypes()) {
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
    foreach (QmlType *qmlType, QmlMetaType::qmlTypes()) {
        parseNonQmlClassRecursively(qmlType->metaObject());
    }
}

} // namespace Internal

using Qml::Internal::MetaInfoPrivate;

MetaInfo MetaInfo::s_global;
QStringList MetaInfo::s_pluginDirs;


/*!
\class QKineticDesigner::MetaInfo
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

\see Model::metaInfo(), QKineticDesigner::NodeMetaInfo, QKineticDesigner::PropertyMetaInfo, QKineticDesigner::EnumeratorMetaInfo
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

QList<NodeMetaInfo> MetaInfo::allTypes() const
{
    return m_p->m_nodeMetaInfoHash.values();
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
        throw InvalidMetaInfoException(__LINE__, __FUNCTION__, __FILE__);
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
        throw InvalidMetaInfoException(__LINE__, __FUNCTION__, __FILE__);
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
        throw new InvalidMetaInfoException(__LINE__, __FUNCTION__, __FILE__);

    if (nodeInfo.typeName() == baseType) // prevent simple recursion
        throw new InvalidMetaInfoException(__LINE__, __FUNCTION__, __FILE__);

    m_p->m_nodeMetaInfoHash.insert(nodeInfo.typeName(), nodeInfo);

    if (!baseType.isEmpty()) {
        m_p->m_superClassHash.insert(nodeInfo.typeName(), baseType);
    }
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
} //namespace QKineticDesigner

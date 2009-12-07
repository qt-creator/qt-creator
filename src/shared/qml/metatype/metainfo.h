#ifndef METAINFO_H
#define METAINFO_H

#include <qml/qml_global.h>
#include <qml/metatype/nodemetainfo.h>
#include <qml/metatype/propertymetainfo.h>

#include <QMultiHash>
#include <QString>
#include <QStringList>
#include <QtCore/QSharedPointer>

namespace QKineticDesigner {

class ModelNode;
class AbstractProperty;

namespace Internal {
    class MetaInfoPrivate;
    class ModelPrivate;
    class SubComponentManagerPrivate;
    typedef QSharedPointer<MetaInfoPrivate> MetaInfoPrivatePointer;
}

QML_EXPORT bool operator==(const MetaInfo &first, const MetaInfo &second);
QML_EXPORT bool operator!=(const MetaInfo &first, const MetaInfo &second);

class QML_EXPORT MetaInfo
{
    friend class QKineticDesigner::Internal::MetaInfoPrivate;
    friend class QKineticDesigner::Internal::MetaInfoParser;
    friend class QKineticDesigner::NodeMetaInfo;
    friend bool QKineticDesigner::operator==(const MetaInfo &, const MetaInfo &);

public:
    MetaInfo(const MetaInfo &metaInfo);
    ~MetaInfo();
    MetaInfo& operator=(const MetaInfo &other);

    QList<NodeMetaInfo> allTypes() const;

    bool hasNodeMetaInfo(const QString &typeName, int majorVersion = 4, int minorVersion = 6) const;
    // ### makes no sense since ModelNode has minor/major version
    NodeMetaInfo nodeMetaInfo(const ModelNode &node) const;
    NodeMetaInfo nodeMetaInfo(const QString &typeName, int majorVersion = 4, int minorVersion = 6) const;

    // TODO: Move these to private
    bool isSubclassOf(const QString &className, const QString &superClassName) const;
    bool isSubclassOf(const ModelNode &modelNode, const QString &superClassName) const;

    bool hasEnumerator(const QString &enumeratorName) const;

    QStringList itemLibraryItems() const;

public:
    static MetaInfo global();
    static void clearGlobal();

    static void setPluginPaths(const QStringList &paths);

private:
    QStringList superClasses(const QString &className) const;
    QStringList directSuperClasses(const QString &className) const;
    QList<NodeMetaInfo> superClasses(const NodeMetaInfo &nodeMetaInfo) const;
    QList<NodeMetaInfo> directSuperClasses(const NodeMetaInfo &nodeMetaInfo) const;

    void addSuperClassRelationship(const QString &superClassName, const QString &className);

    void addNodeInfo(NodeMetaInfo &info, const QString &baseType);

    bool isGlobal() const;

private:
    MetaInfo();

    Internal::MetaInfoPrivatePointer m_p;
    static MetaInfo s_global;
    static QStringList s_pluginDirs;
};

} //namespace QKineticDesigner

#endif // METAINFO_H

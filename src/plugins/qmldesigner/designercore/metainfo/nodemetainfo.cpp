/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "nodemetainfo.h"
#include "model.h"
#include "invalidargumentexception.h"

#include "metainfo.h"
#include <model.h>
#include <rewriterview.h>
#include <propertyparser.h>

#include <QSharedData>
#include <QDebug>
#include <QIcon>

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/parser/qmljsast_p.h>
#include <languageutils/fakemetaobject.h>

namespace QmlDesigner {

namespace Internal {

struct TypeDescription
{
    QString className;
    int minorVersion;
    int majorVersion;
};

} //Internal

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

namespace Internal {

using namespace QmlJS;

typedef QPair<QString, QString> PropertyInfo;

QList<PropertyInfo> getObjectTypes(const ObjectValue *ov, const ContextPtr &context, bool local = false);

static QString resolveTypeName(const ASTPropertyReference *ref, const ContextPtr &context,  QList<PropertyInfo> &dotProperties)
{
    QString type = QLatin1String("unknown");

    if (!ref->ast()->memberType.isEmpty()) {
        type = ref->ast()->memberType.toString();

        if (type == QLatin1String("alias")) {
            const Value *value = context->lookupReference(ref);

            if (!value)
                return type;

            if (const ASTObjectValue * astObjectValue = value->asAstObjectValue()) {
                if (astObjectValue->typeName())
                    type = astObjectValue->typeName()->name.toString();
            } else if (const ObjectValue * objectValue = value->asObjectValue()) {
                type = objectValue->className();
                dotProperties = getObjectTypes(objectValue, context);
            } else if (value->asColorValue()) {
                type = QLatin1String("color");
            } else if (value->asUrlValue()) {
                type = QLatin1String("url");
            } else if (value->asStringValue()) {
                type = QLatin1String("string");
            } else if (value->asRealValue()) {
                type = QLatin1String("real");
            } else if (value->asIntValue()) {
                type = QLatin1String("int");
            } else if (value->asBooleanValue()) {
                type = QLatin1String("boolean");
            }
        }
    }

    return type;
}

class PropertyMemberProcessor : public MemberProcessor
{
public:
    PropertyMemberProcessor(const ContextPtr &context) : m_context(context)
    {}
    virtual bool processProperty(const QString &name, const Value *value)
    {
        const ASTPropertyReference *ref = value_cast<ASTPropertyReference>(value);
        if (ref) {
            QList<PropertyInfo> dotProperties;
            const QString type = resolveTypeName(ref, m_context, dotProperties);
            m_properties.append(qMakePair(name, type));
            if (!dotProperties.isEmpty()) {
                foreach (const PropertyInfo &propertyInfo, dotProperties) {
                    QString dotName = propertyInfo.first;
                    QString type = propertyInfo.second;
                    dotName = name + '.' + dotName;
                    m_properties.append(qMakePair(dotName, type));
                }
            }
        } else {
            if (const CppComponentValue * ov = value_cast<CppComponentValue>(value)) {
                QString qualifiedTypeName = ov->moduleName().isEmpty() ? ov->className() : ov->moduleName() + '.' + ov->className();
                m_properties.append(qMakePair(name, qualifiedTypeName));
            } else {
                TypeId typeId;
                QString typeName = typeId(value);
                if (typeName == QLatin1String("number")) {
                    if (value->asRealValue())
                        typeName = "real";
                    else
                        typeName = "int";
                }
                m_properties.append(qMakePair(name, typeName));
            }
        }
        return true;
    }

    QList<PropertyInfo> properties() const { return m_properties; }

private:
    QList<PropertyInfo> m_properties;
    const ContextPtr m_context;
};

static inline bool isValueType(const QString &type)
{
    QStringList objectValuesList;
    objectValuesList << "QFont" << "QPoint" << "QPointF" << "QSize" << "QSizeF" << "QVector3D" << "QVector2D";
    return objectValuesList.contains(type);
}

const CppComponentValue *findQmlPrototype(const ObjectValue *ov, const ContextPtr &context)
{
    if (!ov)
        return 0;

    const CppComponentValue * qmlValue = value_cast<CppComponentValue>(ov);
    if (qmlValue)
        return qmlValue;

    return findQmlPrototype(ov->prototype(context), context);
}

QStringList prototypes(const ObjectValue *ov, const ContextPtr &context, bool versions = false)
{
    QStringList list;
    if (!ov)
        return list;
    ov = ov->prototype(context);
    while (ov) {
        const CppComponentValue * qmlValue = value_cast<CppComponentValue>(ov);
        if (qmlValue) {
            if (versions) {
                list << qmlValue->moduleName() + '.' + qmlValue->className() +
                ' ' + QString::number(qmlValue->componentVersion().majorVersion()) +
                '.' + QString::number(qmlValue->componentVersion().minorVersion());
            } else {
                list << qmlValue->moduleName() + QLatin1Char('.') + qmlValue->className();
            }
        } else {
            if (versions)
                list << ov->className() + " -1.-1";
            else
                list << ov->className();
        }
        ov = ov->prototype(context);
    }
    return list;
}

QList<PropertyInfo> getQmlTypes(const CppComponentValue *ov, const ContextPtr &context, bool local = false)
{
    QList<PropertyInfo> list;
    if (!ov)
        return list;
    if (ov->className().isEmpty())
        return list;

    PropertyMemberProcessor processor(context);
    ov->processMembers(&processor);

    QList<PropertyInfo> newList = processor.properties();

    foreach (PropertyInfo property, newList) {
        QString name = property.first;
        if (!ov->isWritable(name) && ov->isPointer(name)) {
            //dot property
            const CppComponentValue * qmlValue = value_cast<CppComponentValue>(ov->lookupMember(name, context));
            if (qmlValue) {
                QList<PropertyInfo> dotProperties = getQmlTypes(qmlValue, context);
                foreach (const PropertyInfo &propertyInfo, dotProperties) {
                    QString dotName = propertyInfo.first;
                    QString type = propertyInfo.second;
                    dotName = name + '.' + dotName;
                    list.append(qMakePair(dotName, type));
                }
            }
        }
        if (isValueType(ov->propertyType(name))) {
            const ObjectValue *dotObjectValue = value_cast<ObjectValue>(ov->lookupMember(name, context));
            if (dotObjectValue) {
                QList<PropertyInfo> dotProperties = getObjectTypes(dotObjectValue, context);
                foreach (const PropertyInfo &propertyInfo, dotProperties) {
                    QString dotName = propertyInfo.first;
                    QString type = propertyInfo.second;
                    dotName = name + '.' + dotName;
                    list.append(qMakePair(dotName, type));
                }
            }
        }
        QString type = property.second;
        if (!ov->isPointer(name) && !ov->isListProperty(name))
            type = ov->propertyType(name);
        list.append(qMakePair(name, type));
    }

    if (!local) {
        const ObjectValue* prototype = ov->prototype(context);

        const CppComponentValue * qmlObjectValue = value_cast<CppComponentValue>(prototype);

        if (qmlObjectValue)
            list << getQmlTypes(qmlObjectValue, context);
        else
            list << getObjectTypes(prototype, context);
    }

    return list;
}

QList<PropertyInfo> getTypes(const ObjectValue *ov, const ContextPtr &context, bool local = false)
{
    QList<PropertyInfo> list;

    const CppComponentValue * qmlObjectValue = value_cast<CppComponentValue>(ov);

    if (qmlObjectValue)
        list << getQmlTypes(qmlObjectValue, context, local);
    else
        list << getObjectTypes(ov, context, local);

    return list;
}

QList<PropertyInfo> getObjectTypes(const ObjectValue *ov, const ContextPtr &context, bool local)
{
    QList<PropertyInfo> list;
    if (!ov)
        return list;
    if (ov->className().isEmpty())
        return list;

    PropertyMemberProcessor processor(context);
    ov->processMembers(&processor);

    list << processor.properties();

    if (!local) {
        const ObjectValue* prototype = ov->prototype(context);

        if (prototype == ov)
            return list;

        const CppComponentValue * qmlObjectValue = value_cast<CppComponentValue>(prototype);

        if (qmlObjectValue)
            list << getQmlTypes(qmlObjectValue, context);
        else
            list << getObjectTypes(prototype, context);
    }

    return list;
}

class NodeMetaInfoPrivate
{
public:
    typedef QSharedPointer<NodeMetaInfoPrivate> Pointer;
    NodeMetaInfoPrivate();
    ~NodeMetaInfoPrivate() {}

    bool isValid() const;
    bool isFileComponent() const;
    QStringList properties() const;
    QStringList localProperties() const;
    QString defaultPropertyName() const;
    QString propertyType(const QString &propertyName) const;

    void setupPrototypes();
    QList<TypeDescription> prototypes() const;

    bool isPropertyWritable(const QString &propertyName) const;
    bool isPropertyPointer(const QString &propertyName) const;
    bool isPropertyList(const QString &propertyName) const;
    bool isPropertyEnum(const QString &propertyName) const;
    QString propertyEnumScope(const QString &propertyName) const;
    QStringList keysForEnum(const QString &enumName) const;
    bool cleverCheckType(const QString &otherType) const;
    QVariant::Type variantTypeId(const QString &properyName) const;

    int majorVersion() const;
    int minorVersion() const;
    QString qualfiedTypeName() const;
    Model *model() const;

    QString cppPackageName() const;

    QString componentSource() const;
    QString componentFileName() const;
    QString importDirectoryPath() const;

    static Pointer create(Model *model, const QString &type, int maj = -1, int min = -1);

    QSet<QString> &prototypeCachePositives();
    QSet<QString> &prototypeCacheNegatives();

    static void clearCache();


private:
    NodeMetaInfoPrivate(Model *model, QString type, int maj = -1, int min = -1);

    const QmlJS::CppComponentValue *getCppComponentValue() const;
    const QmlJS::ObjectValue *getObjectValue() const;
    void setupPropertyInfo(QList<PropertyInfo> propertyInfos);
    void setupLocalPropertyInfo(QList<PropertyInfo> propertyInfos);
    QString lookupName() const;
    QStringList lookupNameComponent() const;
    const QmlJS::CppComponentValue *getNearestCppComponentValue() const;
    QString fullQualifiedImportAliasType() const;

    QString m_qualfiedTypeName;
    int m_majorVersion;
    int m_minorVersion;
    bool m_isValid;
    bool m_isFileComponent;
    QStringList m_properties;
    QStringList m_propertyTypes;
    QStringList m_localProperties;
    QString m_defaultPropertyName;
    QList<TypeDescription> m_prototypes;
    QSet<QString> m_prototypeCachePositives;
    QSet<QString> m_prototypeCacheNegatives;

    //storing the pointer would not be save
    QmlJS::ContextPtr context() const;
    const Document *document() const;

    QPointer<Model> m_model;
    static QHash<QString, Pointer> m_nodeMetaInfoCache;
};

QHash<QString, NodeMetaInfoPrivate::Pointer> NodeMetaInfoPrivate::m_nodeMetaInfoCache;

bool NodeMetaInfoPrivate::isFileComponent() const
{
    return m_isFileComponent;
}

QStringList NodeMetaInfoPrivate::properties() const
{
    return m_properties;
}

QStringList NodeMetaInfoPrivate::localProperties() const
{
    return m_localProperties;
}

QSet<QString> &NodeMetaInfoPrivate::prototypeCachePositives()
{
    return m_prototypeCachePositives;
}

QSet<QString> &NodeMetaInfoPrivate::prototypeCacheNegatives()
{
    return m_prototypeCacheNegatives;
}

void NodeMetaInfoPrivate::clearCache()
{
    m_nodeMetaInfoCache.clear();
}

QString NodeMetaInfoPrivate::defaultPropertyName() const
{
    if (!m_defaultPropertyName.isEmpty())
        return m_defaultPropertyName;
    return QLatin1String("data");
}

static inline QString stringIdentifier( const QString &type, int maj, int min)
{
    return type + QString::number(maj) + '_' + QString::number(min);
}

NodeMetaInfoPrivate::Pointer NodeMetaInfoPrivate::create(Model *model, const QString &type, int maj, int min)
{
    if (m_nodeMetaInfoCache.contains(stringIdentifier(type, maj, min))) {
        const Pointer &info = m_nodeMetaInfoCache.value(stringIdentifier(type, maj, min));
        if (info->model() == model)
            return info;
        else
            m_nodeMetaInfoCache.clear();
    }

    Pointer newData(new NodeMetaInfoPrivate(model, type, maj, min));
    if (newData->isValid())
        m_nodeMetaInfoCache.insert(stringIdentifier(type, maj, min), newData);
    return newData;
}

NodeMetaInfoPrivate::NodeMetaInfoPrivate() : m_isValid(false)
{

}

NodeMetaInfoPrivate::NodeMetaInfoPrivate(Model *model, QString type, int maj, int min) :
                                        m_qualfiedTypeName(type), m_majorVersion(maj),
                                        m_minorVersion(min), m_isValid(false), m_isFileComponent(false),
                                        m_model(model)
{
    if (context()) {
        const CppComponentValue *objectValue = getCppComponentValue();
        if (objectValue) {
            if (m_majorVersion == -1 && m_minorVersion == -1) {
                m_majorVersion = objectValue->componentVersion().majorVersion();
                m_minorVersion = objectValue->componentVersion().minorVersion();
            }
            setupPropertyInfo(getTypes(objectValue, context()));
            setupLocalPropertyInfo(getTypes(objectValue, context(), true));
            m_defaultPropertyName = objectValue->defaultPropertyName();
            m_isValid = true;
            setupPrototypes();
        } else {
            const ObjectValue *objectValue = getObjectValue();
            if (objectValue) {
                const CppComponentValue *qmlValue = value_cast<CppComponentValue>(objectValue);
                if (qmlValue) {
                    if (m_majorVersion == -1 && m_minorVersion == -1) {
                        m_majorVersion = qmlValue->componentVersion().majorVersion();
                        m_minorVersion = qmlValue->componentVersion().minorVersion();
                        m_qualfiedTypeName = qmlValue->moduleName() + '.' + qmlValue->className();
                    } else if (m_majorVersion == qmlValue->componentVersion().majorVersion() && m_minorVersion == qmlValue->componentVersion().minorVersion()) {
                        m_qualfiedTypeName = qmlValue->moduleName() + '.' + qmlValue->className();
                    } else {
                        return;
                    }
                } else {
                    m_isFileComponent = true;
                    const Imports *imports = context()->imports(document());
                    ImportInfo importInfo = imports->info(lookupNameComponent().last(), context().data());
                    if (importInfo.isValid() && importInfo.type() == ImportInfo::LibraryImport) {
                        m_majorVersion = importInfo.version().majorVersion();
                        m_minorVersion = importInfo.version().minorVersion();
                    }
                }
                setupPropertyInfo(getTypes(objectValue, context()));
                setupLocalPropertyInfo(getTypes(objectValue, context(), true));
                m_defaultPropertyName = context()->defaultPropertyName(objectValue);
                m_isValid = true;
                setupPrototypes();
            }
        }
    }
}

const QmlJS::CppComponentValue *NodeMetaInfoPrivate::getCppComponentValue() const
{
    const QStringList nameComponents = m_qualfiedTypeName.split('.');
    if (nameComponents.size() < 2)
        return 0;
    const QString type = nameComponents.last();

    // maybe 'type' is a cpp name
    const QmlJS::CppComponentValue *value = context()->valueOwner()->cppQmlTypes().objectByCppName(type);
    if (value)
        return value;

    QString module;
    for (int i = 0; i < nameComponents.size() - 1; ++i) {
        if (i != 0)
            module += QLatin1Char('/');
        module += nameComponents.at(i);
    }

    // otherwise get the qml object value that's available in the document
    foreach (const QmlJS::Import &import, context()->imports(document())->all()) {
        if (import.info.path() != module)
            continue;
        const Value *lookupResult = import.object->lookupMember(type, context());
        const CppComponentValue *cppValue = value_cast<CppComponentValue>(lookupResult);
        if (cppValue
                && (m_majorVersion == -1 || m_majorVersion == cppValue->componentVersion().majorVersion())
                && (m_minorVersion == -1 || m_minorVersion == cppValue->componentVersion().minorVersion())
                )
            return cppValue;
    }

    return 0;
}

const QmlJS::ObjectValue *NodeMetaInfoPrivate::getObjectValue() const
{
    return context()->lookupType(document(), lookupNameComponent());
}

QmlJS::ContextPtr NodeMetaInfoPrivate::context() const
{
    if (m_model && m_model->rewriterView() && m_model->rewriterView()->scopeChain())
        return m_model->rewriterView()->scopeChain()->context();
    return QmlJS::ContextPtr(0);
}

const QmlJS::Document *NodeMetaInfoPrivate::document() const
{
    if (m_model && m_model->rewriterView())
        return m_model->rewriterView()->document();
    return 0;
}

void NodeMetaInfoPrivate::setupLocalPropertyInfo(QList<PropertyInfo> localPropertyInfos)
{
    foreach (const PropertyInfo &propertyInfo, localPropertyInfos) {
        m_localProperties.append(propertyInfo.first);
    }
}

void NodeMetaInfoPrivate::setupPropertyInfo(QList<PropertyInfo> propertyInfos)
{
    foreach (const PropertyInfo &propertyInfo, propertyInfos) {
        m_properties.append(propertyInfo.first);
        m_propertyTypes.append(propertyInfo.second);
    }
}

bool NodeMetaInfoPrivate::isPropertyWritable(const QString &propertyName) const
{
    if (!isValid())
        return false;

    if (propertyName.contains('.')) {
        const QStringList parts = propertyName.split('.');
        const QString objectName = parts.first();
        const QString rawPropertyName = parts.last();
        const QString objectType = propertyType(objectName);

        if (isValueType(objectType))
            return true;

        QSharedPointer<NodeMetaInfoPrivate> objectInfo(create(m_model, objectType));
        if (objectInfo->isValid())
            return objectInfo->isPropertyWritable(rawPropertyName);
        else
            return true;
    }

    const QmlJS::CppComponentValue *qmlObjectValue = getNearestCppComponentValue();
    if (!qmlObjectValue)
        return true;
    if (qmlObjectValue->hasProperty(propertyName))
        return qmlObjectValue->isWritable(propertyName);
    else
        return true; //all properties of components are writable
}


bool NodeMetaInfoPrivate::isPropertyList(const QString &propertyName) const
{
    if (!isValid())
        return false;

    if (propertyName.contains('.')) {
        const QStringList parts = propertyName.split('.');
        const QString objectName = parts.first();
        const QString rawPropertyName = parts.last();
        const QString objectType = propertyType(objectName);

        if (isValueType(objectType))
            return false;

        QSharedPointer<NodeMetaInfoPrivate> objectInfo(create(m_model, objectType));
        if (objectInfo->isValid())
            return objectInfo->isPropertyList(rawPropertyName);
        else
            return true;
    }

    const QmlJS::CppComponentValue *qmlObjectValue = getNearestCppComponentValue();
    if (!qmlObjectValue)
        return false;
    return qmlObjectValue->isListProperty(propertyName);
}

bool NodeMetaInfoPrivate::isPropertyPointer(const QString &propertyName) const
{
    if (!isValid())
        return false;

    if (propertyName.contains('.')) {
        const QStringList parts = propertyName.split('.');
        const QString objectName = parts.first();
        const QString rawPropertyName = parts.last();
        const QString objectType = propertyType(objectName);

        if (isValueType(objectType))
            return false;

        QSharedPointer<NodeMetaInfoPrivate> objectInfo(create(m_model, objectType));
        if (objectInfo->isValid())
            return objectInfo->isPropertyPointer(rawPropertyName);
        else
            return true;
    }

    const QmlJS::CppComponentValue *qmlObjectValue = getNearestCppComponentValue();
    if (!qmlObjectValue)
        return false;
    return qmlObjectValue->isPointer(propertyName);
}

bool NodeMetaInfoPrivate::isPropertyEnum(const QString &propertyName) const
{
    if (!isValid())
        return false;

    if (propertyName.contains('.')) {
        const QStringList parts = propertyName.split('.');
        const QString objectName = parts.first();
        const QString rawPropertyName = parts.last();
        const QString objectType = propertyType(objectName);

        if (isValueType(objectType))
            return false;

        QSharedPointer<NodeMetaInfoPrivate> objectInfo(create(m_model, objectType));
        if (objectInfo->isValid())
            return objectInfo->isPropertyEnum(rawPropertyName);
        else
            return false;
    }

    const CppComponentValue *qmlObjectValue = getNearestCppComponentValue();
    if (!qmlObjectValue)
        return false;
    return qmlObjectValue->getEnum(propertyType(propertyName)).isValid();
}

QString NodeMetaInfoPrivate::propertyEnumScope(const QString &propertyName) const
{
    if (!isValid())
        return QString();

    if (propertyName.contains('.')) {
        const QStringList parts = propertyName.split('.');
        const QString objectName = parts.first();
        const QString rawPropertyName = parts.last();
        const QString objectType = propertyType(objectName);

        if (isValueType(objectType))
            return QString();

        QSharedPointer<NodeMetaInfoPrivate> objectInfo(create(m_model, objectType));
        if (objectInfo->isValid())
            return objectInfo->propertyEnumScope(rawPropertyName);
        else
            return QString();
    }

    const CppComponentValue *qmlObjectValue = getNearestCppComponentValue();
    if (!qmlObjectValue)
        return QString();
    const CppComponentValue *definedIn = 0;
    qmlObjectValue->getEnum(propertyType(propertyName), &definedIn);
    if (definedIn)
        return definedIn->className();

    return QString();
}

static QString getUnqualifiedName(const QString &name)
{
    const QStringList nameComponents = name.split('.');
    if (nameComponents.size() < 2)
        return QString();
    return nameComponents.last();
}

bool NodeMetaInfoPrivate::cleverCheckType(const QString &otherType) const
{
    if (otherType == qualfiedTypeName())
            return true;

    if (isFileComponent())
        return false;

    QStringList split = otherType.split('.');
    QString package;
    QString typeName = otherType;
    if (split.count() > 1) {
        package = split.first();
        typeName = split.at(1);
    }
    if (cppPackageName() == package)
        return QString(package + '.' + typeName) == cppPackageName() + '.' + getUnqualifiedName(qualfiedTypeName());

    const CppComponentValue *qmlObjectValue = getCppComponentValue();
    if (!qmlObjectValue)
        return false;

    const LanguageUtils::FakeMetaObject::Export exp =
            qmlObjectValue->metaObject()->exportInPackage(package);
    QString convertedName = exp.type;
    if (convertedName.isEmpty())
        convertedName = qmlObjectValue->className();

    return typeName == convertedName;
}

QVariant::Type NodeMetaInfoPrivate::variantTypeId(const QString &properyName) const
{
    QString typeName = propertyType(properyName);
    if (typeName == "string")
        return QVariant::String;

    if (typeName == "color")
        return QVariant::Color;

    if (typeName == "int")
        return QVariant::Int;

    if (typeName == "url")
        return QVariant::Url;

    if (typeName == "real")
        return QVariant::Double;

    if (typeName == "bool")
        return QVariant::Bool;

    if (typeName == "boolean")
        return QVariant::Bool;

    if (typeName == "date")
        return QVariant::Date;

    if (typeName == "alias")
        return QVariant::UserType;

    if (typeName == "var")
        return QVariant::UserType;

    return QVariant::nameToType(typeName.toLatin1().data());
}

int NodeMetaInfoPrivate::majorVersion() const
{
    return m_majorVersion;
}

int NodeMetaInfoPrivate::minorVersion() const
{
    return m_minorVersion;
}

QString NodeMetaInfoPrivate::qualfiedTypeName() const
{
    return m_qualfiedTypeName;
}

Model *NodeMetaInfoPrivate::model() const
{
    return m_model;
}


QStringList NodeMetaInfoPrivate::keysForEnum(const QString &enumName) const
{
    if (!isValid())
        return QStringList();

    const CppComponentValue *qmlObjectValue = getNearestCppComponentValue();
    if (!qmlObjectValue)
        return QStringList();
    return qmlObjectValue->getEnum(enumName).keys();
}

QString NodeMetaInfoPrivate::cppPackageName() const
{
    if (!isFileComponent()) {
        if (const CppComponentValue *qmlObject = getCppComponentValue())
            return qmlObject->moduleName();
    }
    return QString();
}

QString NodeMetaInfoPrivate::componentSource() const
{
    if (isFileComponent()) {
        const ASTObjectValue * astObjectValue = value_cast<ASTObjectValue>(getObjectValue());
        if (astObjectValue)
            return astObjectValue->document()->source().mid(astObjectValue->typeName()->identifierToken.begin(),
                                                            astObjectValue->initializer()->rbraceToken.end());
    }
    return QString();
}

QString NodeMetaInfoPrivate::componentFileName() const
{
    if (isFileComponent()) {
        const ASTObjectValue * astObjectValue = value_cast<ASTObjectValue>(getObjectValue());
        if (astObjectValue) {
            QString fileName;
            int line;
            int column;
            if (astObjectValue->getSourceLocation(&fileName, &line, &column))
                return fileName;
        }
    }
    return QString();
}

QString NodeMetaInfoPrivate::importDirectoryPath() const
{
    if (isValid()) {
        const Imports *imports = context()->imports(document());
        ImportInfo importInfo = imports->info(qualfiedTypeName(), context().data());

        return importInfo.path();
    }
    return QString();
}

QString NodeMetaInfoPrivate::lookupName() const
{
    QString className = m_qualfiedTypeName;
    QString packageName;

    QStringList packageClassName = m_qualfiedTypeName.split(QLatin1Char('.'));
    if (packageClassName.size() > 1) {
        className = packageClassName.takeLast();
        packageName = packageClassName.join(QLatin1String("."));
    }

    return CppQmlTypes::qualifiedName(
                packageName,
                className,
                LanguageUtils::ComponentVersion(m_majorVersion, m_minorVersion));
}

QStringList NodeMetaInfoPrivate::lookupNameComponent() const
{
        QString tempString = fullQualifiedImportAliasType();
        return tempString.split('.');
}


bool NodeMetaInfoPrivate::isValid() const
{
    return m_isValid && context() && document();
}

QString NodeMetaInfoPrivate::propertyType(const QString &propertyName) const
{
    if (!m_properties.contains(propertyName))
        return QLatin1String("Property does not exist...");
    return m_propertyTypes.at(m_properties.indexOf(propertyName));
}

void NodeMetaInfoPrivate::setupPrototypes()
{
    QList<const ObjectValue *> objects;

    const ObjectValue *ov;

    if (m_isFileComponent)
        ov = getObjectValue();
    else
        ov = getCppComponentValue();

    PrototypeIterator prototypeIterator(ov, context());

    objects = prototypeIterator.all();

    if (prototypeIterator.error() != PrototypeIterator::NoError) {
        m_isValid = false;
        return;
    }

    foreach (const ObjectValue *ov, objects) {
        TypeDescription description;
        description.className = ov->className();
        description.minorVersion = -1;
        description.majorVersion = -1;
        if (const CppComponentValue * qmlValue = value_cast<CppComponentValue>(ov)) {
            description.minorVersion = qmlValue->componentVersion().minorVersion();
            description.majorVersion = qmlValue->componentVersion().majorVersion();
            LanguageUtils::FakeMetaObject::Export qtquickExport = qmlValue->metaObject()->exportInPackage("QtQuick");
            LanguageUtils::FakeMetaObject::Export cppExport = qmlValue->metaObject()->exportInPackage("<cpp>");
            if (qtquickExport.isValid())
                description.className = qtquickExport.package + '.' + qtquickExport.type;
            else if (qmlValue->moduleName().isEmpty() && cppExport.isValid())
                description.className = cppExport.package + '.' + cppExport.type;
            else if (!qmlValue->moduleName().isEmpty())
                description.className = qmlValue->moduleName() + '.' + description.className;
            m_prototypes.append(description);
        } else {
            if (context()->lookupType(document(), QStringList() << ov->className()))
                m_prototypes.append(description);
        }
    }
}


QList<TypeDescription> NodeMetaInfoPrivate::prototypes() const
{
    return m_prototypes;
}

const QmlJS::CppComponentValue *NodeMetaInfoPrivate::getNearestCppComponentValue() const
{
    if (m_isFileComponent)
        return findQmlPrototype(getObjectValue(), context());
    return getCppComponentValue();
}

QString NodeMetaInfoPrivate::fullQualifiedImportAliasType() const
{
    if (m_model && m_model->rewriterView())
        return model()->rewriterView()->convertTypeToImportAlias(m_qualfiedTypeName);
    return m_qualfiedTypeName;
}

} //namespace Internal

NodeMetaInfo::NodeMetaInfo() : m_privateData(new Internal::NodeMetaInfoPrivate())
{

}

NodeMetaInfo::NodeMetaInfo(Model *model, QString type, int maj, int min) : m_privateData(Internal::NodeMetaInfoPrivate::create(model, type, maj, min))
{

}

NodeMetaInfo::~NodeMetaInfo()
{
}

NodeMetaInfo::NodeMetaInfo(const NodeMetaInfo &other)
    : m_privateData(other.m_privateData)
{
}

NodeMetaInfo &NodeMetaInfo::operator=(const NodeMetaInfo &other)
{
    if (this != &other)
        this->m_privateData = other.m_privateData;

    return *this;
}

bool NodeMetaInfo::isValid() const
{
    return m_privateData->isValid();
}

bool NodeMetaInfo::isFileComponent() const
{
    return m_privateData->isFileComponent();
}

bool NodeMetaInfo::hasProperty(const QString &propertyName) const
{
    return propertyNames().contains(propertyName);
}

QStringList NodeMetaInfo::propertyNames() const
{
    return m_privateData->properties();
}

QStringList NodeMetaInfo::directPropertyNames() const
{
    return m_privateData->localProperties();
}

QString NodeMetaInfo::defaultPropertyName() const
{
    return m_privateData->defaultPropertyName();
}

bool NodeMetaInfo::hasDefaultProperty() const
{
    return !defaultPropertyName().isEmpty();
}

QString NodeMetaInfo::propertyTypeName(const QString &propertyName) const
{
    return m_privateData->propertyType(propertyName);
}

bool NodeMetaInfo::propertyIsWritable(const QString &propertyName) const
{
    return m_privateData->isPropertyWritable(propertyName);
}

bool NodeMetaInfo::propertyIsListProperty(const QString &propertyName) const
{
    return m_privateData->isPropertyList(propertyName);
}

bool NodeMetaInfo::propertyIsEnumType(const QString &propertyName) const
{
    return m_privateData->isPropertyEnum(propertyName);
}

QString NodeMetaInfo::propertyEnumScope(const QString &propertyName) const
{
    return m_privateData->propertyEnumScope(propertyName);
}

QStringList NodeMetaInfo::propertyKeysForEnum(const QString &propertyName) const
{
    return m_privateData->keysForEnum(propertyTypeName(propertyName));
}

QVariant NodeMetaInfo::propertyCastedValue(const QString &propertyName, const QVariant &value) const
{

    QVariant variant = value;
    if (propertyIsEnumType(propertyName))
        return variant;

    const QString typeName = propertyTypeName(propertyName);

    QVariant::Type typeId = m_privateData->variantTypeId(propertyName);

    if (variant.type() == QVariant::UserType && variant.userType() == ModelNode::variantUserType()) {
        return variant;
    } else if (typeId == QVariant::UserType && typeName == QLatin1String("QVariant")) {
        return variant;
    } else if (typeId == QVariant::UserType && typeName == QLatin1String("variant")) {
        return variant;
    } else if (typeId == QVariant::UserType && typeName == QLatin1String("var")) {
        return variant;
    } else if (variant.type() == QVariant::List && variant.type() == QVariant::List) {
        // TODO: check the contents of the list
        return variant;
    } else if (typeName == "var" || typeName == "variant") {
        return variant;
    } else if (typeName == "alias") {
        // TODO: The QML compiler resolves the alias type. We probably should do the same.
        return variant;
    } else if (variant.convert(typeId)) {
        return variant;
    }

    return Internal::PropertyParser::variantFromString(variant.toString());
}

QList<NodeMetaInfo> NodeMetaInfo::superClasses() const
{
    QList<NodeMetaInfo> list;

    foreach (const Internal::TypeDescription &type,  m_privateData->prototypes()) {
        list.append(NodeMetaInfo(m_privateData->model(), type.className, type.majorVersion, type.minorVersion));
    }
    return list;
}

NodeMetaInfo NodeMetaInfo::directSuperClass() const
{
    QList<NodeMetaInfo> superClassesList = superClasses();
    if (superClassesList.count() > 1)
        return superClassesList.at(1);
    return NodeMetaInfo();
}

QString NodeMetaInfo::typeName() const
{
    return m_privateData->qualfiedTypeName();
}
int NodeMetaInfo::majorVersion() const
{
    return m_privateData->majorVersion();
}
int NodeMetaInfo::minorVersion() const
{
    return m_privateData->minorVersion();
}

QString NodeMetaInfo::componentSource() const
{
    return m_privateData->componentSource();
}

QString NodeMetaInfo::componentFileName() const
{
    return m_privateData->componentFileName();
}

QString NodeMetaInfo::importDirectoryPath() const
{
    return m_privateData->importDirectoryPath();
}

bool NodeMetaInfo::hasCustomParser() const
{
    return false;
}

bool NodeMetaInfo::availableInVersion(int majorVersion, int minorVersion) const
{
    if (majorVersion == -1 && minorVersion == -1)
        return true;

    return (m_privateData->majorVersion() >= majorVersion)
        || (majorVersion == m_privateData->majorVersion() && m_privateData->minorVersion() >= minorVersion);
}

bool NodeMetaInfo::isSubclassOf(const QString &type, int majorVersion, int minorVersion) const
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid";
        return false;
    }

    if (typeName().isEmpty())
        return false;

    if (typeName() == type
        && availableInVersion(majorVersion, minorVersion))
        return true;

    if (m_privateData->prototypeCachePositives().contains(Internal::stringIdentifier(type, majorVersion, minorVersion)))
        return true; //take a shortcut - optimization

    if (m_privateData->prototypeCacheNegatives().contains(Internal::stringIdentifier(type, majorVersion, minorVersion)))
        return false; //take a shortcut - optimization

    foreach (const NodeMetaInfo &superClass, superClasses()) {
        if (superClass.m_privateData->cleverCheckType(type)
            && superClass.availableInVersion(majorVersion, minorVersion)) {
                m_privateData->prototypeCachePositives().insert(Internal::stringIdentifier(type, majorVersion, minorVersion));
            return true;
        }
    }
    m_privateData->prototypeCacheNegatives().insert(Internal::stringIdentifier(type, majorVersion, minorVersion));
    return false;
}

void NodeMetaInfo::clearCache()
{
    Internal::NodeMetaInfoPrivate::clearCache();
}

bool NodeMetaInfo::isPositioner() const
{
    if (majorVersion() < 2)
        return isSubclassOf("<cpp>.QDeclarativeBasePositioner", -1, -1);
    return isSubclassOf("QtQuick.Positioner", -1, -1);
}

} // namespace QmlDesigner

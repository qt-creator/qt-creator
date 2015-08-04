/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "nodemetainfo.h"
#include "model.h"

#include "metainfo.h"
#include <enumeration.h>
#include <rewriterview.h>
#include <propertyparser.h>

#include <QDir>
#include <QDebug>

#include <qmljs/qmljsscopechain.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsvalueowner.h>
#include <languageutils/fakemetaobject.h>

namespace QmlDesigner {

namespace Internal {

struct TypeDescription
{
    TypeName className;
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
a sub component qml file is deleted). Trying to call any accessor functions on an invalid
NodeMetaInfo object will result in an InvalidMetaInfoException being thrown.

\see QmlDesigner::MetaInfo, QmlDesigner::PropertyMetaInfo, QmlDesigner::EnumeratorMetaInfo
*/

namespace Internal {

using namespace QmlJS;

typedef QPair<PropertyName, TypeName> PropertyInfo;

QList<PropertyInfo> getObjectTypes(const ObjectValue *ov, const ContextPtr &context, bool local = false, int rec = 0);

static TypeName resolveTypeName(const ASTPropertyReference *ref, const ContextPtr &context,  QList<PropertyInfo> &dotProperties)
{
    TypeName type = "unknown";

    if (!ref->ast()->memberType.isEmpty()) {
        type = ref->ast()->memberType.toUtf8();

        if (type == "alias") {
            const Value *value = context->lookupReference(ref);

            if (!value)
                return type;

            if (const ASTObjectValue * astObjectValue = value->asAstObjectValue()) {
                if (astObjectValue->typeName()) {
                    type = astObjectValue->typeName()->name.toUtf8();
                    const ObjectValue *  objectValue =  context->lookupType(astObjectValue->document(), astObjectValue->typeName());;
                    if (objectValue)
                        dotProperties = getObjectTypes(objectValue, context);
                }
            } else if (const ObjectValue * objectValue = value->asObjectValue()) {
                type = objectValue->className().toUtf8();
                dotProperties = getObjectTypes(objectValue, context);
            } else if (value->asColorValue()) {
                type = "color";
            } else if (value->asUrlValue()) {
                type = "url";
            } else if (value->asStringValue()) {
                type = "string";
            } else if (value->asRealValue()) {
                type = "real";
            } else if (value->asIntValue()) {
                type = "int";
            } else if (value->asBooleanValue()) {
                type = "boolean";
            }
        }
    }

    return type;
}

static QString qualifiedTypeNameForContext(const ObjectValue *objectValue,
                                           const ViewerContext &vContext,
                                           const ImportDependencies &dep)
{
    QString cppName;
    QStringList packages;
    if (const CppComponentValue *cppComponent = value_cast<CppComponentValue>(objectValue)) {
        QString className = cppComponent->className();
        foreach (const LanguageUtils::FakeMetaObject::Export &e, cppComponent->metaObject()->exports()) {
            if (e.type == className)
                packages << e.package;
            if (e.package == CppQmlTypes::cppPackage)
                cppName = e.type;
        }
        if (packages.size() == 1 && packages.at(0) == CppQmlTypes::cppPackage)
            return packages.at(0) + QLatin1Char('.') + className;
    }
    // try to recover a "global context name"
    QStringList possibleLibraries;
    QStringList possibleQrcFiles;
    QStringList possibleFiles;
    bool hasQtQuick = false;
    do {
        if (objectValue->originId().isEmpty())
            break;
        CoreImport cImport = dep.coreImport(objectValue->originId());
        if (!cImport.valid())
            break;
        foreach (const Export &e, cImport.possibleExports) {
            if (e.pathRequired.isEmpty() || vContext.paths.contains(e.pathRequired)) {
                switch (e.exportName.type) {
                case ImportType::Library:
                {
                    QString typeName = objectValue->className();
                    if (!e.typeName.isEmpty() && e.typeName != Export::LibraryTypeName) {
                        typeName = e.typeName;
                        if (typeName != objectValue->className())
                            qCWarning(qmljsLog) << "Outdated classname " << objectValue->className()
                                                << " vs " << typeName
                                                << " for " << e.exportName.toString();
                    }
                    if (packages.isEmpty() || packages.contains(e.exportName.libraryQualifiedPath())) {
                        if (e.exportName.splitPath.value(0) == QLatin1String("QtQuick"))
                            hasQtQuick = true;
                        possibleLibraries.append(e.exportName.libraryQualifiedPath() + '.' + typeName);
                    }
                    break;
                }
                case ImportType::File:
                {
                    // remove the search path prefix.
                    // this means that the same relative path wrt. different import paths will clash
                    QString filePath = e.exportName.path();
                    foreach (const QString &path, vContext.paths) {
                        if (filePath.startsWith(path) && filePath.size() > path.size()
                                && filePath.at(path.size()) == QLatin1Char('/'))
                        {
                            filePath = filePath.mid(path.size() + 1);
                            break;
                        }
                    }

                    if (filePath.startsWith(QLatin1Char('/')))
                        filePath = filePath.mid(1);
                    QFileInfo fileInfo(filePath);
                    QStringList splitName = fileInfo.path().split(QLatin1Char('/'));
                    QString typeName = fileInfo.baseName();
                    if (!e.typeName.isEmpty()) {
                        if (e.typeName != fileInfo.baseName())
                            qCWarning(qmljsLog) << "type renaming in file import " << e.typeName
                                                << " for " << e.exportName.path();
                        typeName = e.typeName;
                    }
                    if (typeName != objectValue->className())
                        qCWarning(qmljsLog) << "Outdated classname " << objectValue->className()
                                            << " vs " << typeName
                                            << " for " << e.exportName.toString();
                    splitName.append(typeName);
                    possibleFiles.append(splitName.join(QLatin1Char('.')));
                    break;
                }
                case ImportType::QrcFile:
                {
                    QString filePath = e.exportName.path();
                    if (filePath.startsWith(QLatin1Char('/')))
                        filePath = filePath.mid(1);
                    QFileInfo fileInfo(filePath);
                    QStringList splitName = fileInfo.path().split(QLatin1Char('/'));
                    QString typeName = fileInfo.baseName();
                    if (!e.typeName.isEmpty()) {
                        if (e.typeName != fileInfo.baseName())
                            qCWarning(qmljsLog) << "type renaming in file import " << e.typeName
                                                << " for " << e.exportName.path();
                        typeName = e.typeName;
                    }
                    if (typeName != objectValue->className())
                        qCWarning(qmljsLog) << "Outdated classname " << objectValue->className()
                                            << " vs " << typeName
                                            << " for " << e.exportName.toString();
                    splitName.append(typeName);
                    possibleQrcFiles.append(splitName.join(QLatin1Char('.')));
                    break;
                }
                case ImportType::Invalid:
                case ImportType::UnknownFile:
                    break;
                case ImportType::Directory:
                case ImportType::ImplicitDirectory:
                case ImportType::QrcDirectory:
                    qCWarning(qmljsLog) << "unexpected import type in export "
                                        << e.exportName.toString() << " of coreExport "
                                        << objectValue->originId();
                    break;
                }
            }
        }
        auto optimalName = [] (const QStringList &list) -> QString {
            QString res = list.at(0);
            for (int i = 1; i < list.size(); ++i) {
                const QString &nameNow = list.at(i);
                if (nameNow.size() < res.size()
                        || (nameNow.size() == res.size() && nameNow < res))
                    res = nameNow;
            }
            return res;
        };
        if (!possibleLibraries.isEmpty()) {
            if (hasQtQuick) {
                foreach (const QString &libImport, possibleLibraries)
                    if (!libImport.startsWith(QLatin1String("QtQuick")))
                        possibleLibraries.removeAll(libImport);
            }
            return optimalName(possibleLibraries);
        }
        if (!possibleQrcFiles.isEmpty())
            return optimalName(possibleQrcFiles);
        if (!possibleFiles.isEmpty())
            return optimalName(possibleFiles);
    } while (false);
    if (!cppName.isEmpty())
        return CppQmlTypes::cppPackage + QLatin1Char('.') + cppName;
    if (const CppComponentValue *cppComponent = value_cast<CppComponentValue>(objectValue)) {
        if (cppComponent->moduleName().isEmpty())
            return cppComponent->className();
        else
            return cppComponent->moduleName() + QLatin1Char('.') + cppComponent->className();
    } else {
        return objectValue->className();
    }
}

class PropertyMemberProcessor : public MemberProcessor
{
public:
    PropertyMemberProcessor(const ContextPtr &context) : m_context(context)
    {}
    bool processProperty(const QString &name, const Value *value, const QmlJS::PropertyInfo &) override
    {
        PropertyName propertyName = name.toUtf8();
        const ASTPropertyReference *ref = value_cast<ASTPropertyReference>(value);
        if (ref) {
            QList<PropertyInfo> dotProperties;
            const TypeName type = resolveTypeName(ref, m_context, dotProperties);
            m_properties.append(qMakePair(propertyName, type));
            if (!dotProperties.isEmpty()) {
                foreach (const PropertyInfo &propertyInfo, dotProperties) {
                    PropertyName dotName = propertyInfo.first;
                    TypeName type = propertyInfo.second;
                    dotName = propertyName + '.' + dotName;
                    m_properties.append(qMakePair(dotName, type));
                }
            }
        } else {

            if (const CppComponentValue * cppComponentValue = value_cast<CppComponentValue>(value)) {
                TypeName qualifiedTypeName = qualifiedTypeNameForContext(cppComponentValue,
                    m_context->viewerContext(), *m_context->snapshot().importDependencies()).toUtf8();
                m_properties.append(qMakePair(propertyName, qualifiedTypeName));
            } else {
                TypeId typeId;
                TypeName typeName = typeId(value).toUtf8();
                if (typeName == "number") {
                    if (value->asIntValue()) {
                        typeName = "int";
                    } else {
                        typeName = "real";
                    }
                }
                m_properties.append(qMakePair(propertyName, typeName));
            }
        }
        return true;
    }

    virtual bool processSignal(const QString &name, const Value * /*value*/)
    {
        m_signals.append(name.toUtf8());
        return true;
    }

    QList<PropertyInfo> properties() const { return m_properties; }

    PropertyNameList signalList() const { return m_signals; }

private:
    QList<PropertyInfo> m_properties;
    PropertyNameList m_signals;
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

QList<PropertyInfo> getQmlTypes(const CppComponentValue *objectValue, const ContextPtr &context, bool local = false, int rec = 0)
{
    QList<PropertyInfo> propertyList;

    if (!objectValue)
        return propertyList;
    if (objectValue->className().isEmpty())
        return propertyList;

    if (rec > 2)
        return propertyList;

    PropertyMemberProcessor processor(context);
    objectValue->processMembers(&processor);

    QList<PropertyInfo> newList = processor.properties();

    foreach (PropertyInfo property, newList) {
        PropertyName name = property.first;
        if (!objectValue->isWritable(name) && objectValue->isPointer(name)) {
            //dot property
            const CppComponentValue * qmlValue = value_cast<CppComponentValue>(objectValue->lookupMember(name, context));
            if (qmlValue) {
                QList<PropertyInfo> dotProperties = getQmlTypes(qmlValue, context, false, rec + 1);
                foreach (const PropertyInfo &propertyInfo, dotProperties) {
                    PropertyName dotName = propertyInfo.first;
                    TypeName type = propertyInfo.second;
                    dotName = name + '.' + dotName;
                    propertyList.append(qMakePair(dotName, type));
                }
            }
        }
        if (isValueType(objectValue->propertyType(name))) {
            const ObjectValue *dotObjectValue = value_cast<ObjectValue>(objectValue->lookupMember(name, context));
            if (dotObjectValue) {
                QList<PropertyInfo> dotProperties = getObjectTypes(dotObjectValue, context, false, rec + 1);
                foreach (const PropertyInfo &propertyInfo, dotProperties) {
                    PropertyName dotName = propertyInfo.first;
                    TypeName type = propertyInfo.second;
                    dotName = name + '.' + dotName;
                    propertyList.append(qMakePair(dotName, type));
                }
            }
        }
        TypeName type = property.second;
        if (!objectValue->isPointer(name) && !objectValue->isListProperty(name))
            type = objectValue->propertyType(name).toUtf8();
        propertyList.append(qMakePair(name, type));
    }

    if (!local) {
        const ObjectValue* prototype = objectValue->prototype(context);

        const CppComponentValue * qmlObjectValue = value_cast<CppComponentValue>(prototype);

        if (qmlObjectValue)
            propertyList.append(getQmlTypes(qmlObjectValue, context, false, rec));
        else
            propertyList.append(getObjectTypes(prototype, context, false, rec));
    }

    return propertyList;
}

PropertyNameList getSignals(const ObjectValue *objectValue, const ContextPtr &context, bool local = false)
{
    PropertyNameList signalList;

     if (!objectValue)
        return signalList;
    if (objectValue->className().isEmpty())
        return signalList;

    PropertyMemberProcessor processor(context);
    objectValue->processMembers(&processor);

    signalList.append(processor.signalList());

    if (!local) {
        const ObjectValue* prototype = objectValue->prototype(context);

        if (prototype == objectValue)
            return signalList;

        signalList.append(getSignals(prototype, context));
    }

    return signalList;
}

QList<PropertyInfo> getTypes(const ObjectValue *objectValue, const ContextPtr &context, bool local = false)
{
    QList<PropertyInfo> propertyList;

    const CppComponentValue * qmlObjectValue = value_cast<CppComponentValue>(objectValue);

    if (qmlObjectValue)
        propertyList.append(getQmlTypes(qmlObjectValue, context, local));
    else
        propertyList.append(getObjectTypes(objectValue, context, local));

    return propertyList;
}

QList<PropertyInfo> getObjectTypes(const ObjectValue *objectValue, const ContextPtr &context, bool local, int rec)
{
    QList<PropertyInfo> propertyList;

    if (!objectValue)
        return propertyList;
    if (objectValue->className().isEmpty())
        return propertyList;

    if (rec > 2)
        return propertyList;

    PropertyMemberProcessor processor(context);
    objectValue->processMembers(&processor);

    propertyList.append(processor.properties());

    if (!local) {
        const ObjectValue* prototype = objectValue->prototype(context);

        if (prototype == objectValue)
            return propertyList;

        const CppComponentValue * qmlObjectValue = value_cast<CppComponentValue>(prototype);

        if (qmlObjectValue)
            propertyList.append(getQmlTypes(qmlObjectValue, context, local, rec));
        else
            propertyList.append(getObjectTypes(prototype, context, local, rec));
    }

    return propertyList;
}

class NodeMetaInfoPrivate
{
public:
    typedef QSharedPointer<NodeMetaInfoPrivate> Pointer;
    NodeMetaInfoPrivate();
    ~NodeMetaInfoPrivate() {}

    bool isValid() const;
    bool isFileComponent() const;
    PropertyNameList properties() const;
    PropertyNameList localProperties() const;
    PropertyNameList signalNames() const;
    PropertyName defaultPropertyName() const;
    TypeName propertyType(const PropertyName &propertyName) const;

    void setupPrototypes();
    QList<TypeDescription> prototypes() const;

    bool isPropertyWritable(const PropertyName &propertyName) const;
    bool isPropertyPointer(const PropertyName &propertyName) const;
    bool isPropertyList(const PropertyName &propertyName) const;
    bool isPropertyEnum(const PropertyName &propertyName) const;
    QString propertyEnumScope(const PropertyName &propertyName) const;
    QStringList keysForEnum(const QString &enumName) const;
    bool cleverCheckType(const QString &otherType) const;
    QVariant::Type variantTypeId(const PropertyName &properyName) const;

    int majorVersion() const;
    int minorVersion() const;
    TypeName qualfiedTypeName() const;
    Model *model() const;

    QString cppPackageName() const;

    QString componentSource() const;
    QString componentFileName() const;
    QString importDirectoryPath() const;

    static Pointer create(Model *model, const TypeName &type, int maj = -1, int min = -1);

    QSet<QString> &prototypeCachePositives();
    QSet<QString> &prototypeCacheNegatives();

    static void clearCache();


private:
    NodeMetaInfoPrivate(Model *model, TypeName type, int maj = -1, int min = -1);

    const CppComponentValue *getCppComponentValue() const;
    const ObjectValue *getObjectValue() const;
    void setupPropertyInfo(QList<PropertyInfo> propertyInfos);
    void setupLocalPropertyInfo(QList<PropertyInfo> propertyInfos);
    QString lookupName() const;
    QStringList lookupNameComponent() const;
    const CppComponentValue *getNearestCppComponentValue() const;
    QString fullQualifiedImportAliasType() const;

    TypeName m_qualfiedTypeName;
    int m_majorVersion;
    int m_minorVersion;
    bool m_isValid;
    bool m_isFileComponent;
    PropertyNameList m_properties;
    PropertyNameList m_signals;
    QList<TypeName> m_propertyTypes;
    PropertyNameList m_localProperties;
    PropertyName m_defaultPropertyName;
    QList<TypeDescription> m_prototypes;
    QSet<QString> m_prototypeCachePositives;
    QSet<QString> m_prototypeCacheNegatives;

    //storing the pointer would not be save
    ContextPtr context() const;
    const Document *document() const;

    QPointer<Model> m_model;
    static QHash<QString, Pointer> m_nodeMetaInfoCache;
};

QHash<QString, NodeMetaInfoPrivate::Pointer> NodeMetaInfoPrivate::m_nodeMetaInfoCache;

bool NodeMetaInfoPrivate::isFileComponent() const
{
    return m_isFileComponent;
}

PropertyNameList NodeMetaInfoPrivate::properties() const
{
    return m_properties;
}

PropertyNameList NodeMetaInfoPrivate::localProperties() const
{
    return m_localProperties;
}

PropertyNameList NodeMetaInfoPrivate::signalNames() const
{
    return m_signals;
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

PropertyName NodeMetaInfoPrivate::defaultPropertyName() const
{
    if (!m_defaultPropertyName.isEmpty())
        return m_defaultPropertyName;
    return PropertyName("data");
}

static inline QString stringIdentifier( const QString &type, int maj, int min)
{
    return type + QString::number(maj) + '_' + QString::number(min);
}

NodeMetaInfoPrivate::Pointer NodeMetaInfoPrivate::create(Model *model, const TypeName &type, int major, int minor)
{
    if (m_nodeMetaInfoCache.contains(stringIdentifier(type, major, minor))) {
        const Pointer &info = m_nodeMetaInfoCache.value(stringIdentifier(type, major, minor));
        if (info->model() == model)
            return info;
        else
            m_nodeMetaInfoCache.clear();
    }

    Pointer newData(new NodeMetaInfoPrivate(model, type, major, minor));
    if (newData->isValid())
        m_nodeMetaInfoCache.insert(stringIdentifier(type, major, minor), newData);
    return newData;
}

NodeMetaInfoPrivate::NodeMetaInfoPrivate() : m_isValid(false)
{

}

NodeMetaInfoPrivate::NodeMetaInfoPrivate(Model *model, TypeName type, int maj, int min) :
                                        m_qualfiedTypeName(type), m_majorVersion(maj),
                                        m_minorVersion(min), m_isValid(false), m_isFileComponent(false),
                                        m_model(model)
{
    if (context()) {
        const CppComponentValue *cppObjectValue = getCppComponentValue();

        if (cppObjectValue) {
            if (m_majorVersion == -1 && m_minorVersion == -1) {
                m_majorVersion = cppObjectValue->componentVersion().majorVersion();
                m_minorVersion = cppObjectValue->componentVersion().minorVersion();
            }
            setupPropertyInfo(getTypes(cppObjectValue, context()));
            setupLocalPropertyInfo(getTypes(cppObjectValue, context(), true));
            m_defaultPropertyName = cppObjectValue->defaultPropertyName().toUtf8();
            m_isValid = true;
            setupPrototypes();
            m_signals = getSignals(cppObjectValue, context());
        } else {
            const ObjectValue *objectValue = getObjectValue();
            if (objectValue) {
                const CppComponentValue *qmlValue = value_cast<CppComponentValue>(objectValue);
                if (qmlValue) {
                    if (m_majorVersion == -1 && m_minorVersion == -1) {
                        m_majorVersion = qmlValue->componentVersion().majorVersion();
                        m_minorVersion = qmlValue->componentVersion().minorVersion();
                        m_qualfiedTypeName = qmlValue->moduleName().toUtf8() + '.' + qmlValue->className().toUtf8();
                    } else if (m_majorVersion == qmlValue->componentVersion().majorVersion() && m_minorVersion == qmlValue->componentVersion().minorVersion()) {
                        m_qualfiedTypeName = qmlValue->moduleName().toUtf8() + '.' + qmlValue->className().toUtf8();
                    } else {
                        return;
                    }
                } else {
                    m_isFileComponent = true;
                    const Imports *imports = context()->imports(document());
                    ImportInfo importInfo = imports->info(lookupNameComponent().last(), context().data());
                    if (importInfo.isValid() && importInfo.type() == ImportType::Library) {
                        m_majorVersion = importInfo.version().majorVersion();
                        m_minorVersion = importInfo.version().minorVersion();
                    }
                }
                setupPropertyInfo(getTypes(objectValue, context()));
                setupLocalPropertyInfo(getTypes(objectValue, context(), true));
                m_defaultPropertyName = context()->defaultPropertyName(objectValue).toUtf8();
                m_isValid = true;
                setupPrototypes();
                m_signals = getSignals(objectValue, context());
            }
        }
    }
}

const CppComponentValue *NodeMetaInfoPrivate::getCppComponentValue() const
{
    const QList<TypeName> nameComponents = m_qualfiedTypeName.split('.');
    if (nameComponents.size() < 2)
        return 0;
    const TypeName type = nameComponents.last();

    TypeName module;
    for (int i = 0; i < nameComponents.size() - 1; ++i) {
        if (i != 0)
            module += '/';
        module += nameComponents.at(i);
    }

    // get the qml object value that's available in the document
    foreach (const QmlJS::Import &import, context()->imports(document())->all()) {
        if (import.info.path() != QString::fromUtf8(module))
            continue;
        const Value *lookupResult = import.object->lookupMember(QString::fromUtf8(type), context());
        const CppComponentValue *cppValue = value_cast<CppComponentValue>(lookupResult);
        if (cppValue
                && (m_majorVersion == -1 || m_majorVersion == cppValue->componentVersion().majorVersion())
                && (m_minorVersion == -1 || m_minorVersion == cppValue->componentVersion().minorVersion())
                )
            return cppValue;
    }

    const CppComponentValue *value = value_cast<CppComponentValue>(getObjectValue());
    if (value)
        return value;

    // maybe 'type' is a cpp name
    const CppComponentValue *cppValue = context()->valueOwner()->cppQmlTypes().objectByCppName(type);

    return cppValue;
}

const ObjectValue *NodeMetaInfoPrivate::getObjectValue() const
{
    return context()->lookupType(document(), lookupNameComponent());
}

ContextPtr NodeMetaInfoPrivate::context() const
{
    if (m_model && m_model->rewriterView() && m_model->rewriterView()->scopeChain())
        return m_model->rewriterView()->scopeChain()->context();
    return ContextPtr(0);
}

const Document *NodeMetaInfoPrivate::document() const
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
        if (!m_properties.contains(propertyInfo.first)) {
            m_properties.append(propertyInfo.first);
            m_propertyTypes.append(propertyInfo.second);
        }
    }
}

bool NodeMetaInfoPrivate::isPropertyWritable(const PropertyName &propertyName) const
{
    if (!isValid())
        return false;

    if (propertyName.contains('.')) {
        const PropertyNameList parts = propertyName.split('.');
        const PropertyName objectName = parts.first();
        const PropertyName rawPropertyName = parts.last();
        const QString objectType = propertyType(objectName);

        if (isValueType(objectType))
            return true;

        QSharedPointer<NodeMetaInfoPrivate> objectInfo(create(m_model, objectType.toUtf8()));
        if (objectInfo->isValid())
            return objectInfo->isPropertyWritable(rawPropertyName);
        else
            return true;
    }

    const CppComponentValue *qmlObjectValue = getNearestCppComponentValue();
    if (!qmlObjectValue)
        return true;
    if (qmlObjectValue->hasProperty(propertyName))
        return qmlObjectValue->isWritable(propertyName);
    else
        return true; //all properties of components are writable
}


bool NodeMetaInfoPrivate::isPropertyList(const PropertyName &propertyName) const
{
    if (!isValid())
        return false;

    if (propertyName.contains('.')) {
        const PropertyNameList parts = propertyName.split('.');
        const PropertyName objectName = parts.first();
        const PropertyName rawPropertyName = parts.last();
        const QString objectType = propertyType(objectName);

        if (isValueType(objectType))
            return false;

        QSharedPointer<NodeMetaInfoPrivate> objectInfo(create(m_model, objectType.toUtf8()));
        if (objectInfo->isValid())
            return objectInfo->isPropertyList(rawPropertyName);
        else
            return true;
    }

    const CppComponentValue *qmlObjectValue = getNearestCppComponentValue();
    if (!qmlObjectValue)
        return false;
    return qmlObjectValue->isListProperty(propertyName);
}

bool NodeMetaInfoPrivate::isPropertyPointer(const PropertyName &propertyName) const
{
    if (!isValid())
        return false;

    if (propertyName.contains('.')) {
        const PropertyNameList parts = propertyName.split('.');
        const PropertyName objectName = parts.first();
        const PropertyName rawPropertyName = parts.last();
        const QString objectType = propertyType(objectName);

        if (isValueType(objectType))
            return false;

        QSharedPointer<NodeMetaInfoPrivate> objectInfo(create(m_model, objectType.toUtf8()));
        if (objectInfo->isValid())
            return objectInfo->isPropertyPointer(rawPropertyName);
        else
            return true;
    }

    const CppComponentValue *qmlObjectValue = getNearestCppComponentValue();
    if (!qmlObjectValue)
        return false;
    return qmlObjectValue->isPointer(propertyName);
}

bool NodeMetaInfoPrivate::isPropertyEnum(const PropertyName &propertyName) const
{
    if (!isValid())
        return false;

    if (propertyType(propertyName).contains("Qt::"))
        return true;

    if (propertyName.contains('.')) {
        const PropertyNameList parts = propertyName.split('.');
        const PropertyName objectName = parts.first();
        const PropertyName rawPropertyName = parts.last();
        const QString objectType = propertyType(objectName);

        if (isValueType(objectType))
            return false;

        QSharedPointer<NodeMetaInfoPrivate> objectInfo(create(m_model, objectType.toUtf8()));
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

QString NodeMetaInfoPrivate::propertyEnumScope(const PropertyName &propertyName) const
{
    if (!isValid())
        return QString();

    if (propertyType(propertyName).contains("Qt::"))
        return QStringLiteral("Qt");

    if (propertyName.contains('.')) {
        const PropertyNameList parts = propertyName.split('.');
        const PropertyName objectName = parts.first();
        const PropertyName rawPropertyName = parts.last();
        const QString objectType = propertyType(objectName);

        if (isValueType(objectType))
            return QString();

        QSharedPointer<NodeMetaInfoPrivate> objectInfo(create(m_model, objectType.toUtf8()));
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
    if (definedIn) {
        QString nonCppPackage;
        foreach (const LanguageUtils::FakeMetaObject::Export &qmlExport, definedIn->metaObject()->exports()) {
            if (qmlExport.package != QStringLiteral("<cpp>"))
                nonCppPackage = qmlExport.package;
        }

        const LanguageUtils::FakeMetaObject::Export qmlExport =
                definedIn->metaObject()->exportInPackage(nonCppPackage);
        if (qmlExport.isValid())
            return qmlExport.type;

        return definedIn->className();
    }

    return QString();
}

static QString getUnqualifiedName(const QString &name)
{
    const QStringList nameComponents = name.split('.');
    if (nameComponents.size() < 2)
        return name;
    return nameComponents.last();
}

static QString getPackage(const QString &name)
{
    QStringList nameComponents = name.split('.');
    if (nameComponents.size() < 2)
        return QString();
    nameComponents.removeLast();

    return nameComponents.join(QLatin1Char('.'));
}

bool NodeMetaInfoPrivate::cleverCheckType(const QString &otherType) const
{
    if (otherType == qualfiedTypeName())
            return true;

    if (isFileComponent())
        return false;

    const QString typeName = getUnqualifiedName(otherType);
    const QString package = getPackage(otherType);

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

QVariant::Type NodeMetaInfoPrivate::variantTypeId(const PropertyName &properyName) const
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

    return QVariant::nameToType(typeName.toUtf8().data());
}

int NodeMetaInfoPrivate::majorVersion() const
{
    return m_majorVersion;
}

int NodeMetaInfoPrivate::minorVersion() const
{
    return m_minorVersion;
}

TypeName NodeMetaInfoPrivate::qualfiedTypeName() const
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
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    if (isValid()) {
        const Imports *imports = context()->imports(document());
        ImportInfo importInfo = imports->info(lookupNameComponent().last(), context().data());

        if (importInfo.type() == ImportType::Directory) {
            return importInfo.path();
        } else if (importInfo.type() == ImportType::Library) {
            if (modelManager) {
                foreach (const QString &importPath, model()->importPaths()) {
                    const QString targetPath = QDir(importPath).filePath(importInfo.path());
                    if (QDir(targetPath).exists())
                        return targetPath;
                    const QString targetPathVersion = QDir(importPath).filePath(importInfo.path()
                                                                                + QLatin1Char('.')
                                                                                + QString::number(importInfo.version().majorVersion()));
                    if (QDir(targetPathVersion).exists())
                        return targetPathVersion;
                }
            }
        }
    }
    return QString();
}

QString NodeMetaInfoPrivate::lookupName() const
{
    QString className = QString::fromUtf8(m_qualfiedTypeName);
    QString packageName;

    QStringList packageClassName = className.split(QLatin1Char('.'));
    if (packageClassName.size() > 1) {
        className = packageClassName.takeLast();
        packageName = packageClassName.join(QLatin1Char('.'));
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

TypeName NodeMetaInfoPrivate::propertyType(const PropertyName &propertyName) const
{
    if (!m_properties.contains(propertyName))
        return TypeName("Property does not exist...");
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
        description.className = ov->className().toUtf8();
        description.minorVersion = -1;
        description.majorVersion = -1;
        if (const CppComponentValue * qmlValue = value_cast<CppComponentValue>(ov)) {
            description.minorVersion = qmlValue->componentVersion().minorVersion();
            description.majorVersion = qmlValue->componentVersion().majorVersion();
            LanguageUtils::FakeMetaObject::Export qtquickExport = qmlValue->metaObject()->exportInPackage("QtQuick");
            LanguageUtils::FakeMetaObject::Export cppExport = qmlValue->metaObject()->exportInPackage("<cpp>");

            if (qtquickExport.isValid()) {
                description.className = qtquickExport.package.toUtf8() + '.' + qtquickExport.type.toUtf8();
            } else {
                bool found = false;
                if (cppExport.isValid()) {
                    foreach (const LanguageUtils::FakeMetaObject::Export &exportValue, qmlValue->metaObject()->exports()) {
                        if (exportValue.package.toUtf8() != "<cpp>") {
                            found = true;
                            description.className = exportValue.package.toUtf8() + '.' + exportValue.type.toUtf8();
                        }
                    }
                }
                if (!found) {
                    if (qmlValue->moduleName().isEmpty() && cppExport.isValid()) {
                        description.className = cppExport.package.toUtf8() + '.' + cppExport.type.toUtf8();
                    } else if (!qmlValue->moduleName().isEmpty()) {
                        description.className = qmlValue->moduleName().toUtf8() + '.' + description.className;
                    }
                }
            }
            m_prototypes.append(description);
        } else {
            if (context()->lookupType(document(), QStringList() << ov->className())) {
                const Imports *allImports = context()->imports(document());
                ImportInfo importInfo = allImports->info(description.className, context().data());

                if (importInfo.isValid()) {
                    QString uri = importInfo.name();
                    uri.replace(QStringLiteral(","), QStringLiteral("."));
                    if (!uri.isEmpty())
                        description.className = QString(uri + QString::fromLatin1(".") + QString::fromUtf8(description.className)).toUtf8();
                }

                m_prototypes.append(description);
            }
        }
    }
}


QList<TypeDescription> NodeMetaInfoPrivate::prototypes() const
{
    return m_prototypes;
}

const CppComponentValue *NodeMetaInfoPrivate::getNearestCppComponentValue() const
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

NodeMetaInfo::NodeMetaInfo(Model *model, TypeName type, int maj, int min) : m_privateData(Internal::NodeMetaInfoPrivate::create(model, type, maj, min))
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

bool NodeMetaInfo::hasProperty(const PropertyName &propertyName) const
{
    return propertyNames().contains(propertyName);
}

PropertyNameList NodeMetaInfo::propertyNames() const
{
    return m_privateData->properties();
}

PropertyNameList NodeMetaInfo::signalNames() const
{
    return m_privateData->signalNames();
}

PropertyNameList NodeMetaInfo::directPropertyNames() const
{
    return m_privateData->localProperties();
}

PropertyName NodeMetaInfo::defaultPropertyName() const
{
    return m_privateData->defaultPropertyName();
}

bool NodeMetaInfo::hasDefaultProperty() const
{
    return !defaultPropertyName().isEmpty();
}

TypeName NodeMetaInfo::propertyTypeName(const PropertyName &propertyName) const
{
    return m_privateData->propertyType(propertyName);
}

bool NodeMetaInfo::propertyIsWritable(const PropertyName &propertyName) const
{
    return m_privateData->isPropertyWritable(propertyName);
}

bool NodeMetaInfo::propertyIsListProperty(const PropertyName &propertyName) const
{
    return m_privateData->isPropertyList(propertyName);
}

bool NodeMetaInfo::propertyIsEnumType(const PropertyName &propertyName) const
{
    return m_privateData->isPropertyEnum(propertyName);
}

bool NodeMetaInfo::propertyIsPrivate(const PropertyName &propertyName) const
{
    return propertyName.startsWith("__");
}

QString NodeMetaInfo::propertyEnumScope(const PropertyName &propertyName) const
{
    return m_privateData->propertyEnumScope(propertyName);
}

QStringList NodeMetaInfo::propertyKeysForEnum(const PropertyName &propertyName) const
{
    return m_privateData->keysForEnum(propertyTypeName(propertyName));
}

QVariant NodeMetaInfo::propertyCastedValue(const PropertyName &propertyName, const QVariant &value) const
{

    const QVariant variant = value;
    QVariant copyVariant = variant;
    if (propertyIsEnumType(propertyName)
            || variant.canConvert<Enumeration>())
        return variant;

    const QString typeName = propertyTypeName(propertyName);

    QVariant::Type typeId = m_privateData->variantTypeId(propertyName);

    if (variant.type() == QVariant::UserType && variant.userType() == ModelNode::variantUserType()) {
        return variant;
    } else if (typeId == QVariant::UserType && typeName == QStringLiteral("QVariant")) {
        return variant;
    } else if (typeId == QVariant::UserType && typeName == QStringLiteral("variant")) {
        return variant;
    } else if (typeId == QVariant::UserType && typeName == QStringLiteral("var")) {
        return variant;
    } else if (variant.type() == QVariant::List) {
        // TODO: check the contents of the list
        return variant;
    } else if (typeName == "var" || typeName == "variant") {
        return variant;
    } else if (typeName == "alias") {
        // TODO: The QML compiler resolves the alias type. We probably should do the same.
        return variant;
    } else if (copyVariant.convert(typeId)) {
        return copyVariant;
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

QList<TypeName> NodeMetaInfo::superClassNames() const
{
    QList<TypeName> list;

    foreach (const Internal::TypeDescription &type,  m_privateData->prototypes()) {
        list.append(type.className);
    }
    return list;
}

bool NodeMetaInfo::defaultPropertyIsComponent() const
{
    if (hasDefaultProperty())
        return propertyTypeName(defaultPropertyName()) == "Component";

    return false;
}

TypeName NodeMetaInfo::typeName() const
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

bool NodeMetaInfo::isSubclassOf(const TypeName &type, int majorVersion, int minorVersion) const
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

bool NodeMetaInfo::isGraphicalItem() const
{
    return isSubclassOf("QtQuick.Item", -1, -1) || isSubclassOf("QtQuick.Window.Window", -1, -1);
}

void NodeMetaInfo::clearCache()
{
    Internal::NodeMetaInfoPrivate::clearCache();
}

bool NodeMetaInfo::isLayoutable() const
{
    if (isSubclassOf("<cpp>.QDeclarativeBasePositioner", -1, -1))
        return true; //QtQuick 1

    return isSubclassOf("QtQuick.Positioner", -1, -1)
            || isSubclassOf("QtQuick.Layouts.Layout", -1, -1)
            || isSubclassOf("QtQuick.Controls.SplitView", -1, -1);
}

bool NodeMetaInfo::isView() const
{
    return isValid() &&
            (isSubclassOf("QtQuick.ListView", -1, -1) ||
             isSubclassOf("QtQuick.GridView", -1, -1) ||
             isSubclassOf("QtQuick.PathView", -1, -1));
}

bool NodeMetaInfo::isTabView() const
{
    return isSubclassOf("QtQuick.Controls.TabView", -1, -1);
}

} // namespace QmlDesigner

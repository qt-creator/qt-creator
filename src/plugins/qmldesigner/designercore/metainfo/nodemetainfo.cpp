// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nodemetainfo.h"
#include "model.h"
#include "model/model_p.h"

#include "metainfo.h"
#include <enumeration.h>
#include <projectstorage/projectstorage.h>
#include <propertyparser.h>
#include <rewriterview.h>

#include <QDebug>
#include <QDir>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include <languageutils/fakemetaobject.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsvalueowner.h>

#include <utils/qtcassert.h>
#include <utils/algorithm.h>

namespace QmlDesigner {

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

namespace {

struct TypeDescription
{
    QString className;
    int minorVersion{};
    int majorVersion{};
};

using namespace QmlJS;

using PropertyInfo = QPair<PropertyName, TypeName>;

QVector<PropertyInfo> getObjectTypes(const ObjectValue *ov, const ContextPtr &context, bool local = false, int rec = 0);


static QByteArray getUnqualifiedName(const QByteArray &name)
{
    const QList<QByteArray> nameComponents = name.split('.');
    if (nameComponents.size() < 2)
        return name;
    return nameComponents.constLast();
}

static TypeName resolveTypeName(const ASTPropertyReference *ref, const ContextPtr &context, QVector<PropertyInfo> &dotProperties)
{
    TypeName type = "unknown";

    if (ref->ast()->propertyToken().isValid()) {
        type = ref->ast()->memberType->name.toUtf8();

        const Value *value = context->lookupReference(ref);

        if (!value)
            return type;

        if (const CppComponentValue * componentObjectValue = value->asCppComponentValue()) {
            type = componentObjectValue->className().toUtf8();
            dotProperties = getObjectTypes(componentObjectValue, context);
        }  else if (const ObjectValue * objectValue = value->asObjectValue()) {
            dotProperties = getObjectTypes(objectValue, context);
        }

        if (type == "alias") {

            if (const ASTObjectValue * astObjectValue = value->asAstObjectValue()) {
                if (astObjectValue->typeName()) {
                    type = astObjectValue->typeName()->name.toUtf8();
                    const ObjectValue *objectValue = context->lookupType(astObjectValue->document(), astObjectValue->typeName());
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
        const QList<LanguageUtils::FakeMetaObject::Export> &exports = cppComponent->metaObject()->exports();
        for (const LanguageUtils::FakeMetaObject::Export &e : exports) {
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
        for (const Export &e : std::as_const(cImport.possibleExports)) {
            if (e.pathRequired.isEmpty() || vContext.paths.count(e.pathRequired) > 0) {
                switch (e.exportName.type) {
                case ImportType::Library:
                {
                    QString typeName = objectValue->className();
                    if (!e.typeName.isEmpty() && e.typeName != Export::libraryTypeName()) {
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
                    for (const Utils::FilePath &path : std::as_const(vContext.paths)) {
                        if (filePath.startsWith(path.path()) && filePath.size() > path.path().size()
                            && filePath.at(path.path().size()) == QLatin1Char('/')) {
                            filePath = filePath.mid(path.path().size() + 1);
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
                for (const QString &libImport : std::as_const(possibleLibraries))
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
            QVector<PropertyInfo> dotProperties;
            const TypeName type = resolveTypeName(ref, m_context, dotProperties);
            m_properties.append({propertyName, type});
            if (!dotProperties.isEmpty()) {
                for (const PropertyInfo &propertyInfo : std::as_const(dotProperties)) {
                    PropertyName dotName = propertyInfo.first;
                    TypeName type = propertyInfo.second;
                    dotName = propertyName + '.' + dotName;
                    m_properties.append({dotName, type});
                }
            }
        } else {
            if (const CppComponentValue * cppComponentValue = value_cast<CppComponentValue>(value)) {
                TypeName qualifiedTypeName = qualifiedTypeNameForContext(cppComponentValue,
                    m_context->viewerContext(), *m_context->snapshot().importDependencies()).toUtf8();
                m_properties.append({propertyName, qualifiedTypeName});
            } else {
                QmlJS::TypeId typeId;
                TypeName typeName = typeId(value).toUtf8();

                if (typeName == "Function")
                    return processSlot(name, value);

                if (typeName == "number")
                    typeName = value->asIntValue() ? "int" : "real";

                m_properties.append({propertyName, typeName});
            }
        }
        return true;
    }

    bool processSignal(const QString &name, const Value * /*value*/) override
    {
        m_signals.append(name.toUtf8());
        return true;
    }

    bool processSlot(const QString &name, const Value * /*value*/) override
    {
        m_slots.append(name.toUtf8());
        return true;
    }

    QVector<PropertyInfo> properties() const { return m_properties; }

    PropertyNameList signalList() const { return m_signals; }
    PropertyNameList slotList() const { return m_slots; }

private:
    QVector<PropertyInfo> m_properties;
    PropertyNameList m_signals;
    PropertyNameList m_slots;
    const ContextPtr m_context;
};

inline static bool isValueType(const TypeName &type)
{
    static const PropertyTypeList objectValuesList({"QFont",
                                                    "QPoint",
                                                    "QPointF",
                                                    "QSize",
                                                    "QSizeF",
                                                    "QRect",
                                                    "QRectF",
                                                    "QVector2D",
                                                    "QVector3D",
                                                    "QVector4D",
                                                    "vector2d",
                                                    "vector3d",
                                                    "vector4d",
                                                    "font",
                                                    "QQuickIcon"});
    return objectValuesList.contains(type);
}

inline static bool isValueType(const QString &type)
{
    static const QStringList objectValuesList({"QFont",
                                               "QPoint",
                                               "QPointF",
                                               "QSize",
                                               "QSizeF",
                                               "QRect",
                                               "QRectF",
                                               "QVector2D",
                                               "QVector3D",
                                               "QVector4D",
                                               "vector2d",
                                               "vector3d",
                                               "vector4d",
                                               "font",
                                               "QQuickIcon"});
    return objectValuesList.contains(type);
}

const CppComponentValue *findQmlPrototype(const ObjectValue *ov, const ContextPtr &context)
{
    if (!ov)
        return nullptr;

    const CppComponentValue * qmlValue = value_cast<CppComponentValue>(ov);
    if (qmlValue)
        return qmlValue;

    return findQmlPrototype(ov->prototype(context), context);
}

QVector<PropertyInfo> getQmlTypes(const CppComponentValue *objectValue, const ContextPtr &context, bool local = false, int rec = 0);

QVector<PropertyInfo> getTypes(const ObjectValue *objectValue, const ContextPtr &context, bool local = false, int rec = 0)
{
    const CppComponentValue * qmlObjectValue = value_cast<CppComponentValue>(objectValue);

    if (qmlObjectValue)
        return getQmlTypes(qmlObjectValue, context, local, rec);

    return getObjectTypes(objectValue, context, local, rec);
}

QVector<PropertyInfo> getQmlTypes(const CppComponentValue *objectValue, const ContextPtr &context, bool local, int rec)
{
    QVector<PropertyInfo> propertyList;

    if (!objectValue)
        return propertyList;
    if (objectValue->className().isEmpty())
        return propertyList;

    if (rec > 4)
        return propertyList;

    PropertyMemberProcessor processor(context);
    objectValue->processMembers(&processor);

    for (const PropertyInfo &property : processor.properties()) {
        const PropertyName name = property.first;
        const QString nameAsString = QString::fromUtf8(name);
        if (!objectValue->isWritable(nameAsString) && objectValue->isPointer(nameAsString)) {
            //dot property
            const CppComponentValue * qmlValue = value_cast<CppComponentValue>(objectValue->lookupMember(nameAsString, context));
            if (qmlValue) {
                const QVector<PropertyInfo> dotProperties = getQmlTypes(qmlValue, context, false, rec + 1);
                for (const PropertyInfo &propertyInfo : dotProperties) {
                    const PropertyName dotName = name + '.' + propertyInfo.first;
                    const TypeName type = propertyInfo.second;
                    propertyList.append({dotName, type});
                }
            }
        }
        if (isValueType(objectValue->propertyType(nameAsString))) {
            const ObjectValue *dotObjectValue = value_cast<ObjectValue>(
                objectValue->lookupMember(nameAsString, context));

            if (dotObjectValue) {
                const QVector<PropertyInfo> dotProperties = getObjectTypes(dotObjectValue,
                                                                           context,
                                                                           false,
                                                                           rec + 1);
                for (const PropertyInfo &propertyInfo : dotProperties) {
                    const PropertyName dotName = name + '.' + propertyInfo.first;
                    const TypeName type = propertyInfo.second;
                    propertyList.append({dotName, type});
                }
            }
        }
        TypeName type = property.second;
        if (!objectValue->isPointer(nameAsString) && !objectValue->isListProperty(nameAsString))
            type = objectValue->propertyType(nameAsString).toUtf8();

        if (type == "unknown" && objectValue->hasProperty(nameAsString))
            type = objectValue->propertyType(nameAsString).toUtf8();

        propertyList.append({name, type});
    }

    if (!local)
        propertyList.append(getTypes(objectValue->prototype(context), context, local, rec));

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

    PrototypeIterator prototypeIterator(objectValue, context);
    QList<const ObjectValue *> objects = prototypeIterator.all();

    if (!local) {
        for (const ObjectValue *prototype : objects)
            signalList.append(getSignals(prototype, context, true));
    }

    std::sort(signalList.begin(), signalList.end());
    signalList.erase(std::unique(signalList.begin(), signalList.end()), signalList.end());

    return signalList;
}

PropertyNameList getSlots(const ObjectValue *objectValue, const ContextPtr &context, bool local = false)
{
    PropertyNameList slotList;

    if (!objectValue)
        return slotList;
    if (objectValue->className().isEmpty())
        return slotList;

    PropertyMemberProcessor processor(context);
    objectValue->processMembers(&processor);

    if (const ASTObjectValue *astObjectValue = objectValue->asAstObjectValue())
        astObjectValue->processMembers(&processor);

    slotList.append(processor.slotList());

    PrototypeIterator prototypeIterator(objectValue, context);
    const QList<const ObjectValue *> objects = prototypeIterator.all();

    if (!local) {
        for (const ObjectValue *prototype : objects)
            slotList.append(getSlots(prototype, context, true));
    }

    std::sort(slotList.begin(), slotList.end());
    slotList.erase(std::unique(slotList.begin(), slotList.end()), slotList.end());

    return slotList;
}

QVector<PropertyInfo> getObjectTypes(const ObjectValue *objectValue, const ContextPtr &context, bool local, int rec)
{
    QVector<PropertyInfo> propertyList;

    if (!objectValue)
        return propertyList;
    if (objectValue->className().isEmpty())
        return propertyList;

    if (rec > 4)
        return propertyList;

    PropertyMemberProcessor processor(context);
    objectValue->processMembers(&processor);

    const auto props = processor.properties();

    for (const PropertyInfo &property : props) {
        const PropertyName name = property.first;
        const QString nameAsString = QString::fromUtf8(name);

        if (isValueType(property.second)) {
            const Value *dotValue = objectValue->lookupMember(nameAsString, context);

            if (!dotValue)
                continue;

            if (const Reference *ref = dotValue->asReference())
                dotValue = context->lookupReference(ref);

            if (const ObjectValue *dotObjectValue = dotValue->asObjectValue()) {
                const QVector<PropertyInfo> dotProperties = getObjectTypes(dotObjectValue,
                                                                           context,
                                                                           false,
                                                                           rec + 1);
                for (const PropertyInfo &propertyInfo : dotProperties) {
                    const PropertyName dotName = name + '.' + propertyInfo.first;
                    const TypeName type = propertyInfo.second;
                    propertyList.append({dotName, type});
                }
            }
        }
        propertyList.append(property);
    }

    if (!local) {
        const ObjectValue* prototype = objectValue->prototype(context);
        // TODO: can we move this to getType methode and use that one here then
        if (prototype == objectValue)
            return propertyList;

        const CppComponentValue * qmlObjectValue = value_cast<CppComponentValue>(prototype);

        if (qmlObjectValue)
            propertyList.append(getQmlTypes(qmlObjectValue, context, local, rec + 1));
        else
            propertyList.append(getObjectTypes(prototype, context, local, rec + 1));
    }

    return propertyList;
}
} // namespace

class NodeMetaInfoPrivate
{
public:
    using Pointer = std::shared_ptr<NodeMetaInfoPrivate>;
    NodeMetaInfoPrivate() = delete;
    NodeMetaInfoPrivate(Model *model, TypeName type, int maj = -1, int min = -1);
    NodeMetaInfoPrivate(const NodeMetaInfoPrivate &) = delete;
    NodeMetaInfoPrivate &operator=(const NodeMetaInfoPrivate &) = delete;
    ~NodeMetaInfoPrivate() = default;

    bool isValid() const;
    bool isFileComponent() const;
    const PropertyNameList &properties() const;
    const PropertyNameList &localProperties() const;
    PropertyNameList signalNames() const;
    PropertyNameList slotNames() const;
    PropertyName defaultPropertyName() const;
    const TypeName &propertyType(const PropertyName &propertyName) const;

    void setupPrototypes();
    QList<TypeDescription> prototypes() const;

    bool isPropertyWritable(const PropertyName &propertyName) const;
    bool isPropertyPointer(const PropertyName &propertyName) const;
    bool isPropertyList(const PropertyName &propertyName) const;
    bool isPropertyEnum(const PropertyName &propertyName) const;
    QStringList keysForEnum(const QString &enumName) const;
    bool cleverCheckType(const TypeName &otherType) const;
    QVariant::Type variantTypeId(const PropertyName &properyName) const;

    int majorVersion() const;
    int minorVersion() const;
    const TypeName &qualfiedTypeName() const;
    Model *model() const;

    QByteArray cppPackageName() const;

    QString componentFileName() const;
    QString importDirectoryPath() const;
    Import requiredImport() const;

    static std::shared_ptr<NodeMetaInfoPrivate> create(Model *model,
                                                       const TypeName &type,
                                                       int maj = -1,
                                                       int min = -1);

    QSet<QByteArray> &prototypeCachePositives();
    QSet<QByteArray> &prototypeCacheNegatives();

private:

    const CppComponentValue *getCppComponentValue() const;
    const ObjectValue *getObjectValue() const;
    void setupPropertyInfo(const QVector<PropertyInfo> &propertyInfos);
    void setupLocalPropertyInfo(const QVector<PropertyInfo> &propertyInfos);
    QString lookupName() const;
    QStringList lookupNameComponent() const;
    const CppComponentValue *getNearestCppComponentValue() const;
    QString fullQualifiedImportAliasType() const;

    void ensureProperties() const;
    void initialiseProperties();

    TypeName m_qualfiedTypeName;
    int m_majorVersion = -1;
    int m_minorVersion = -1;
    bool m_isValid = false;
    bool m_isFileComponent = false;
    PropertyNameList m_properties;
    PropertyNameList m_signals;
    PropertyNameList m_slots;
    QList<TypeName> m_propertyTypes;
    PropertyNameList m_localProperties;
    PropertyName m_defaultPropertyName;
    QList<TypeDescription> m_prototypes;
    QSet<QByteArray> m_prototypeCachePositives;
    QSet<QByteArray> m_prototypeCacheNegatives;

    //storing the pointer would not be save
    ContextPtr context() const;
    const Document *document() const;

    QPointer<Model> m_model;
    const ObjectValue *m_objectValue = nullptr;
    bool m_propertiesSetup = false;
};

bool NodeMetaInfoPrivate::isFileComponent() const
{
    return m_isFileComponent;
}

const PropertyNameList &NodeMetaInfoPrivate::properties() const
{
    ensureProperties();

    return m_properties;
}

const PropertyNameList &NodeMetaInfoPrivate::localProperties() const
{
    ensureProperties();

    return m_localProperties;
}

PropertyNameList NodeMetaInfoPrivate::signalNames() const
{
    ensureProperties();
    return m_signals;
}

PropertyNameList NodeMetaInfoPrivate::slotNames() const
{
    ensureProperties();
    return m_slots;
}

QSet<QByteArray> &NodeMetaInfoPrivate::prototypeCachePositives()
{
    return m_prototypeCachePositives;
}

QSet<QByteArray> &NodeMetaInfoPrivate::prototypeCacheNegatives()
{
    return m_prototypeCacheNegatives;
}

PropertyName NodeMetaInfoPrivate::defaultPropertyName() const
{
    if (!m_defaultPropertyName.isEmpty())
        return m_defaultPropertyName;
    return PropertyName("data");
}

static TypeName stringIdentifier(const TypeName &type, int maj, int min)
{
    return type + QByteArray::number(maj) + '_' + QByteArray::number(min);
}

std::shared_ptr<NodeMetaInfoPrivate> NodeMetaInfoPrivate::create(Model *model,
                                                                 const TypeName &type,
                                                                 int major,
                                                                 int minor)
{
    auto stringfiedType = stringIdentifier(type, major, minor);
    auto &cache = model->d->nodeMetaInfoCache();
    if (auto found = cache.find(stringfiedType); found != cache.end())
        return *found;

    auto newData = std::make_shared<NodeMetaInfoPrivate>(model, type, major, minor);

    if (!newData->isValid())
        return newData;

    auto stringfiedQualifiedType = stringIdentifier(newData->qualfiedTypeName(),
                                                    newData->majorVersion(),
                                                    newData->minorVersion());

    if (auto found = cache.find(stringfiedQualifiedType); found != cache.end()) {
        newData = *found;
        cache.insert(stringfiedType, newData);
        return newData;
    }

    if (stringfiedQualifiedType != stringfiedType)
        cache.insert(stringfiedQualifiedType, newData);

    cache.insert(stringfiedType, newData);

    return newData;
}

NodeMetaInfoPrivate::NodeMetaInfoPrivate(Model *model, TypeName type, int maj, int min)
    : m_qualfiedTypeName(type)
    , m_majorVersion(maj)
    , m_minorVersion(min)
    , m_model(model)
{
    if (context()) {
        const CppComponentValue *cppObjectValue = getCppComponentValue();

        if (cppObjectValue) {
            if (m_majorVersion == -1 && m_minorVersion == -1) {
                m_majorVersion = cppObjectValue->componentVersion().majorVersion();
                m_minorVersion = cppObjectValue->componentVersion().minorVersion();
            }
            m_objectValue = cppObjectValue;
            m_defaultPropertyName = cppObjectValue->defaultPropertyName().toUtf8();
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
                        m_qualfiedTypeName = qmlValue->moduleName().toUtf8() + '.'
                                             + qmlValue->className().toUtf8();

                    } else if (m_majorVersion == qmlValue->componentVersion().majorVersion()
                               && m_minorVersion == qmlValue->componentVersion().minorVersion()) {
                        m_qualfiedTypeName = qmlValue->moduleName().toUtf8() + '.' + qmlValue->className().toUtf8();
                    } else {
                        return;
                    }
                } else {
                    m_isFileComponent = true;
                    const auto *imports = context()->imports(document());
                    const ImportInfo importInfo = imports->info(lookupNameComponent().constLast(),
                                                                context().data());

                    if (importInfo.isValid()) {
                        if (importInfo.type() == ImportType::Library) {
                            m_majorVersion = importInfo.version().majorVersion();
                            m_minorVersion = importInfo.version().minorVersion();
                        }
                        bool prepandName = (importInfo.type() == ImportType::Library
                                            || importInfo.type() == ImportType::Directory)
                                           && !m_qualfiedTypeName.contains('.');
                        if (prepandName)
                            m_qualfiedTypeName.prepend(importInfo.name().toUtf8() + '.');
                    }
                }
                m_objectValue = objectValue;
                m_defaultPropertyName = context()->defaultPropertyName(objectValue).toUtf8();
                m_isValid = true;
                setupPrototypes();
            } else {
                // Special case for aliased types for the rewriter

                const auto *imports = context()->imports(document());
                const ImportInfo importInfo = imports->info(QString::fromUtf8(m_qualfiedTypeName),
                                                            context().data());
                if (importInfo.isValid()) {
                    if (importInfo.type() == ImportType::Library) {
                        m_majorVersion = importInfo.version().majorVersion();
                        m_minorVersion = importInfo.version().minorVersion();
                    } else {
                        m_isFileComponent = true;
                    }

                    m_qualfiedTypeName = getUnqualifiedName(m_qualfiedTypeName);

                    bool prepandName = (importInfo.type() == ImportType::Library
                                        || importInfo.type() == ImportType::Directory);
                    if (prepandName)
                        m_qualfiedTypeName.prepend(importInfo.name().toUtf8() + '.');

                    m_qualfiedTypeName.replace("/", ".");
                }

                m_objectValue = getObjectValue();
                m_defaultPropertyName = context()->defaultPropertyName(objectValue).toUtf8();
                m_isValid = true;
                setupPrototypes();
            }
        }
    }
}

const CppComponentValue *NodeMetaInfoPrivate::getCppComponentValue() const
{
    const QList<TypeName> nameComponents = m_qualfiedTypeName.split('.');
    if (nameComponents.size() < 2)
        return nullptr;
    const TypeName &type = nameComponents.constLast();

    TypeName module;
    for (int i = 0; i < nameComponents.size() - 1; ++i) {
        if (i != 0)
            module += '/';
        module += nameComponents.at(i);
    }

    // get the qml object value that's available in the document
    const QmlJS::Imports *importsPtr = context()->imports(document());
    if (importsPtr) {
        const QList<QmlJS::Import> &imports = importsPtr->all();
        for (const QmlJS::Import &import : imports) {
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
    }

    const CppComponentValue *value = value_cast<CppComponentValue>(getObjectValue());
    if (value)
        return value;

    // maybe 'type' is a cpp name
    const CppComponentValue *cppValue = context()->valueOwner()->cppQmlTypes().objectByCppName(QString::fromUtf8(type));

    if (cppValue) {
        const QList<LanguageUtils::FakeMetaObject::Export> exports = cppValue->metaObject()->exports();
        for (const LanguageUtils::FakeMetaObject::Export &exportValue : exports) {
            if (exportValue.package.toUtf8() != "<cpp>") {
                const QList<QmlJS::Import> imports = context()->imports(document())->all();
                for (const QmlJS::Import &import : imports) {
                    if (import.info.path() != exportValue.package)
                        continue;
                    const Value *lookupResult = import.object->lookupMember(exportValue.type, context());
                    const CppComponentValue *cppValue = value_cast<CppComponentValue>(lookupResult);
                    if (cppValue)
                        return cppValue;
                }
            }
        }
    }

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
    return ContextPtr(nullptr);
}

const Document *NodeMetaInfoPrivate::document() const
{
    if (m_model && m_model->rewriterView())
        return m_model->rewriterView()->document();
    return nullptr;
}

void NodeMetaInfoPrivate::setupLocalPropertyInfo(const QVector<PropertyInfo> &localPropertyInfos)
{
    for (const PropertyInfo &propertyInfo : localPropertyInfos) {
        m_localProperties.append(propertyInfo.first);
    }
}

void NodeMetaInfoPrivate::setupPropertyInfo(const QVector<PropertyInfo> &propertyInfos)
{
    for (const PropertyInfo &propertyInfo : propertyInfos) {
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

    ensureProperties();

    if (propertyName.contains('.')) {
        const PropertyNameList parts = propertyName.split('.');
        const PropertyName &objectName = parts.constFirst();
        const PropertyName &rawPropertyName = parts.constLast();
        const TypeName objectType = propertyType(objectName);

        if (isValueType(objectType))
            return true;

        auto objectInfo = create(m_model, objectType);
        if (objectInfo->isValid())
            return objectInfo->isPropertyWritable(rawPropertyName);
        else
            return true;
    }

    const CppComponentValue *qmlObjectValue = getNearestCppComponentValue();
    if (!qmlObjectValue)
        return true;
    if (qmlObjectValue->hasProperty(QString::fromUtf8(propertyName)))
        return qmlObjectValue->isWritable(QString::fromUtf8(propertyName));
    else
        return true; //all properties of components are writable
}

bool NodeMetaInfoPrivate::isPropertyList(const PropertyName &propertyName) const
{
    if (!isValid())
        return false;

    ensureProperties();

    if (propertyName.contains('.')) {
        const PropertyNameList parts = propertyName.split('.');
        const PropertyName &objectName = parts.constFirst();
        const PropertyName &rawPropertyName = parts.constLast();
        const TypeName objectType = propertyType(objectName);

        if (isValueType(objectType))
            return false;

        auto objectInfo = create(m_model, objectType);
        if (objectInfo->isValid())
            return objectInfo->isPropertyList(rawPropertyName);
        else
            return true;
    }

    const CppComponentValue *qmlObjectValue = getNearestCppComponentValue();
    if (!qmlObjectValue)
        return false;

    if (!qmlObjectValue->hasProperty(QString::fromUtf8(propertyName))) {
        const TypeName typeName = propertyType(propertyName);
        return (typeName == "Item"  || typeName == "QtObject");
    }

    return qmlObjectValue->isListProperty(QString::fromUtf8(propertyName));
}

bool NodeMetaInfoPrivate::isPropertyPointer(const PropertyName &propertyName) const
{
    if (!isValid())
        return false;

    ensureProperties();

    if (propertyName.contains('.')) {
        const PropertyNameList parts = propertyName.split('.');
        const PropertyName &objectName = parts.constFirst();
        const PropertyName &rawPropertyName = parts.constLast();
        const TypeName objectType = propertyType(objectName);

        if (isValueType(objectType))
            return false;

        auto objectInfo = create(m_model, objectType);
        if (objectInfo->isValid())
            return objectInfo->isPropertyPointer(rawPropertyName);
        else
            return true;
    }

    const CppComponentValue *qmlObjectValue = getNearestCppComponentValue();
    if (!qmlObjectValue)
        return false;
    return qmlObjectValue->isPointer(QString::fromUtf8(propertyName));
}

bool NodeMetaInfoPrivate::isPropertyEnum(const PropertyName &propertyName) const
{
    if (!isValid())
        return false;

    ensureProperties();

    if (propertyType(propertyName).contains("Qt::"))
        return true;

    if (propertyName.contains('.')) {
        const PropertyNameList parts = propertyName.split('.');
        const PropertyName &objectName = parts.constFirst();
        const PropertyName &rawPropertyName = parts.constLast();
        const TypeName objectType = propertyType(objectName);

        if (isValueType(objectType))
            return false;

        auto objectInfo = create(m_model, objectType);
        if (objectInfo->isValid())
            return objectInfo->isPropertyEnum(rawPropertyName);
        else
            return false;
    }

    const CppComponentValue *qmlObjectValue = getNearestCppComponentValue();
    if (!qmlObjectValue)
        return false;
    return qmlObjectValue->getEnum(QString::fromUtf8(propertyType(propertyName))).isValid();
}

static QByteArray getPackage(const QByteArray &name)
{
    QList<QByteArray> nameComponents = name.split('.');
    if (nameComponents.size() < 2)
        return QByteArray();
    nameComponents.removeLast();

    return nameComponents.join('.');
}


QList<TypeName> qtObjectTypes()
{
    static QList<TypeName> typeNames = {"QML.QtObject", "QtQml.QtObject", "<cpp>.QObject"};

    return typeNames;
}

bool NodeMetaInfoPrivate::cleverCheckType(const TypeName &otherType) const
{
    if (otherType == qualfiedTypeName())
        return true;

    if (isFileComponent())
        return false;

    if (qtObjectTypes().contains(qualfiedTypeName()) && qtObjectTypes().contains(otherType))
        return true;

    const QByteArray typeName = getUnqualifiedName(otherType);
    const QByteArray package = getPackage(otherType);

    if (cppPackageName() == package)
        return QByteArray(package + '.' + typeName) == (cppPackageName() + '.' + getUnqualifiedName(qualfiedTypeName()));

    const CppComponentValue *qmlObjectValue = getCppComponentValue();
    if (!qmlObjectValue)
        return false;

    const LanguageUtils::FakeMetaObject::Export exp =
            qmlObjectValue->metaObject()->exportInPackage(QString::fromUtf8(package));
    QString convertedName = exp.type;
    if (convertedName.isEmpty())
        convertedName = qmlObjectValue->className();

    return typeName == convertedName.toUtf8();
}

QVariant::Type NodeMetaInfoPrivate::variantTypeId(const PropertyName &propertyName) const
{
    TypeName typeName = propertyType(propertyName);
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

    if (typeName == "vector2d")
        return QVariant::Vector2D;

    if (typeName == "vector3d")
        return QVariant::Vector3D;

    if (typeName == "vector4d")
        return QVariant::Vector4D;

    return QVariant::nameToType(typeName.data()); // This is deprecated
}

int NodeMetaInfoPrivate::majorVersion() const
{
    return m_majorVersion;
}

int NodeMetaInfoPrivate::minorVersion() const
{
    return m_minorVersion;
}

const TypeName &NodeMetaInfoPrivate::qualfiedTypeName() const
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
        return {};

    const CppComponentValue *qmlObjectValue = getNearestCppComponentValue();
    if (!qmlObjectValue)
        return {};
    return qmlObjectValue->getEnum(enumName).keys();
}

QByteArray NodeMetaInfoPrivate::cppPackageName() const
{
    if (!isFileComponent()) {
        if (const CppComponentValue *qmlObject = getCppComponentValue())
            return qmlObject->moduleName().toUtf8();
    }
    return QByteArray();
}

QString NodeMetaInfoPrivate::componentFileName() const
{
    if (isFileComponent()) {
        const ASTObjectValue * astObjectValue = value_cast<ASTObjectValue>(getObjectValue());
        if (astObjectValue) {
            Utils::FilePath fileName;
            int line;
            int column;
            if (astObjectValue->getSourceLocation(&fileName, &line, &column))
                return fileName.toString();
        }
    }
    return QString();
}

QString NodeMetaInfoPrivate::importDirectoryPath() const
{
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    if (isValid()) {
        const auto *imports = context()->imports(document());
        ImportInfo importInfo = imports->info(lookupNameComponent().constLast(), context().data());

        if (importInfo.type() == ImportType::Directory) {
            return importInfo.path();
        } else if (importInfo.type() == ImportType::Library) {
            if (modelManager) {
                const QStringList importPaths = model()->importPaths();
                for (const QString &importPath : importPaths) {
                    const QString targetPath = QDir(importPath).filePath(importInfo.path());
                    if (QDir(targetPath).exists())
                        return targetPath;
                    const QString targetPathVersion = QDir(importPath).filePath(importInfo.path()
                                                                                + '.'
                                                                                + QString::number(importInfo.version().majorVersion()));
                    if (QDir(targetPathVersion).exists())
                        return targetPathVersion;
                }
            }
        }
    }
    return QString();
}

Import NodeMetaInfoPrivate::requiredImport() const
{
    if (!isValid())
        return {};

    const auto *imports = context()->imports(document());
    ImportInfo importInfo = imports->info(lookupNameComponent().constLast(), context().data());

    if (importInfo.type() == ImportType::Directory) {
        return Import::createFileImport(importInfo.name(),
                                        importInfo.version().toString(),
                                        importInfo.as());
    } else if (importInfo.type() == ImportType::Library) {
        const QStringList importPaths = model()->importPaths();
        for (const QString &importPath : importPaths) {
            const QDir importDir(importPath);
            const QString targetPathVersion = importDir.filePath(
                importInfo.path() + '.' + QString::number(importInfo.version().majorVersion()));
            if (QDir(targetPathVersion).exists()) {
                return Import::createLibraryImport(importInfo.name(),
                                                   importInfo.version().toString(),
                                                   importInfo.as(),
                                                   {targetPathVersion});
            }

            const QString targetPath = importDir.filePath(importInfo.path());
            if (QDir(targetPath).exists()) {
                return Import::createLibraryImport(importInfo.name(),
                                                   importInfo.version().toString(),
                                                   importInfo.as(),
                                                   {targetPath});
            }
        }
    }
    return {};
}

QString NodeMetaInfoPrivate::lookupName() const
{
    QString className = QString::fromUtf8(m_qualfiedTypeName);
    QString packageName;

    QStringList packageClassName = className.split('.');
    if (packageClassName.size() > 1) {
        className = packageClassName.takeLast();
        packageName = packageClassName.join('.');
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

namespace {
TypeName nonexistingTypeName("Property does not exist...");
}

const TypeName &NodeMetaInfoPrivate::propertyType(const PropertyName &propertyName) const
{
    ensureProperties();

    if (!m_properties.contains(propertyName))
        return nonexistingTypeName;
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

    for (const ObjectValue *ov : std::as_const(objects)) {
        TypeDescription description;
        description.className = ov->className();
        description.minorVersion = -1;
        description.majorVersion = -1;
        if (description.className == "QQuickItem") {
            /* Ugly hack to recover from wrong prototypes for Item */
            if (const CppComponentValue *qmlValue = value_cast<CppComponentValue>(
                    context()->lookupType(document(), {"Item"}))) {
                description.className = "QtQuick.Item";
                description.minorVersion = qmlValue->componentVersion().minorVersion();
                description.majorVersion = qmlValue->componentVersion().majorVersion();
                m_prototypes.append(description);
            } else {
                qWarning() << Q_FUNC_INFO << "Lookup for Item failed";
            }
            continue;
        }

        if (const CppComponentValue *qmlValue = value_cast<CppComponentValue>(ov)) {
            description.minorVersion = qmlValue->componentVersion().minorVersion();
            description.majorVersion = qmlValue->componentVersion().majorVersion();
            LanguageUtils::FakeMetaObject::Export qtquickExport = qmlValue->metaObject()->exportInPackage(QLatin1String("QtQuick"));
            LanguageUtils::FakeMetaObject::Export cppExport = qmlValue->metaObject()->exportInPackage(QLatin1String("<cpp>"));

            if (qtquickExport.isValid()) {
                description.className = qtquickExport.package + '.' + qtquickExport.type;
            } else {
                bool found = false;
                if (cppExport.isValid()) {
                    const QList<LanguageUtils::FakeMetaObject::Export> exports = qmlValue->metaObject()->exports();
                    for (const LanguageUtils::FakeMetaObject::Export &exportValue : exports) {
                        if (exportValue.package.toUtf8() != "<cpp>") {
                            found = true;
                            description.className = exportValue.package + '.' + exportValue.type;
                        }
                    }
                }
                if (!found) {
                    if (qmlValue->moduleName().isEmpty() && cppExport.isValid()) {
                        description.className = cppExport.package + '.' + cppExport.type;
                    } else if (!qmlValue->moduleName().isEmpty()) {
                        description.className.prepend(qmlValue->moduleName() + QLatin1Char('.'));
                    }
                }
            }
            m_prototypes.append(description);
        } else {
            if (context()->lookupType(document(), {ov->className()})) {
                const auto *allImports = context()->imports(document());
                ImportInfo importInfo = allImports->info(description.className, context().data());

                if (importInfo.isValid()) {
                    QString uri = importInfo.name();
                    uri.replace(QStringLiteral(","), QStringLiteral("."));
                    if (!uri.isEmpty())
                        description.className = QString(uri + "." + description.className);
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
        return model()->rewriterView()->convertTypeToImportAlias(QString::fromUtf8(m_qualfiedTypeName));
    return QString::fromUtf8(m_qualfiedTypeName);
}

void NodeMetaInfoPrivate::ensureProperties() const
{
    if (m_propertiesSetup)
        return;

    const_cast<NodeMetaInfoPrivate*>(this)->initialiseProperties();
}

void NodeMetaInfoPrivate::initialiseProperties()
{
    if (!isValid())
        return;

    m_propertiesSetup = true;

    QTC_ASSERT(m_objectValue, qDebug() << qualfiedTypeName(); return);

    setupPropertyInfo(getTypes(m_objectValue, context()));
    setupLocalPropertyInfo(getTypes(m_objectValue, context(), true));

    m_signals = getSignals(m_objectValue, context());
    m_slots = getSlots(m_objectValue, context());
}

NodeMetaInfo::NodeMetaInfo() = default;
NodeMetaInfo::NodeMetaInfo(const NodeMetaInfo &) = default;
NodeMetaInfo &NodeMetaInfo::operator=(const NodeMetaInfo &) = default;
NodeMetaInfo::NodeMetaInfo(NodeMetaInfo &&) = default;
NodeMetaInfo &NodeMetaInfo::operator=(NodeMetaInfo &&) = default;

NodeMetaInfo::NodeMetaInfo(Model *model, const TypeName &type, int maj, int min)
    : m_privateData(NodeMetaInfoPrivate::create(model, type, maj, min))
{
}

NodeMetaInfo::~NodeMetaInfo() = default;

bool NodeMetaInfo::isValid() const
{
    if constexpr (useProjectStorage())
        return bool(m_typeId);
    else
        return m_privateData && m_privateData->isValid();
}

MetaInfoType NodeMetaInfo::type() const
{
    if constexpr (useProjectStorage()) {
        if (isValid()) {
            switch (typeData().traits) {
            case Storage::TypeTraits::Reference:
                return MetaInfoType::Reference;
            case Storage::TypeTraits::Value:
                return MetaInfoType::Value;
            case Storage::TypeTraits::Sequence:
                return MetaInfoType::Sequence;
            default:
                break;
            }
        }
    }

    return MetaInfoType::None;
}

bool NodeMetaInfo::isFileComponent() const
{
    if constexpr (useProjectStorage())
        return isValid() && bool(typeData().traits & Storage::TypeTraits::IsFileComponent);
    else
        return isValid() && m_privateData->isFileComponent();
}

bool NodeMetaInfo::isProjectComponent() const
{
    if constexpr (useProjectStorage()) {
        return isValid() && bool(typeData().traits & Storage::TypeTraits::IsProjectComponent);
    }

    return false;
}

bool NodeMetaInfo::isInProjectModule() const
{
    if constexpr (useProjectStorage()) {
        return isValid() && bool(typeData().traits & Storage::TypeTraits::IsInProjectModule);
    }

    return false;
}

namespace {

[[maybe_unused]] auto propertyId(const ProjectStorageType &projectStorage,
                                 TypeId typeId,
                                 Utils::SmallStringView propertyName)
{
    auto begin = propertyName.begin();
    const auto end = propertyName.end();

    auto found = std::find(begin, end, '.');
    auto propertyId = projectStorage.propertyDeclarationId(typeId, {begin, found});

    if (propertyId && found != end) {
        auto propertyData = projectStorage.propertyDeclaration(propertyId);
        if (auto propertyTypeId = propertyData->propertyTypeId) {
            begin = std::next(found);
            found = std::find(begin, end, '.');
            propertyId = projectStorage.propertyDeclarationId(propertyTypeId, {begin, found});

            if (propertyId && found != end) {
                begin = std::next(found);
                return projectStorage.propertyDeclarationId(propertyTypeId, {begin, end});
            }
        }
    }

    return propertyId;
}

} // namespace

bool NodeMetaInfo::hasProperty(Utils::SmallStringView propertyName) const
{
    if constexpr (useProjectStorage())
        return isValid() && bool(propertyId(*m_projectStorage, m_typeId, propertyName));
    else
        return isValid() && m_privateData->properties().contains(propertyName);
}

PropertyMetaInfos NodeMetaInfo::properties() const
{
    if (!isValid())
        return {};

    if constexpr (useProjectStorage()) {
        if (isValid()) {
            return Utils::transform<PropertyMetaInfos>(
                m_projectStorage->propertyDeclarationIds(m_typeId), [&](auto id) {
                    return PropertyMetaInfo{id, m_projectStorage};
                });
        }
    } else {
        const auto &properties = m_privateData->properties();

        PropertyMetaInfos propertyMetaInfos;
        propertyMetaInfos.reserve(static_cast<std::size_t>(properties.size()));

        for (const auto &name : properties)
            propertyMetaInfos.push_back({m_privateData, name});

        return propertyMetaInfos;
    }

    return {};
}

PropertyMetaInfos NodeMetaInfo::localProperties() const
{
    if constexpr (useProjectStorage()) {
        if (isValid()) {
            return Utils::transform<PropertyMetaInfos>(
                m_projectStorage->localPropertyDeclarationIds(m_typeId), [&](auto id) {
                    return PropertyMetaInfo{id, m_projectStorage};
                });
        }
    } else {
        const auto &properties = m_privateData->localProperties();

        PropertyMetaInfos propertyMetaInfos;
        propertyMetaInfos.reserve(static_cast<std::size_t>(properties.size()));

        for (const auto &name : properties)
            propertyMetaInfos.emplace_back(m_privateData, name);

        return propertyMetaInfos;
    }

    return {};
}

PropertyMetaInfo NodeMetaInfo::property(const PropertyName &propertyName) const
{
    if constexpr (useProjectStorage()) {
        if (isValid())
            return {propertyId(*m_projectStorage, m_typeId, propertyName), m_projectStorage};
    } else {
        if (hasProperty(propertyName)) {
            return PropertyMetaInfo{m_privateData, propertyName};
        }
    }

    return {};
}

PropertyNameList NodeMetaInfo::signalNames() const
{
    if constexpr (useProjectStorage()) {
        if (isValid()) {
            return Utils::transform<PropertyNameList>(m_projectStorage->signalDeclarationNames(
                                                          m_typeId),
                                                      [&](const auto &name) {
                                                          return name.toQByteArray();
                                                      });
        }
    } else {
        if (isValid())
            return m_privateData->signalNames();
    }

    return {};
}

PropertyNameList NodeMetaInfo::slotNames() const
{
    if constexpr (useProjectStorage()) {
        if (isValid()) {
            return Utils::transform<PropertyNameList>(m_projectStorage->functionDeclarationNames(
                                                          m_typeId),
                                                      [&](const auto &name) {
                                                          return name.toQByteArray();
                                                      });
        }
    } else {
        if (isValid())
            return m_privateData->slotNames();
    }

    return {};
}

PropertyName NodeMetaInfo::defaultPropertyName() const
{
    if constexpr (useProjectStorage()) {
        if (isValid()) {
            if (auto name = m_projectStorage->propertyName(typeData().defaultPropertyId)) {
                return name->toQByteArray();
            }
        }
    } else {
        if (isValid())
            return m_privateData->defaultPropertyName();
    }

    return {};
}

PropertyMetaInfo NodeMetaInfo::defaultProperty() const
{
    if constexpr (useProjectStorage()) {
        if (isValid()) {
            return PropertyMetaInfo(typeData().defaultPropertyId, m_projectStorage);
        }
    } else {
        return property(defaultPropertyName());
    }

    return {};
}
bool NodeMetaInfo::hasDefaultProperty() const
{
    if constexpr (useProjectStorage())
        return isValid() && bool(typeData().defaultPropertyId);
    else
        return !defaultPropertyName().isEmpty();
}

std::vector<NodeMetaInfo> NodeMetaInfo::selfAndPrototypes() const
{
    if constexpr (useProjectStorage()) {
        if (isValid()) {
            return Utils::transform<NodeMetaInfos>(
                m_projectStorage->prototypeAndSelfIds(m_typeId), [&](TypeId typeId) {
                    return NodeMetaInfo{typeId, m_projectStorage};
                });
        }
    } else {
        if (isValid()) {
            NodeMetaInfos hierarchy = {*this};
            Model *model = m_privateData->model();
            for (const TypeDescription &type : m_privateData->prototypes()) {
                auto &last = hierarchy.emplace_back(model,
                                                    type.className.toUtf8(),
                                                    type.majorVersion,
                                                    type.minorVersion);
                if (!last.isValid())
                    hierarchy.pop_back();
            }

            return hierarchy;
        }
    }

    return {};
}

NodeMetaInfos NodeMetaInfo::prototypes() const
{
    if constexpr (useProjectStorage()) {
        if (isValid()) {
            return Utils::transform<NodeMetaInfos>(
                m_projectStorage->prototypeIds(m_typeId), [&](TypeId typeId) {
                    return NodeMetaInfo{typeId, m_projectStorage};
                });
        }
    } else {
        if (isValid()) {
            NodeMetaInfos hierarchy;
            Model *model = m_privateData->model();
            for (const TypeDescription &type : m_privateData->prototypes()) {
                auto &last = hierarchy.emplace_back(model,
                                                    type.className.toUtf8(),
                                                    type.majorVersion,
                                                    type.minorVersion);
                if (!last.isValid())
                    hierarchy.pop_back();
            }

            return hierarchy;
        }
    }

    return {};
}

bool NodeMetaInfo::defaultPropertyIsComponent() const
{
    if (hasDefaultProperty())
        return defaultProperty().propertyType().isQmlComponent();

    return false;
}

TypeName NodeMetaInfo::typeName() const
{
    if (isValid())
        return m_privateData->qualfiedTypeName();

    return {};
}

TypeName NodeMetaInfo::simplifiedTypeName() const
{
    if (isValid())
        return typeName().split('.').constLast();

    return {};
}

int NodeMetaInfo::majorVersion() const
{
    if constexpr (!useProjectStorage()) {
        if (isValid())
            return m_privateData->majorVersion();
    }

    return -1;
}

int NodeMetaInfo::minorVersion() const
{
    if constexpr (!useProjectStorage()) {
        if (isValid())
            return m_privateData->minorVersion();
    }

    return -1;
}

Storage::Info::ExportedTypeNames NodeMetaInfo::allExportedTypeNames() const
{
    if constexpr (useProjectStorage()) {
        if (isValid()) {
            return m_projectStorage->exportedTypeNames(m_typeId);
        }
    }

    return {};
}

Storage::Info::ExportedTypeNames NodeMetaInfo::exportedTypeNamesForSourceId(SourceId sourceId) const
{
    if constexpr (useProjectStorage()) {
        if (isValid()) {
            return m_projectStorage->exportedTypeNames(m_typeId, sourceId);
        }
    }

    return {};
}

SourceId NodeMetaInfo::sourceId() const
{
    if constexpr (useProjectStorage()) {
        if (isValid()) {
            return typeData().sourceId;
        }
    }

    return SourceId{};
}

QString NodeMetaInfo::componentFileName() const
{
    if constexpr (!useProjectStorage()) {
        if (isValid()) {
            return m_privateData->componentFileName();
        }
    } else {
        if (isValid()) {
            return m_privateData->componentFileName();
        }
    }

    return {};
}

QString NodeMetaInfo::importDirectoryPath() const
{
    if constexpr (!useProjectStorage()) {
        if (isValid()) {
            return m_privateData->importDirectoryPath();
        }
    }

    return {};
}

QString NodeMetaInfo::requiredImportString() const
{
    if (!isValid())
        return {};

    Import imp = m_privateData->requiredImport();
    if (!imp.isEmpty())
        return imp.toImportString();

    return {};
}

SourceId NodeMetaInfo::propertyEditorPathId() const
{
    if (useProjectStorage()) {
        if (isValid())
            return m_projectStorage->propertyEditorPathId(m_typeId);
    }

    return SourceId{};
}

const Storage::Info::Type &NodeMetaInfo::typeData() const
{
    if (!m_typeData)
        m_typeData = m_projectStorage->type(m_typeId);

    return *m_typeData;
}

bool NodeMetaInfo::isSubclassOf(const TypeName &type, int majorVersion, int minorVersion) const
{
    if (!isValid()) {
        qWarning() << "NodeMetaInfo is invalid" << type;
        return false;
    }

    if (typeName().isEmpty())
        return false;

    if (typeName() == type)
        return true;

    if (m_privateData->prototypeCachePositives().contains(
            stringIdentifier(type, majorVersion, minorVersion)))
        return true; //take a shortcut - optimization

    if (m_privateData->prototypeCacheNegatives().contains(
            stringIdentifier(type, majorVersion, minorVersion)))
        return false; //take a shortcut - optimization

    const NodeMetaInfos superClassList = prototypes();
    for (const NodeMetaInfo &superClass : superClassList) {
        if (superClass.m_privateData->cleverCheckType(type)) {
            m_privateData->prototypeCachePositives().insert(
                stringIdentifier(type, majorVersion, minorVersion));
            return true;
        }
    }
    m_privateData->prototypeCacheNegatives().insert(stringIdentifier(type, majorVersion, minorVersion));
    return false;
}

bool NodeMetaInfo::isSuitableForMouseAreaFill() const
{
    if constexpr (useProjectStorage()) {
        if (!isValid()) {
            return false;
        }

        using namespace Storage::Info;
        auto itemId = m_projectStorage->commonTypeId<QtQuick, Item>();
        auto mouseAreaId = m_projectStorage->commonTypeId<QtQuick, MouseArea>();
        auto controlsControlId = m_projectStorage->commonTypeId<QtQuick_Controls, Control>();
        auto templatesControlId = m_projectStorage->commonTypeId<QtQuick_Templates, Control>();

        return m_projectStorage->isBasedOn(m_typeId,
                                           itemId,
                                           mouseAreaId,
                                           controlsControlId,
                                           templatesControlId);
    } else {
        return isSubclassOf("QtQuick.Item") && !isSubclassOf("QtQuick.MouseArea")
               && !isSubclassOf("QtQuick.Controls.Control")
               && !isSubclassOf("QtQuick.Templates.Control");
    }
}

bool NodeMetaInfo::isBasedOn(const NodeMetaInfo &metaInfo) const
{
    if constexpr (useProjectStorage()) {
        return m_projectStorage->isBasedOn(m_typeId, metaInfo.m_typeId);
    } else {
        if (!isValid())
            return false;
        if (majorVersion() == -1 && minorVersion() == -1)
            return isSubclassOf(metaInfo.typeName());
        return isSubclassOf(metaInfo.typeName(), metaInfo.majorVersion(), metaInfo.minorVersion());
    }
}

bool NodeMetaInfo::isBasedOn(const NodeMetaInfo &metaInfo1, const NodeMetaInfo &metaInfo2) const
{
    if constexpr (useProjectStorage()) {
        return m_projectStorage->isBasedOn(m_typeId, metaInfo1.m_typeId, metaInfo2.m_typeId);
    } else {
        if (!isValid())
            return false;
        if (majorVersion() == -1 && minorVersion() == -1)
            return (isSubclassOf(metaInfo1.typeName()) || isSubclassOf(metaInfo2.typeName()));

        return (
            isSubclassOf(metaInfo1.typeName(), metaInfo1.majorVersion(), metaInfo1.minorVersion())
            || isSubclassOf(metaInfo2.typeName(), metaInfo2.majorVersion(), metaInfo2.minorVersion()));
    }
}

bool NodeMetaInfo::isBasedOn(const NodeMetaInfo &metaInfo1,
                             const NodeMetaInfo &metaInfo2,
                             const NodeMetaInfo &metaInfo3) const
{
    if constexpr (useProjectStorage()) {
        return m_projectStorage->isBasedOn(m_typeId,
                                           metaInfo1.m_typeId,
                                           metaInfo2.m_typeId,
                                           metaInfo3.m_typeId);
    } else {
        if (!isValid())
            return false;
        if (majorVersion() == -1 && minorVersion() == -1)
            return (isSubclassOf(metaInfo1.typeName()) || isSubclassOf(metaInfo2.typeName())
                    || isSubclassOf(metaInfo3.typeName()));

        return (
            isSubclassOf(metaInfo1.typeName(), metaInfo1.majorVersion(), metaInfo1.minorVersion())
            || isSubclassOf(metaInfo2.typeName(), metaInfo2.majorVersion(), metaInfo2.minorVersion())
            || isSubclassOf(metaInfo3.typeName(), metaInfo3.majorVersion(), metaInfo3.minorVersion()));
    }
}

bool NodeMetaInfo::isBasedOn(const NodeMetaInfo &metaInfo1,
                             const NodeMetaInfo &metaInfo2,
                             const NodeMetaInfo &metaInfo3,
                             const NodeMetaInfo &metaInfo4) const
{
    if constexpr (useProjectStorage()) {
        return m_projectStorage->isBasedOn(m_typeId,
                                           metaInfo1.m_typeId,
                                           metaInfo2.m_typeId,
                                           metaInfo3.m_typeId,
                                           metaInfo4.m_typeId);
    } else {
        return isValid()
               && (isSubclassOf(metaInfo1.typeName(), metaInfo1.majorVersion(), metaInfo1.minorVersion())
                   || isSubclassOf(metaInfo2.typeName(),
                                   metaInfo2.majorVersion(),
                                   metaInfo2.minorVersion())
                   || isSubclassOf(metaInfo3.typeName(),
                                   metaInfo3.majorVersion(),
                                   metaInfo3.minorVersion())
                   || isSubclassOf(metaInfo4.typeName(),
                                   metaInfo4.majorVersion(),
                                   metaInfo4.minorVersion()));
    }
}

bool NodeMetaInfo::isBasedOn(const NodeMetaInfo &metaInfo1,
                             const NodeMetaInfo &metaInfo2,
                             const NodeMetaInfo &metaInfo3,
                             const NodeMetaInfo &metaInfo4,
                             const NodeMetaInfo &metaInfo5) const
{
    if constexpr (useProjectStorage()) {
        return m_projectStorage->isBasedOn(m_typeId,
                                           metaInfo1.m_typeId,
                                           metaInfo2.m_typeId,
                                           metaInfo3.m_typeId,
                                           metaInfo4.m_typeId,
                                           metaInfo5.m_typeId);
    } else {
        return isValid()
               && (isSubclassOf(metaInfo1.typeName(), metaInfo1.majorVersion(), metaInfo1.minorVersion())
                   || isSubclassOf(metaInfo2.typeName(),
                                   metaInfo2.majorVersion(),
                                   metaInfo2.minorVersion())
                   || isSubclassOf(metaInfo3.typeName(),
                                   metaInfo3.majorVersion(),
                                   metaInfo3.minorVersion())
                   || isSubclassOf(metaInfo4.typeName(),
                                   metaInfo4.majorVersion(),
                                   metaInfo4.minorVersion())
                   || isSubclassOf(metaInfo5.typeName(),
                                   metaInfo5.majorVersion(),
                                   metaInfo5.minorVersion()));
    }
}

bool NodeMetaInfo::isBasedOn(const NodeMetaInfo &metaInfo1,
                             const NodeMetaInfo &metaInfo2,
                             const NodeMetaInfo &metaInfo3,
                             const NodeMetaInfo &metaInfo4,
                             const NodeMetaInfo &metaInfo5,
                             const NodeMetaInfo &metaInfo6) const
{
    if constexpr (useProjectStorage()) {
        return m_projectStorage->isBasedOn(m_typeId,
                                           metaInfo1.m_typeId,
                                           metaInfo2.m_typeId,
                                           metaInfo3.m_typeId,
                                           metaInfo4.m_typeId,
                                           metaInfo5.m_typeId,
                                           metaInfo6.m_typeId);
    } else {
        return isValid()
               && (isSubclassOf(metaInfo1.typeName(), metaInfo1.majorVersion(), metaInfo1.minorVersion())
                   || isSubclassOf(metaInfo2.typeName(),
                                   metaInfo2.majorVersion(),
                                   metaInfo2.minorVersion())
                   || isSubclassOf(metaInfo3.typeName(),
                                   metaInfo3.majorVersion(),
                                   metaInfo3.minorVersion())
                   || isSubclassOf(metaInfo4.typeName(),
                                   metaInfo4.majorVersion(),
                                   metaInfo4.minorVersion())
                   || isSubclassOf(metaInfo5.typeName(),
                                   metaInfo5.majorVersion(),
                                   metaInfo5.minorVersion())
                   || isSubclassOf(metaInfo6.typeName(),
                                   metaInfo6.majorVersion(),
                                   metaInfo6.minorVersion()));
    }
}

bool NodeMetaInfo::isBasedOn(const NodeMetaInfo &metaInfo1,
                             const NodeMetaInfo &metaInfo2,
                             const NodeMetaInfo &metaInfo3,
                             const NodeMetaInfo &metaInfo4,
                             const NodeMetaInfo &metaInfo5,
                             const NodeMetaInfo &metaInfo6,
                             const NodeMetaInfo &metaInfo7) const
{
    if constexpr (useProjectStorage()) {
        return m_projectStorage->isBasedOn(m_typeId,
                                           metaInfo1.m_typeId,
                                           metaInfo2.m_typeId,
                                           metaInfo3.m_typeId,
                                           metaInfo4.m_typeId,
                                           metaInfo5.m_typeId,
                                           metaInfo6.m_typeId,
                                           metaInfo7.m_typeId);
    } else {
        return isValid()
               && (isSubclassOf(metaInfo1.typeName(), metaInfo1.majorVersion(), metaInfo1.minorVersion())
                   || isSubclassOf(metaInfo2.typeName(),
                                   metaInfo2.majorVersion(),
                                   metaInfo2.minorVersion())
                   || isSubclassOf(metaInfo3.typeName(),
                                   metaInfo3.majorVersion(),
                                   metaInfo3.minorVersion())
                   || isSubclassOf(metaInfo4.typeName(),
                                   metaInfo4.majorVersion(),
                                   metaInfo4.minorVersion())
                   || isSubclassOf(metaInfo5.typeName(),
                                   metaInfo5.majorVersion(),
                                   metaInfo5.minorVersion())
                   || isSubclassOf(metaInfo6.typeName(),
                                   metaInfo6.majorVersion(),
                                   metaInfo6.minorVersion())
                   || isSubclassOf(metaInfo7.typeName(),
                                   metaInfo7.majorVersion(),
                                   metaInfo7.minorVersion()));
    }
}

namespace {
template<const char *moduleName, const char *typeName>
bool isBasedOnCommonType(NotNullPointer<const ProjectStorageType> projectStorage, TypeId typeId)
{
    if (!typeId) {
        return false;
    }

    auto base = projectStorage->commonTypeId<moduleName, typeName>();

    return projectStorage->isBasedOn(typeId, base);
}
} // namespace

bool NodeMetaInfo::isGraphicalItem() const
{
    if constexpr (useProjectStorage()) {
        if (!isValid()) {
            return false;
        }

        using namespace Storage::Info;
        auto itemId = m_projectStorage->commonTypeId<QtQuick, Item>();
        auto windowId = m_projectStorage->commonTypeId<QtQuick_Window, Window>();
        auto dialogId = m_projectStorage->commonTypeId<QtQuick_Dialogs, Dialog>();
        auto popupId = m_projectStorage->commonTypeId<QtQuick_Controls, Popup>();

        return m_projectStorage->isBasedOn(m_typeId, itemId, windowId, dialogId, popupId);
    } else {
        return isValid()
               && (isSubclassOf("QtQuick.Item") || isSubclassOf("QtQuick.Window.Window")
                   || isSubclassOf("QtQuick.Dialogs.Dialog")
                   || isSubclassOf("QtQuick.Controls.Popup"));
    }
}

bool NodeMetaInfo::isQtObject() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QML, QtObject>(m_projectStorage, m_typeId);
    } else {
        return isValid() && (isSubclassOf("QtQuick.QtObject") || isSubclassOf("QtQml.QtObject"));
    }
}

bool NodeMetaInfo::isLayoutable() const
{
    if constexpr (useProjectStorage()) {
        if (!isValid()) {
            return false;
        }

        using namespace Storage::Info;
        auto positionerId = m_projectStorage->commonTypeId<QtQuick, Positioner>();
        auto layoutId = m_projectStorage->commonTypeId<QtQuick_Layouts, Layout>();
        auto splitViewId = m_projectStorage->commonTypeId<QtQuick_Controls, SplitView>();

        return m_projectStorage->isBasedOn(m_typeId, positionerId, layoutId, splitViewId);

    } else {
        return isValid()
               && (isSubclassOf("QtQuick.Positioner") || isSubclassOf("QtQuick.Layouts.Layout")
                   || isSubclassOf("QtQuick.Controls.SplitView"));
    }
}

bool NodeMetaInfo::isQtQuickLayoutsLayout() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick_Layouts, Layout>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Layouts.Layout");
    }
}

bool NodeMetaInfo::isView() const
{
    if constexpr (useProjectStorage()) {
        if (!isValid()) {
            return false;
        }

        using namespace Storage::Info;
        auto listViewId = m_projectStorage->commonTypeId<QtQuick, ListView>();
        auto gridViewId = m_projectStorage->commonTypeId<QtQuick, GridView>();
        auto pathViewId = m_projectStorage->commonTypeId<QtQuick, PathView>();
        return m_projectStorage->isBasedOn(m_typeId, listViewId, gridViewId, pathViewId);
    } else {
        return isValid()
               && (isSubclassOf("QtQuick.ListView") || isSubclassOf("QtQuick.GridView")
                   || isSubclassOf("QtQuick.PathView"));
    }
}

bool NodeMetaInfo::usesCustomParser() const
{
    if constexpr (useProjectStorage()) {
        return isValid() && bool(typeData().traits & Storage::TypeTraits::UsesCustomParser);
    } else {
        if (!isValid())
            return false;

        auto type = typeName();
        return type == "QtQuick.VisualItemModel" || type == "Qt.VisualItemModel"
               || type == "QtQuick.VisualDataModel" || type == "Qt.VisualDataModel"
               || type == "QtQuick.ListModel" || type == "Qt.ListModel"
               || type == "QtQml.Models.ListModel" || type == "QtQuick.XmlListModel"
               || type == "Qt.XmlListModel" || type == "QtQml.XmlListModel.XmlListModel";
    }
}

namespace {

template<typename... TypeIds>
bool isTypeId(TypeId typeId, TypeIds... otherTypeIds)
{
    static_assert(((std::is_same_v<TypeId, TypeIds>) &&...), "Parameter must be a TypeId!");

    return ((typeId == otherTypeIds) || ...);
}

} // namespace

bool NodeMetaInfo::isVector2D() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isValid() && isTypeId(m_typeId, m_projectStorage->commonTypeId<QtQuick, vector2d>());
    } else {
        if (!m_privateData)
            return false;

        auto type = m_privateData->qualfiedTypeName();

        return type == "vector2d" || type == "QtQuick.vector2d" || type == "QVector2D";
    }
}

bool NodeMetaInfo::isVector3D() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isValid() && isTypeId(m_typeId, m_projectStorage->commonTypeId<QtQuick, vector3d>());
    } else {
        if (!m_privateData)
            return false;

        auto type = m_privateData->qualfiedTypeName();

        return type == "vector3d" || type == "QtQuick.vector3d" || type == "QVector3D";
    }
}

bool NodeMetaInfo::isVector4D() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isValid() && isTypeId(m_typeId, m_projectStorage->commonTypeId<QtQuick, vector4d>());
    } else {
        if (!m_privateData)
            return false;

        auto type = m_privateData->qualfiedTypeName();

        return type == "vector4d" || type == "QtQuick.vector4d" || type == "QVector4D";
    }
}

bool NodeMetaInfo::isQtQuickPropertyChanges() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick, Storage::Info::PropertyChanges>(m_projectStorage,
                                                                            m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.PropertyChanges");
    }
}

bool NodeMetaInfo::isQtSafeRendererSafeRendererPicture() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<Qt_SafeRenderer, SafeRendererPicture>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("Qt.SafeRenderer.SafeRendererPicture");
    }
}

bool NodeMetaInfo::isQtSafeRendererSafePicture() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<Qt_SafeRenderer, SafePicture>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("Qt.SafeRenderer.SafePicture");
    }
}

bool NodeMetaInfo::isQtQuickTimelineKeyframe() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick_Timeline, Keyframe>(m_projectStorage, m_typeId);

    } else {
        return isValid() && isSubclassOf("QtQuick.Timeline.Keyframe");
    }
}

bool NodeMetaInfo::isQtQuickTimelineTimelineAnimation() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick_Timeline, TimelineAnimation>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Timeline.TimelineAnimation");
    }
}

bool NodeMetaInfo::isQtQuickTimelineTimeline() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick_Timeline, Timeline>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Timeline.Timeline");
    }
}

bool NodeMetaInfo::isQtQuickTimelineKeyframeGroup() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick_Timeline, KeyframeGroup>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Timeline.KeyframeGroup");
    }
}

bool NodeMetaInfo::isListOrGridView() const
{
    if constexpr (useProjectStorage()) {
        if (!isValid()) {
            return false;
        }

        using namespace Storage::Info;
        auto listViewId = m_projectStorage->commonTypeId<QtQuick, ListView>();
        auto gridViewId = m_projectStorage->commonTypeId<QtQuick, GridView>();
        return m_projectStorage->isBasedOn(m_typeId, listViewId, gridViewId);
    } else {
        return isValid() && (isSubclassOf("QtQuick.ListView") || isSubclassOf("QtQuick.GridView"));
    }
}

bool NodeMetaInfo::isNumber() const
{
    if constexpr (useProjectStorage()) {
        if (!isValid()) {
            return false;
        }

        using namespace Storage::Info;
        auto intId = m_projectStorage->builtinTypeId<int>();
        auto uintId = m_projectStorage->builtinTypeId<uint>();
        auto floatId = m_projectStorage->builtinTypeId<float>();
        auto doubleId = m_projectStorage->builtinTypeId<double>();

        return isTypeId(m_typeId, intId, uintId, floatId, doubleId);
    } else {
        if (!isValid()) {
            return false;
        }

        auto type = simplifiedTypeName();

        return type == "int" || type == "uint" || type == "float" || type == "double";
    }
}

bool NodeMetaInfo::isQtQuickExtrasPicture() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick_Extras, Picture>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Extras.Picture");
    }
}

bool NodeMetaInfo::isQtQuickImage() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;

        return isBasedOnCommonType<QtQuick, Image>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Image");
    }
}

bool NodeMetaInfo::isQtQuickBorderImage() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;

        return isBasedOnCommonType<QtQuick, BorderImage>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.BorderImage");
    }
}

bool NodeMetaInfo::isAlias() const
{
    if constexpr (useProjectStorage()) {
        return false; // there is no type alias
    } else {
        return isValid() && m_privateData->qualfiedTypeName() == "alias";
    }
}

bool NodeMetaInfo::isQtQuickPositioner() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;

        return isBasedOnCommonType<QtQuick, Positioner>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Positioner");
    }
}

bool NodeMetaInfo::isQtQuickPropertyAnimation() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick, PropertyAnimation>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.PropertyAnimation");
    }
}

bool NodeMetaInfo::isQtQuickRepeater() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick, Repeater>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Repeater");
    }
}

bool NodeMetaInfo::isQtQuickControlsTabBar() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick_Controls, TabBar>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Controls.TabBar");
    }
}

bool NodeMetaInfo::isQtQuickControlsSwipeView() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick_Controls, SwipeView>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Controls.SwipeView");
    }
}

bool NodeMetaInfo::isQtQuick3DCamera() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, Camera>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Camera");
    }
}

bool NodeMetaInfo::isQtQuick3DBakedLightmap() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, BakedLightmap>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.BakedLightmap");
    }
}

bool NodeMetaInfo::isQtQuick3DBuffer() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, Buffer>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Buffer");
    }
}

bool NodeMetaInfo::isQtQuick3DInstanceListEntry() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, InstanceListEntry>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.InstanceListEntry");
    }
}

bool NodeMetaInfo::isQtQuick3DLight() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, Light>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Light");
    }
}

bool NodeMetaInfo::isQtQuickListElement() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQml_Models, ListElement>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.ListElement");
    }
}

bool NodeMetaInfo::isQtQuickListModel() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQml_Models, ListModel>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.ListModel");
    }
}

bool NodeMetaInfo::isQtQuick3DInstanceList() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, InstanceList>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.InstanceList");
    }
}

bool NodeMetaInfo::isQtQuick3DParticles3DParticle3D() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D_Particles3D, Particle3D>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Particles3D.Particle3D");
    }
}

bool NodeMetaInfo::isQtQuick3DParticles3DParticleEmitter3D() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D_Particles3D, ParticleEmitter3D>(m_projectStorage,
                                                                             m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Particles3D.ParticleEmitter3D");
    }
}

bool NodeMetaInfo::isQtQuick3DParticles3DAttractor3D() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D_Particles3D, Attractor3D>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Particles3D.Attractor3D");
    }
}

bool NodeMetaInfo::isQtQuick3DParticlesAbstractShape() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D_Particles3D_cppnative, QQuick3DParticleAbstractShape>(
            m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QQuick3DParticleAbstractShape");
    }
}

bool NodeMetaInfo::isQtQuickItem() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick, Item>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Item");
    }
}

bool NodeMetaInfo::isQtQuickPath() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick, Path>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Path");
    }
}

bool NodeMetaInfo::isQtQuickPauseAnimation() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick, PauseAnimation>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.PauseAnimation");
    }
}

bool NodeMetaInfo::isQtQuickTransition() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick, Transition>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Transition");
    }
}

bool NodeMetaInfo::isQtQuickWindowWindow() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick_Window, Window>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Window.Window");
    }
}

bool NodeMetaInfo::isQtQuickLoader() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick, Loader>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Loader");
    }
}

bool NodeMetaInfo::isQtQuickState() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick, State>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.State");
    }
}

bool NodeMetaInfo::isQtQuickStateOperation() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick_cppnative, QQuickStateOperation>(m_projectStorage,
                                                                            m_typeId);
    } else {
        return isValid() && isSubclassOf("<cpp>.QQuickStateOperation");
    }
}

bool NodeMetaInfo::isQtQuickText() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick, Text>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Text");
    }
}

bool NodeMetaInfo::isQtMultimediaSoundEffect() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtMultimedia, SoundEffect>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtMultimedia.SoundEffect");
    }
}

bool NodeMetaInfo::isFlowViewItem() const
{
    if constexpr (useProjectStorage()) {
        if (!isValid()) {
            return false;
        }

        using namespace Storage::Info;
        auto flowItemId = m_projectStorage->commonTypeId<FlowView, FlowItem>();
        auto flowWildcardId = m_projectStorage->commonTypeId<FlowView, FlowWildcard>();
        auto flowDecisionId = m_projectStorage->commonTypeId<FlowView, FlowDecision>();
        return m_projectStorage->isBasedOn(m_typeId, flowItemId, flowWildcardId, flowDecisionId);
    } else {
        return isValid()
               && (isSubclassOf("FlowView.FlowItem") || isSubclassOf("FlowView.FlowWildcard")
                   || isSubclassOf("FlowView.FlowDecision"));
    }
}

bool NodeMetaInfo::isFlowViewFlowItem() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<FlowView, FlowItem>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("FlowView.FlowItem");
    }
}

bool NodeMetaInfo::isFlowViewFlowView() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<FlowView, FlowView>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("FlowView.FlowView");
    }
}

bool NodeMetaInfo::isFlowViewFlowActionArea() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<FlowView, FlowActionArea>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("FlowView.FlowActionArea");
    }
}

bool NodeMetaInfo::isFlowViewFlowTransition() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<FlowView, FlowTransition>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("FlowView.FlowTransition");
    }
}

bool NodeMetaInfo::isFlowViewFlowDecision() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<FlowView, FlowDecision>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("FlowView.FlowDecision");
    }
}

bool NodeMetaInfo::isFlowViewFlowWildcard() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<FlowView, FlowWildcard>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("FlowView.FlowWildcard");
    }
}

bool NodeMetaInfo::isQtQuickStudioComponentsGroupItem() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick_Studio_Components, GroupItem>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick.Studio.Components.GroupItem");
    }
}

bool NodeMetaInfo::isQmlComponent() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QML, Component>(m_projectStorage, m_typeId);
    } else {
        if (!isValid())
            return false;

        auto type = m_privateData->qualfiedTypeName();

        return type == "Component" || type == "Qt.Component" || type == "QtQuick.Component"
               || type == "QtQml.Component" || type == "<cpp>.QQmlComponent" || type == "QQmlComponent"
               || type == "QML.Component" || type == "QtQml.Base.Component";
    }
}

bool NodeMetaInfo::isFont() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isValid() && isTypeId(m_typeId, m_projectStorage->commonTypeId<QtQuick, font>());
    } else {
        return isValid() && m_privateData->qualfiedTypeName() == "font";
    }
}

bool NodeMetaInfo::isColor() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isValid() && isTypeId(m_typeId, m_projectStorage->builtinTypeId<QColor>());
    } else {
        if (!isValid())
            return false;

        auto type = m_privateData->qualfiedTypeName();

        return type == "QColor" || type == "color" || type == "QtQuick.color";
    }
}

bool NodeMetaInfo::isBool() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isValid() && isTypeId(m_typeId, m_projectStorage->builtinTypeId<bool>());
    } else {
        if (!isValid())
            return false;

        auto type = simplifiedTypeName();

        return type == "bool" || type == "boolean";
    }
}

bool NodeMetaInfo::isInteger() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isValid() && isTypeId(m_typeId, m_projectStorage->builtinTypeId<int>());
    } else {
        if (!isValid())
            return false;

        auto type = simplifiedTypeName();

        return type == "int" || type == "integer";
    }
}

bool NodeMetaInfo::isFloat() const
{
    if constexpr (useProjectStorage()) {
        if (!isValid()) {
            return false;
        }

        using namespace Storage::Info;
        auto floatId = m_projectStorage->builtinTypeId<float>();
        auto doubleId = m_projectStorage->builtinTypeId<double>();

        return isTypeId(m_typeId, floatId, doubleId);
    } else {
        if (!isValid())
            return false;

        auto type = simplifiedTypeName();

        return type == "qreal" || type == "double" || type == "float";
    }
}

bool NodeMetaInfo::isVariant() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isValid() && isTypeId(m_typeId, m_projectStorage->builtinTypeId<QVariant>());
    } else {
        return isValid() && simplifiedTypeName() == "QVariant";
    }
}

bool NodeMetaInfo::isString() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isValid() && isTypeId(m_typeId, m_projectStorage->builtinTypeId<QString>());
    } else {
        if (!isValid())
            return false;

        auto type = simplifiedTypeName();

        return type == "string" || type == "QString";
    }
}

bool NodeMetaInfo::isUrl() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isValid() && isTypeId(m_typeId, m_projectStorage->builtinTypeId<QUrl>());
    } else {
        if (!isValid())
            return false;

        auto type = m_privateData->qualfiedTypeName();

        return type == "url" || type == "QUrl";
    }
}

bool NodeMetaInfo::isQtQuick3DTexture() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, Texture>(m_projectStorage, m_typeId);
    } else {
        return isValid()
               && (isSubclassOf("QtQuick3D.Texture") || isSubclassOf("<cpp>.QQuick3DTexture"));
    }
}

bool NodeMetaInfo::isQtQuick3DShader() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, Shader>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Shader");
    }
}

bool NodeMetaInfo::isQtQuick3DPass() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, Pass>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Pass");
    }
}

bool NodeMetaInfo::isQtQuick3DCommand() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, Command>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Command");
    }
}

bool NodeMetaInfo::isQtQuick3DDefaultMaterial() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, DefaultMaterial>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.DefaultMaterial");
    }
}

bool NodeMetaInfo::isQtQuick3DMaterial() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, Material>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Material");
    }
}

bool NodeMetaInfo::isQtQuick3DModel() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, Storage::Info::Model>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Model");
    }
}

bool NodeMetaInfo::isQtQuick3DNode() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, Node>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Node");
    }
}

bool NodeMetaInfo::isQtQuick3DParticles3DAffector3D() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D_Particles3D, Affector3D>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Affector3D");
    }
}

bool NodeMetaInfo::isQtQuick3DView3D() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, View3D>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.View3D");
    }
}

bool NodeMetaInfo::isQtQuick3DPrincipledMaterial() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, PrincipledMaterial>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.PrincipledMaterial");
    }
}

bool NodeMetaInfo::isQtQuick3DSpecularGlossyMaterial() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, SpecularGlossyMaterial>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.SpecularGlossyMaterial");
    }
}

bool NodeMetaInfo::isQtQuick3DParticles3DSpriteParticle3D() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D_Particles3D, SpriteParticle3D>(m_projectStorage,
                                                                            m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Particles3D.SpriteParticle3D");
    }
}

bool NodeMetaInfo::isQtQuick3DTextureInput() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, TextureInput>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.TextureInput");
    }
}

bool NodeMetaInfo::isQtQuick3DCubeMapTexture() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, CubeMapTexture>(m_projectStorage, m_typeId);
    } else {
        return isValid()
               && (isSubclassOf("QtQuick3D.CubeMapTexture")
                   || isSubclassOf("<cpp>.QQuick3DCubeMapTexture"));
    }
}

bool NodeMetaInfo::isQtQuick3DSceneEnvironment() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, SceneEnvironment>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.SceneEnvironment");
    }
}

bool NodeMetaInfo::isQtQuick3DEffect() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return isBasedOnCommonType<QtQuick3D, Effect>(m_projectStorage, m_typeId);
    } else {
        return isValid() && isSubclassOf("QtQuick3D.Effect");
    }
}

bool NodeMetaInfo::isEnumeration() const
{
    if constexpr (useProjectStorage())
        return isValid() && bool(typeData().traits & Storage::TypeTraits::IsEnum);

    return false;
}

PropertyMetaInfo::PropertyMetaInfo() = default;
PropertyMetaInfo::PropertyMetaInfo(const PropertyMetaInfo &) = default;
PropertyMetaInfo &PropertyMetaInfo::operator=(const PropertyMetaInfo &) = default;
PropertyMetaInfo::PropertyMetaInfo(PropertyMetaInfo &&) = default;
PropertyMetaInfo &PropertyMetaInfo::operator=(PropertyMetaInfo &&) = default;

PropertyMetaInfo::PropertyMetaInfo(
    [[maybe_unused]] std::shared_ptr<NodeMetaInfoPrivate> nodeMetaInfoPrivateData,
    [[maybe_unused]] const PropertyName &propertyName)
#ifndef QDS_USE_PROJECTSTORAGE
    : m_nodeMetaInfoPrivateData{nodeMetaInfoPrivateData}
    , m_propertyName{propertyName}
#endif
{}

PropertyMetaInfo::~PropertyMetaInfo() = default;

NodeMetaInfo PropertyMetaInfo::propertyType() const
{
    if constexpr (useProjectStorage()) {
        if (isValid())
            return {propertyData().propertyTypeId, m_projectStorage};
    } else {
        if (isValid())
            return NodeMetaInfo{nodeMetaInfoPrivateData()->model(),
                                nodeMetaInfoPrivateData()->propertyType(propertyName()),
                                -1,
                                -1};
    }

    return {};
}

NodeMetaInfo PropertyMetaInfo::type() const
{
    if constexpr (useProjectStorage()) {
        if (isValid())
            return NodeMetaInfo(propertyData().typeId, m_projectStorage);
    }

    return {};
}

PropertyName PropertyMetaInfo::name() const
{
    if (isValid()) {
        if constexpr (useProjectStorage())
            return PropertyName(Utils::SmallStringView(propertyData().name));
        else
            return propertyName();
    }

    return {};
}

bool PropertyMetaInfo::isWritable() const
{
    if constexpr (useProjectStorage())
        return isValid() && !(propertyData().traits & Storage::PropertyDeclarationTraits::IsReadOnly);
    else
        return isValid() && nodeMetaInfoPrivateData()->isPropertyWritable(propertyName());
}

bool PropertyMetaInfo::isReadOnly() const
{
    return !isWritable();
}

bool PropertyMetaInfo::isListProperty() const
{
    if constexpr (useProjectStorage())
        return isValid() && propertyData().traits & Storage::PropertyDeclarationTraits::IsList;
    else
        return isValid() && nodeMetaInfoPrivateData()->isPropertyList(propertyName());
}

bool PropertyMetaInfo::isEnumType() const
{
    if constexpr (useProjectStorage())
        return propertyType().isEnumeration();
    else
        return isValid() && nodeMetaInfoPrivateData()->isPropertyEnum(propertyName());
}

bool PropertyMetaInfo::isPrivate() const
{
    if constexpr (useProjectStorage())
        return isValid() && propertyData().name.startsWith("__");
    else
        return isValid() && propertyName().startsWith("__");
}

bool PropertyMetaInfo::isPointer() const
{
    if constexpr (useProjectStorage())
        return isValid() && (propertyData().traits & Storage::PropertyDeclarationTraits::IsPointer);
    else
        return isValid() && nodeMetaInfoPrivateData()->isPropertyPointer(propertyName());
}

namespace {
template<typename... QMetaTypes>
bool isType(const QMetaType &type, const QMetaTypes &...types)
{
    return ((type == types) || ...);
}
} // namespace

QVariant PropertyMetaInfo::castedValue(const QVariant &value) const
{
    if (!isValid())
        return {};

    if constexpr (!useProjectStorage()) {
        const QVariant variant = value;
        QVariant copyVariant = variant;
        if (isEnumType() || variant.canConvert<Enumeration>())
            return variant;

        const TypeName &typeName = propertyTypeName();

        QVariant::Type typeId = nodeMetaInfoPrivateData()->variantTypeId(propertyName());

        if (variant.typeId() == ModelNode::variantTypeId()) {
            return variant;
        } else if (typeId == QVariant::UserType && typeName == "QVariant") {
            return variant;
        } else if (typeId == QVariant::UserType && typeName == "variant") {
            return variant;
        } else if (typeId == QVariant::UserType && typeName == "var") {
            return variant;
        } else if (variant.typeId() == QVariant::List) {
            // TODO: check the contents of the list
            return variant;
        } else if (typeName == "var" || typeName == "variant") {
            return variant;
        } else if (typeName == "alias") {
            // TODO: The QML compiler resolves the alias type. We probably should do the same.
            return variant;
        } else if (typeName == "<cpp>.double") {
            return variant.toDouble();
        } else if (typeName == "<cpp>.float") {
            return variant.toFloat();
        } else if (typeName == "<cpp>.int") {
            return variant.toInt();
        } else if (typeName == "<cpp>.bool") {
            return variant.toBool();
        } else if (copyVariant.convert(typeId)) {
            return copyVariant;
        }

    } else {
        if (isEnumType() && value.canConvert<Enumeration>())
            return value;

        const TypeId &typeId = propertyData().propertyTypeId;

        static constexpr auto boolType = QMetaType::fromType<bool>();
        static constexpr auto intType = QMetaType::fromType<int>();
        static constexpr auto longType = QMetaType::fromType<long>();
        static constexpr auto longLongType = QMetaType::fromType<long long>();
        static constexpr auto floatType = QMetaType::fromType<float>();
        static constexpr auto doubleType = QMetaType::fromType<double>();
        static constexpr auto qStringType = QMetaType::fromType<QString>();
        static constexpr auto qUrlType = QMetaType::fromType<QUrl>();
        static constexpr auto qColorType = QMetaType::fromType<QColor>();

        if (value.typeId() == QVariant::UserType && value.typeId() == ModelNode::variantTypeId()) {
            return value;
        } else if (typeId == m_projectStorage->builtinTypeId<QVariant>()) {
            return value;
        } else if (typeId == m_projectStorage->builtinTypeId<double>()) {
            return value.toDouble();
        } else if (typeId == m_projectStorage->builtinTypeId<float>()) {
            return value.toFloat();
        } else if (typeId == m_projectStorage->builtinTypeId<int>()) {
            return value.toInt();
        } else if (typeId == m_projectStorage->builtinTypeId<bool>()) {
            return isType(value.metaType(), boolType, intType, longType, longLongType, floatType, doubleType)
                   && value.toBool();
        } else if (typeId == m_projectStorage->builtinTypeId<QString>()) {
            if (isType(value.metaType(), qStringType))
                return value;
            else
                return QString{};
        } else if (typeId == m_projectStorage->builtinTypeId<QDateTime>()) {
            return value.toDateTime();
        } else if (typeId == m_projectStorage->builtinTypeId<QUrl>()) {
            if (isType(value.metaType(), qUrlType))
                return value;
            else
                return QUrl{};
        } else if (typeId == m_projectStorage->builtinTypeId<QColor>()) {
            if (isType(value.metaType(), qColorType))
                return value;
            else
                return QColor{};
        } else if (typeId == m_projectStorage->builtinTypeId<QVector2D>()) {
            return value.value<QVector2D>();
        } else if (typeId == m_projectStorage->builtinTypeId<QVector3D>()) {
            return value.value<QVector3D>();
        } else if (typeId == m_projectStorage->builtinTypeId<QVector4D>()) {
            return value.value<QVector4D>();
        }
    }

    return {};
}

const Storage::Info::PropertyDeclaration &PropertyMetaInfo::propertyData() const
{
    if (!m_propertyData)
        m_propertyData = m_projectStorage->propertyDeclaration(m_id);

    return *m_propertyData;
}

TypeName PropertyMetaInfo::propertyTypeName() const
{
    return propertyType().typeName();
}

const NodeMetaInfoPrivate *PropertyMetaInfo::nodeMetaInfoPrivateData() const
{
#ifndef QDS_USE_PROJECTSTORAGE
    return m_nodeMetaInfoPrivateData.get();
#else
    return nullptr;
#endif
}

const PropertyName &PropertyMetaInfo::propertyName() const
{
#ifndef QDS_USE_PROJECTSTORAGE
    return m_propertyName;
#else
    static PropertyName dummy;
    return dummy;
#endif
}

NodeMetaInfo NodeMetaInfo::commonBase(const NodeMetaInfo &metaInfo) const
{
    if constexpr (useProjectStorage()) {
        if (isValid() && metaInfo) {
            const auto firstTypeIds = m_projectStorage->prototypeAndSelfIds(m_typeId);
            const auto secondTypeIds = m_projectStorage->prototypeAndSelfIds(metaInfo.m_typeId);
            auto found
                = std::find_if(firstTypeIds.begin(), firstTypeIds.end(), [&](TypeId firstTypeId) {
                      return std::find(secondTypeIds.begin(), secondTypeIds.end(), firstTypeId)
                             != secondTypeIds.end();
                  });

            if (found != firstTypeIds.end()) {
                return NodeMetaInfo{*found, m_projectStorage};
            }
        }
    } else {
        for (const NodeMetaInfo &info : metaInfo.selfAndPrototypes()) {
            if (isBasedOn(info)) {
                return info;
            }
        }
    }

    return {};
}

namespace {

void addCompoundProperties(CompoundPropertyMetaInfos &inflatedProperties,
                           const PropertyMetaInfo &parentProperty,
                           PropertyMetaInfos properties)
{
    for (PropertyMetaInfo &property : properties)
        inflatedProperties.emplace_back(std::move(property), parentProperty);
}

bool maybeCanHaveProperties(const NodeMetaInfo &type)
{
    if (!type)
        return false;

    using namespace Storage::Info;
    const auto &cache = type.projectStorage().commonTypeCache();
    auto typeId = type.id();
    const auto &typeIdsWithoutProperties = cache.typeIdsWithoutProperties();
    const auto begin = typeIdsWithoutProperties.begin();
    const auto end = typeIdsWithoutProperties.end();

    return std::find(begin, end, typeId) == end;
}

void addSubProperties(CompoundPropertyMetaInfos &inflatedProperties,
                      PropertyMetaInfo &propertyMetaInfo,
                      const NodeMetaInfo &propertyType)
{
    if (maybeCanHaveProperties(propertyType)) {
        auto subProperties = propertyType.properties();
        if (!subProperties.empty()) {
            addCompoundProperties(inflatedProperties, propertyMetaInfo, subProperties);
            return;
        }
    }

    inflatedProperties.emplace_back(std::move(propertyMetaInfo));
}

} // namespace

CompoundPropertyMetaInfos MetaInfoUtils::inflateValueProperties(PropertyMetaInfos properties)
{
    CompoundPropertyMetaInfos inflatedProperties;
    inflatedProperties.reserve(properties.size() * 2);

    for (auto &property : properties) {
        if (auto propertyType = property.propertyType(); propertyType.type() == MetaInfoType::Value)
            addSubProperties(inflatedProperties, property, propertyType);
        else
            inflatedProperties.emplace_back(std::move(property));
    }

    return inflatedProperties;
}

CompoundPropertyMetaInfos MetaInfoUtils::inflateValueAndReadOnlyProperties(PropertyMetaInfos properties)
{
    CompoundPropertyMetaInfos inflatedProperties;
    inflatedProperties.reserve(properties.size() * 2);

    for (auto &property : properties) {
        if (auto propertyType = property.propertyType();
            propertyType.type() == MetaInfoType::Value || property.isReadOnly()) {
            addSubProperties(inflatedProperties, property, propertyType);
        } else {
            inflatedProperties.emplace_back(std::move(property));
        }
    }

    return inflatedProperties;
}

} // namespace QmlDesigner

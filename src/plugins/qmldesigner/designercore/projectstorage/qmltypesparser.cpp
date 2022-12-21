// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltypesparser.h"

#include "projectstorage.h"
#include "sourcepathcache.h"

#include <sqlitedatabase.h>

#ifdef QDS_HAS_QMLDOM
#include <qmlcompiler/qqmljstypedescriptionreader_p.h>
#include <qmldom/qqmldomtop_p.h>
#endif

#include <QDateTime>

#include <algorithm>
#include <tuple>

namespace QmlDesigner {

#ifdef QDS_HAS_QMLDOM
namespace QmlDom = QQmlJS::Dom;

namespace {

using ComponentWithoutNamespaces = QMap<QString, QString>;

ComponentWithoutNamespaces createComponentNameWithoutNamespaces(
    const QHash<QString, QQmlJSExportedScope> &objects)
{
    ComponentWithoutNamespaces componentWithoutNamespaces;

    for (auto current = objects.keyBegin(), end = objects.keyEnd(); current != end; ++current) {
        const QString &key = *current;
        QString searchTerm{"::"};

        auto found = std::search(key.cbegin(), key.cend(), searchTerm.cbegin(), searchTerm.cend());

        if (found == key.cend())
            continue;

        componentWithoutNamespaces.insert(QStringView{std::next(found, 2), key.cend()}.toString(),
                                          key);
    }

    return componentWithoutNamespaces;
}

void appendImports(Storage::Synchronization::Imports &imports,
                   const QString &dependency,
                   SourceId sourceId,
                   QmlTypesParser::ProjectStorage &storage)
{
    auto spaceFound = std::find_if(dependency.begin(), dependency.end(), [](QChar c) {
        return c.isSpace();
    });

    Utils::PathString moduleName{QStringView(dependency.begin(), spaceFound)};
    moduleName.append("-cppnative");
    ModuleId cppModuleId = storage.moduleId(moduleName);

    imports.emplace_back(cppModuleId, Storage::Synchronization::Version{}, sourceId);
}

void addImports(Storage::Synchronization::Imports &imports,
                SourceId sourceId,
                const QStringList &dependencies,
                QmlTypesParser::ProjectStorage &storage,
                ModuleId cppModuleId)
{
    for (const QString &dependency : dependencies)
        appendImports(imports, dependency, sourceId, storage);

    imports.emplace_back(cppModuleId, Storage::Synchronization::Version{}, sourceId);

    if (ModuleId qmlCppModuleId = storage.moduleId("QML-cppnative"); cppModuleId != qmlCppModuleId)
        imports.emplace_back(qmlCppModuleId, Storage::Synchronization::Version{}, sourceId);
}

Storage::Synchronization::TypeTraits createTypeTraits(
    QQmlJSScope::AccessSemantics accessSematics)
{
    switch (accessSematics) {
    case QQmlJSScope::AccessSemantics::Reference:
        return Storage::Synchronization::TypeTraits::Reference;
    case QQmlJSScope::AccessSemantics::Value:
        return Storage::Synchronization::TypeTraits::Value;
    case QQmlJSScope::AccessSemantics::None:
        return Storage::Synchronization::TypeTraits::None;
    case QQmlJSScope::AccessSemantics::Sequence:
        return Storage::Synchronization::TypeTraits::Sequence;
    }

    return Storage::Synchronization::TypeTraits::None;
}

Storage::Synchronization::Version createVersion(QTypeRevision qmlVersion)
{
    return Storage::Synchronization::Version{qmlVersion.majorVersion(), qmlVersion.minorVersion()};
}

Storage::Synchronization::ExportedTypes createExports(const QList<QQmlJSScope::Export> &qmlExports,
                                                      Utils::SmallStringView interanalName,
                                                      QmlTypesParser::ProjectStorage &storage,
                                                      ModuleId cppModuleId)
{
    Storage::Synchronization::ExportedTypes exportedTypes;
    exportedTypes.reserve(Utils::usize(qmlExports));

    for (const QQmlJSScope::Export &qmlExport : qmlExports) {
        TypeNameString exportedTypeName{qmlExport.type()};
        exportedTypes.emplace_back(storage.moduleId(Utils::SmallString{qmlExport.package()}),
                                   std::move(exportedTypeName),
                                   createVersion(qmlExport.version()));
    }

    TypeNameString cppExportedTypeName{interanalName};
    exportedTypes.emplace_back(cppModuleId, cppExportedTypeName);

    return exportedTypes;
}

auto createCppEnumerationExport(Utils::SmallStringView typeName, Utils::SmallStringView enumerationName)
{
    TypeNameString cppExportedTypeName{typeName};
    cppExportedTypeName += "::";
    cppExportedTypeName += enumerationName;

    return cppExportedTypeName;
}

Storage::Synchronization::ExportedTypes createCppEnumerationExports(
    Utils::SmallStringView typeName,
    ModuleId cppModuleId,
    Utils::SmallStringView enumerationName,
    Utils::SmallStringView enumerationAlias)
{
    Storage::Synchronization::ExportedTypes exportedTypes;

    if (!enumerationAlias.empty()) {
        exportedTypes.reserve(2);
        exportedTypes.emplace_back(cppModuleId,
                                   createCppEnumerationExport(typeName, enumerationAlias));
    } else {
        exportedTypes.reserve(1);
    }

    exportedTypes.emplace_back(cppModuleId, createCppEnumerationExport(typeName, enumerationName));

    return exportedTypes;
}

Storage::PropertyDeclarationTraits createPropertyDeclarationTraits(
    const QQmlJSMetaProperty &qmlProperty)
{
    Storage::PropertyDeclarationTraits traits{};

    if (qmlProperty.isList())
        traits = traits | Storage::PropertyDeclarationTraits::IsList;

    if (qmlProperty.isPointer())
        traits = traits | Storage::PropertyDeclarationTraits::IsPointer;

    if (!qmlProperty.isWritable())
        traits = traits | Storage::PropertyDeclarationTraits::IsReadOnly;

    return traits;
}

struct EnumerationType
{
    EnumerationType(Utils::SmallStringView name, Utils::SmallStringView full)
        : name{name}
        , full{full}
    {}

    Utils::SmallString name;
    TypeNameString full;
};

using EnumerationTypes = std::vector<EnumerationType>;

TypeNameString fullyQualifiedTypeName(const QString &typeName,
                                      const ComponentWithoutNamespaces &componentNameWithoutNamespace)
{
    if (auto found = componentNameWithoutNamespace.find(typeName);
        found != componentNameWithoutNamespace.end())
        return found.value();

    return typeName;
}

Storage::Synchronization::PropertyDeclarations createProperties(
    const QHash<QString, QQmlJSMetaProperty> &qmlProperties,
    const EnumerationTypes &enumerationTypes,
    const ComponentWithoutNamespaces &componentNameWithoutNamespace)
{
    Storage::Synchronization::PropertyDeclarations propertyDeclarations;
    propertyDeclarations.reserve(Utils::usize(qmlProperties));

    for (const QQmlJSMetaProperty &qmlProperty : qmlProperties) {
        if (qmlProperty.typeName().isEmpty())
            continue;

        TypeNameString propertyTypeName{
            fullyQualifiedTypeName(qmlProperty.typeName(), componentNameWithoutNamespace)};

        auto found = find_if(enumerationTypes.begin(), enumerationTypes.end(), [&](auto &entry) {
            return entry.name == propertyTypeName;
        });

        if (found != enumerationTypes.end())
            propertyTypeName = found->full;

        propertyDeclarations.emplace_back(Utils::SmallString{qmlProperty.propertyName()},
                                          Storage::Synchronization::ImportedType{propertyTypeName},
                                          createPropertyDeclarationTraits(qmlProperty));
    }

    return propertyDeclarations;
}

Storage::Synchronization::ParameterDeclarations createParameters(
    const QQmlJSMetaMethod &qmlMethod, const ComponentWithoutNamespaces &componentNameWithoutNamespace)
{
    Storage::Synchronization::ParameterDeclarations parameterDeclarations;

    const QStringList &parameterNames = qmlMethod.parameterNames();
    const QStringList &parameterTypeNames = qmlMethod.parameterTypeNames();
    auto currentName = parameterNames.begin();
    auto currentType = parameterTypeNames.begin();
    auto nameEnd = parameterNames.end();
    auto typeEnd = parameterTypeNames.end();

    for (; currentName != nameEnd && currentType != typeEnd; ++currentName, ++currentType) {
        parameterDeclarations.emplace_back(Utils::SmallString{*currentName},
                                           fullyQualifiedTypeName(*currentType,
                                                                  componentNameWithoutNamespace));
    }

    return parameterDeclarations;
}

std::tuple<Storage::Synchronization::FunctionDeclarations, Storage::Synchronization::SignalDeclarations>
createFunctionAndSignals(const QMultiHash<QString, QQmlJSMetaMethod> &qmlMethods,
                         const ComponentWithoutNamespaces &componentNameWithoutNamespace)
{
    std::tuple<Storage::Synchronization::FunctionDeclarations, Storage::Synchronization::SignalDeclarations>
        functionAndSignalDeclarations;
    Storage::Synchronization::FunctionDeclarations &functionsDeclarations{
        std::get<0>(functionAndSignalDeclarations)};
    functionsDeclarations.reserve(Utils::usize(qmlMethods));
    Storage::Synchronization::SignalDeclarations &signalDeclarations{
        std::get<1>(functionAndSignalDeclarations)};
    signalDeclarations.reserve(Utils::usize(qmlMethods));

    for (const QQmlJSMetaMethod &qmlMethod : qmlMethods) {
        if (qmlMethod.isJavaScriptFunction())
            continue;

        if (qmlMethod.methodType() != QQmlJSMetaMethod::Type::Signal) {
            functionsDeclarations.emplace_back(Utils::SmallString{qmlMethod.methodName()},
                                               fullyQualifiedTypeName(qmlMethod.returnTypeName(),
                                                                      componentNameWithoutNamespace),
                                               createParameters(qmlMethod,
                                                                componentNameWithoutNamespace));
        } else {
            signalDeclarations.emplace_back(Utils::SmallString{qmlMethod.methodName()},
                                            createParameters(qmlMethod, componentNameWithoutNamespace));
        }
    }

    return functionAndSignalDeclarations;
}

Storage::Synchronization::EnumeratorDeclarations createEnumeratorsWithValues(
    const QQmlJSMetaEnum &qmlEnumeration)
{
    Storage::Synchronization::EnumeratorDeclarations enumeratorDeclarations;

    const QStringList &keys = qmlEnumeration.keys();
    const QList<int> &values = qmlEnumeration.values();
    auto currentKey = keys.begin();
    auto currentValue = values.begin();
    auto keyEnd = keys.end();
    auto valueEnd = values.end();

    for (; currentKey != keyEnd && currentValue != valueEnd; ++currentKey, ++currentValue)
        enumeratorDeclarations.emplace_back(Utils::SmallString{*currentKey}, *currentValue);

    return enumeratorDeclarations;
}

Storage::Synchronization::EnumeratorDeclarations createEnumeratorsWithoutValues(
    const QQmlJSMetaEnum &qmlEnumeration)
{
    Storage::Synchronization::EnumeratorDeclarations enumeratorDeclarations;

    for (const QString &key : qmlEnumeration.keys())
        enumeratorDeclarations.emplace_back(Utils::SmallString{key});

    return enumeratorDeclarations;
}

Storage::Synchronization::EnumeratorDeclarations createEnumerators(const QQmlJSMetaEnum &qmlEnumeration)
{
    if (qmlEnumeration.hasValues())
        return createEnumeratorsWithValues(qmlEnumeration);

    return createEnumeratorsWithoutValues(qmlEnumeration);
}

Storage::Synchronization::EnumerationDeclarations createEnumeration(
    const QHash<QString, QQmlJSMetaEnum> &qmlEnumerations)
{
    Storage::Synchronization::EnumerationDeclarations enumerationDeclarations;
    enumerationDeclarations.reserve(Utils::usize(qmlEnumerations));

    for (const QQmlJSMetaEnum &qmlEnumeration : qmlEnumerations) {
        enumerationDeclarations.emplace_back(TypeNameString{qmlEnumeration.name()},
                                             createEnumerators(qmlEnumeration));
    }

    return enumerationDeclarations;
}

auto addEnumerationType(EnumerationTypes &enumerationTypes,
                        Utils::SmallStringView typeName,
                        Utils::SmallStringView enumerationName)
{
    auto fullTypeName = TypeNameString::join({typeName, "::", enumerationName});
    enumerationTypes.emplace_back(enumerationName, std::move(fullTypeName));

    return fullTypeName;
}

void addEnumerationType(EnumerationTypes &enumerationTypes,
                        Storage::Synchronization::Types &types,
                        Utils::SmallStringView typeName,
                        Utils::SmallStringView enumerationName,
                        SourceId sourceId,
                        ModuleId cppModuleId,
                        Utils::SmallStringView enumerationAlias)
{
    auto fullTypeName = addEnumerationType(enumerationTypes, typeName, enumerationName);
    types.emplace_back(fullTypeName,
                       Storage::Synchronization::ImportedType{TypeNameString{}},
                       Storage::Synchronization::TypeTraits::Value
                           | Storage::Synchronization::TypeTraits::IsEnum,
                       sourceId,
                       createCppEnumerationExports(typeName,
                                                   cppModuleId,
                                                   enumerationName,
                                                   enumerationAlias));

    if (!enumerationAlias.empty())
        addEnumerationType(enumerationTypes, typeName, enumerationAlias);
}

QSet<QString> createEnumerationAliases(const QHash<QString, QQmlJSMetaEnum> &qmlEnumerations)
{
    QSet<QString> aliases;

    for (const QQmlJSMetaEnum &qmlEnumeration : qmlEnumerations) {
        if (auto &&alias = qmlEnumeration.alias(); !alias.isEmpty())
            aliases.insert(alias);
    }

    return aliases;
}

EnumerationTypes addEnumerationTypes(Storage::Synchronization::Types &types,
                                     Utils::SmallStringView typeName,
                                     SourceId sourceId,
                                     ModuleId cppModuleId,
                                     const QHash<QString, QQmlJSMetaEnum> &qmlEnumerations)
{
    EnumerationTypes enumerationTypes;
    enumerationTypes.reserve(Utils::usize(qmlEnumerations));

    QSet<QString> aliases = createEnumerationAliases(qmlEnumerations);

    for (const QQmlJSMetaEnum &qmlEnumeration : qmlEnumerations) {
        if (aliases.contains(qmlEnumeration.name()))
            continue;

        TypeNameString enumerationName{qmlEnumeration.name()};
        TypeNameString enumerationAlias{qmlEnumeration.alias()};
        addEnumerationType(enumerationTypes,
                           types,
                           typeName,
                           enumerationName,
                           sourceId,
                           cppModuleId,
                           enumerationAlias);
    }

    return enumerationTypes;
}

void addType(Storage::Synchronization::Types &types,
             SourceId sourceId,
             ModuleId cppModuleId,
             const QQmlJSExportedScope &exportScope,
             QmlTypesParser::ProjectStorage &storage,
             const ComponentWithoutNamespaces &componentNameWithoutNamespace)
{
    const auto &component = *exportScope.scope;

    auto [functionsDeclarations, signalDeclarations] = createFunctionAndSignals(
        component.ownMethods(), componentNameWithoutNamespace);
    TypeNameString typeName{component.internalName()};
    auto enumerations = component.ownEnumerations();
    auto exports = exportScope.exports;

    auto enumerationTypes = addEnumerationTypes(types, typeName, sourceId, cppModuleId, enumerations);
    types.emplace_back(Utils::SmallStringView{typeName},
                       Storage::Synchronization::ImportedType{TypeNameString{component.baseTypeName()}},
                       createTypeTraits(component.traits()),
                       sourceId,
                       createExports(exports, typeName, storage, cppModuleId),
                       createProperties(component.ownProperties(),
                                        enumerationTypes,
                                        componentNameWithoutNamespace),
                       std::move(functionsDeclarations),
                       std::move(signalDeclarations),
                       createEnumeration(enumerations));
}

void addTypes(Storage::Synchronization::Types &types,
              const Storage::Synchronization::ProjectData &projectData,
              const QHash<QString, QQmlJSExportedScope> &objects,
              QmlTypesParser::ProjectStorage &storage,
              const ComponentWithoutNamespaces &componentNameWithoutNamespaces)
{
    types.reserve(Utils::usize(objects) + types.size());

    for (const auto &object : objects)
        addType(types,
                projectData.sourceId,
                projectData.moduleId,
                object,
                storage,
                componentNameWithoutNamespaces);
}

} // namespace

void QmlTypesParser::parse(const QString &sourceContent,
                           Storage::Synchronization::Imports &imports,
                           Storage::Synchronization::Types &types,
                           const Storage::Synchronization::ProjectData &projectData)
{
    QQmlJSTypeDescriptionReader reader({}, sourceContent);
    QHash<QString, QQmlJSExportedScope> components;
    QStringList dependencies;
    bool isValid = reader(&components, &dependencies);
    if (!isValid)
        throw CannotParseQmlTypesFile{};

    auto componentNameWithoutNamespaces = createComponentNameWithoutNamespaces(components);

    addImports(imports, projectData.sourceId, dependencies, m_storage, projectData.moduleId);
    addTypes(types, projectData, components, m_storage, componentNameWithoutNamespaces);
}

#else

void QmlTypesParser::parse([[maybe_unused]] const QString &sourceContent,
                           [[maybe_unused]] Storage::Synchronization::Imports &imports,
                           [[maybe_unused]] Storage::Synchronization::Types &types,
                           [[maybe_unused]] const Storage::Synchronization::ProjectData &projectData)
{}

#endif

} // namespace QmlDesigner

// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltypesparser.h"

#include "projectstorage.h"
#include "projectstoragetracing.h"

#include <sqlitedatabase.h>

#include <utils/span.h>

#ifdef QDS_BUILD_QMLPARSER
#include <private/qqmldomtop_p.h>
#include <private/qqmljstypedescriptionreader_p.h>
#endif

#include <QDateTime>

#include <algorithm>
#include <tuple>

namespace QmlDesigner {

#ifdef QDS_BUILD_QMLPARSER

using NanotraceHR::keyValue;
using ProjectStorageTracing::category;
using Storage::IsInsideProject;
using Tracer = NanotraceHR::Tracer<ProjectStorageTracing::Category>;
using Storage::ModuleKind;
namespace QmlDom = QQmlJS::Dom;

namespace {

using ComponentWithoutNamespaces = QMap<QString, QString>;

using Storage::TypeNameString;

ComponentWithoutNamespaces createComponentNameWithoutNamespaces(const QList<QQmlJSExportedScope> &objects)
{
    NanotraceHR::Tracer tracer{"parse qmltypes file", category()};

    ComponentWithoutNamespaces componentWithoutNamespaces;

    for (const auto &object : objects) {
        const QString &name = object.scope->internalName();
        QString searchTerm{"::"};

        auto found = std::search(name.cbegin(), name.cend(), searchTerm.cbegin(), searchTerm.cend());

        if (found == name.cend())
            continue;

        componentWithoutNamespaces.insert(QStringView{std::next(found, 2), name.cend()}.toString(),
                                          name);
    }

    tracer.end(keyValue("components without namespace", componentWithoutNamespaces));

    return componentWithoutNamespaces;
}

const Storage::Import &appendImports(Storage::Imports &imports,
                                     const QString &dependency,
                                     SourceId sourceId,
                                     ModulesStorage &modulesStorage)
{
    auto spaceFound = std::ranges::find_if(dependency, [](QChar c) { return c.isSpace(); });

    Utils::PathString moduleName{QStringView(dependency.begin(), spaceFound)};
    ModuleId cppModuleId = modulesStorage.moduleId(moduleName, ModuleKind::CppLibrary);

    return imports.emplace_back(cppModuleId, Storage::Version{}, sourceId, sourceId);
}

void addImports(Storage::Imports &imports,
                SourceId sourceId,
                const QStringList &dependencies,
                ModulesStorage &modulesStorage,
                ModuleId cppModuleId)
{
    NanotraceHR::Tracer tracer{
        "add imports",
        category(),
        keyValue("source id", sourceId),
        keyValue("module id", cppModuleId),
    };

    for (const QString &dependency : dependencies) {
        const auto &import = appendImports(imports, dependency, sourceId, modulesStorage);
        tracer.tick("append import", keyValue("import", import), keyValue("dependency", dependency));
    }

    const auto &import = imports.emplace_back(cppModuleId, Storage::Version{}, sourceId, sourceId);
    tracer.tick("append import", keyValue("import", import));

    if (ModuleId qmlCppModuleId = modulesStorage.moduleId("QML", ModuleKind::CppLibrary);
        cppModuleId != qmlCppModuleId) {
        const auto &import = imports.emplace_back(qmlCppModuleId, Storage::Version{}, sourceId, sourceId);
        tracer.tick("append import", keyValue("import", import));
    }
}

Storage::TypeTraits createAccessTypeTraits(QQmlJSScope::AccessSemantics accessSematics)
{
    switch (accessSematics) {
    case QQmlJSScope::AccessSemantics::Reference:
        return Storage::TypeTraitsKind::Reference;
    case QQmlJSScope::AccessSemantics::Value:
        return Storage::TypeTraitsKind::Value;
    case QQmlJSScope::AccessSemantics::None:
        return Storage::TypeTraitsKind::None;
    case QQmlJSScope::AccessSemantics::Sequence:
        return Storage::TypeTraitsKind::Sequence;
    }

    return Storage::TypeTraitsKind::None;
}

Storage::TypeTraits createTypeTraits(QQmlJSScope::AccessSemantics accessSematics,
                                     bool hasCustomParser,
                                     bool isSingleton,
                                     IsInsideProject isInsideProject)
{
    auto typeTrait = createAccessTypeTraits(accessSematics);

    typeTrait.usesCustomParser = hasCustomParser;

    typeTrait.isSingleton = isSingleton;

    typeTrait.isInsideProject = isInsideProject == IsInsideProject::Yes;

    return typeTrait;
}

Storage::TypeTraits createListTypeTraits(IsInsideProject isInsideProject)
{
    Storage::TypeTraits typeTrait = Storage::TypeTraitsKind::Sequence;

    typeTrait.isInsideProject = isInsideProject == IsInsideProject::Yes;

    return typeTrait;
}

Storage::Version createVersion(QTypeRevision qmlVersion)
{
    return Storage::Version{qmlVersion.majorVersion(), qmlVersion.minorVersion()};
}

ModuleId getQmlModuleId(const QString &name,
                        ModulesStorage &modulesStorage,
                        Internal::LastModule &lastQmlModule)
{
    if (lastQmlModule.name == name)
        return lastQmlModule.id;

    ModuleId moduleId = modulesStorage.moduleId(Utils::PathString{name}, ModuleKind::QmlLibrary);

    lastQmlModule.name = name;
    lastQmlModule.id = moduleId;

    return moduleId;
}

Storage::Synchronization::ExportedTypes addExports(Storage::Synchronization::ExportedTypes &exportedTypes,
                                                   const QList<QQmlJSScope::Export> &qmlExports,
                                                   Utils::SmallStringView internalName,
                                                   const QStringList &interanalAliases,
                                                   ModulesStorage &modulesStorage,
                                                   ModuleId cppModuleId,
                                                   SourceId sourceId,
                                                   Internal::LastModule &lastQmlModule)
{
    exportedTypes.reserve(Utils::usize(qmlExports) + exportedTypes.size());

    TypeNameString cppExportedTypeName{internalName};

    for (const QQmlJSScope::Export &qmlExport : qmlExports) {
        TypeNameString exportedTypeName{qmlExport.type()};

        exportedTypes.emplace_back(sourceId,
                                   getQmlModuleId(qmlExport.package(), modulesStorage, lastQmlModule),
                                   std::move(exportedTypeName),
                                   createVersion(qmlExport.version()),
                                   sourceId,
                                   cppExportedTypeName);
    }

    exportedTypes.emplace_back(sourceId,
                               cppModuleId,
                               cppExportedTypeName,
                               Storage::Version{},
                               sourceId,
                               cppExportedTypeName);

    for (QStringView internalAlias : interanalAliases) {
        exportedTypes.emplace_back(sourceId,
                                   cppModuleId,
                                   Utils::SmallString{internalAlias},
                                   Storage::Version{},
                                   sourceId,
                                   cppExportedTypeName);
    }

    return exportedTypes;
}

auto createCppEnumerationExport(Utils::SmallStringView typeName, Utils::SmallStringView enumerationName)
{
    TypeNameString cppExportedTypeName{typeName};
    cppExportedTypeName += "::";
    cppExportedTypeName += enumerationName;

    return cppExportedTypeName;
}

Storage::Synchronization::ExportedTypes addCppEnumerationExports(
    Storage::Synchronization::ExportedTypes &exportedTypes,
    Utils::SmallStringView typeName,
    ModuleId cppModuleId,
    SourceId sourceId,
    Utils::SmallStringView enumerationName,
    Utils::SmallStringView enumerationAlias)
{
    auto scopedTypeName = createCppEnumerationExport(typeName, enumerationName);

    if (!enumerationAlias.empty()) {
        exportedTypes.reserve(2);
        auto name = createCppEnumerationExport(typeName, enumerationAlias);
        exportedTypes.emplace_back(sourceId, cppModuleId, name, Storage::Version{}, sourceId, scopedTypeName);
    } else {
        exportedTypes.reserve(1);
    }

    exportedTypes.emplace_back(
        sourceId, cppModuleId, scopedTypeName, Storage::Version{}, sourceId, scopedTypeName);

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

        auto found = std::ranges::find(enumerationTypes, propertyTypeName, &EnumerationType::name);

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

    for (const auto &parameter : qmlMethod.parameters()) {
        parameterDeclarations.emplace_back(Utils::SmallString{parameter.name()},
                                           fullyQualifiedTypeName(parameter.typeName(),
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

        if (qmlMethod.methodType() != QQmlJSMetaMethodType::Signal) {
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
                        Storage::Synchronization::ExportedTypes &exportedTypes,
                        Utils::SmallStringView typeName,
                        Utils::SmallStringView enumerationName,
                        SourceId sourceId,
                        ModuleId cppModuleId,
                        Utils::SmallStringView enumerationAlias)
{
    auto fullTypeName = addEnumerationType(enumerationTypes, typeName, enumerationName);
    Storage::TypeTraits typeTraits{Storage::TypeTraitsKind::Value};
    typeTraits.isEnum = true;
    types.emplace_back(fullTypeName,
                       Storage::Synchronization::ImportedType{TypeNameString{}},
                       Storage::Synchronization::ImportedType{},
                       typeTraits,
                       sourceId);

    addCppEnumerationExports(
        exportedTypes, typeName, cppModuleId, sourceId, enumerationName, enumerationAlias);

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
                                     Storage::Synchronization::ExportedTypes &exportedTypes,
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
                           exportedTypes,
                           typeName,
                           enumerationName,
                           sourceId,
                           cppModuleId,
                           enumerationAlias);
    }

    return enumerationTypes;
}

TypeNameString extensionTypeName(const auto &component)
{
    if (component.extensionIsJavaScript())
        return {};

    return component.extensionTypeName();
}

void addType(Storage::Synchronization::Types &types,
             Storage::Synchronization::ExportedTypes &exportedTypes,
             SourceId sourceId,
             ModuleId cppModuleId,
             const QQmlJSExportedScope &exportScope,
             ModulesStorage &modulesStorage,
             const ComponentWithoutNamespaces &componentNameWithoutNamespace,
             IsInsideProject isInsideProject,
             Internal::LastModule &lastQmlModule)
{
    NanotraceHR::Tracer tracer{"add type",
                               category(),
                               keyValue("source id", sourceId),
                               keyValue("module id", cppModuleId)};

    const auto &component = *exportScope.scope;

    TypeNameString typeName{component.internalName()};
    auto exports = exportScope.exports;
    auto internalAliases = component.aliases();

    addExports(exportedTypes,
               exports,
               typeName,
               internalAliases,
               modulesStorage,
               cppModuleId,
               sourceId,
               lastQmlModule);

    auto [functionsDeclarations, signalDeclarations] = createFunctionAndSignals(
        component.ownMethods(), componentNameWithoutNamespace);
    auto enumerations = component.ownEnumerations();
    auto enumerationTypes = addEnumerationTypes(
        types, exportedTypes, typeName, sourceId, cppModuleId, enumerations);

    const auto &type = types.emplace_back(
        Utils::SmallStringView{typeName},
        Storage::Synchronization::ImportedType{TypeNameString{component.baseTypeName()}},
        Storage::Synchronization::ImportedType{extensionTypeName(component)},
        createTypeTraits(component.accessSemantics(),
                         component.hasCustomParser(),
                         component.isSingleton(),
                         isInsideProject),
        sourceId,
        createProperties(component.ownProperties(), enumerationTypes, componentNameWithoutNamespace),
        std::move(functionsDeclarations),
        std::move(signalDeclarations),
        createEnumeration(enumerations),
        Utils::SmallString{component.ownDefaultPropertyName()});

    tracer.end(keyValue("type", type));

    if (component.isValueType()) {
        NanotraceHR::Tracer tracer{"add value list type", category()};

        const auto &listType = types.emplace_back(Utils::SmallString::join({"QList<", typeName, ">"}),
                                                  Storage::Synchronization::ImportedType{},
                                                  Storage::Synchronization::ImportedType{},
                                                  createListTypeTraits(isInsideProject),
                                                  sourceId);

        tracer.end(keyValue("type", listType));
    }

}

void addTypes(Storage::Synchronization::Types &types,
              Storage::Synchronization::ExportedTypes &exportedTypes,
              const Storage::Synchronization::ProjectEntryInfo &projectEntryInfo,
              const QList<QQmlJSExportedScope> &objects,
              ModulesStorage &modulesStorage,
              const ComponentWithoutNamespaces &componentNameWithoutNamespaces,
              IsInsideProject isInsideProject,
              Internal::LastModule &lastQmlModule)
{
    NanotraceHR::Tracer tracer{"add types", category()};
    types.reserve(Utils::usize(objects) + types.size());


    for (const auto &object : objects) {
        addType(types,
                exportedTypes,
                projectEntryInfo.sourceId,
                projectEntryInfo.moduleId,
                object,
                modulesStorage,
                componentNameWithoutNamespaces,
                isInsideProject,
                lastQmlModule);
    }
}

} // namespace

void QmlTypesParser::parse(const QString &sourceContent,
                           Storage::Imports &imports,
                           Storage::Synchronization::Types &types,
                           Storage::Synchronization::ExportedTypes &exportedTypes,
                           const Storage::Synchronization::ProjectEntryInfo &projectEntryInfo,
                           IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"qmltypes parser parse", category()};

    lastQmlModule.name.clear();
    lastQmlModule.id = ModuleId{};

    QQmlJSTypeDescriptionReader reader({}, sourceContent);
    QList<QQmlJSExportedScope> components;
    QStringList dependencies;
    bool isValid = reader(&components, &dependencies);
    if (!isValid)
        throw CannotParseQmlTypesFile{};

    auto componentNameWithoutNamespaces = createComponentNameWithoutNamespaces(components);

    addImports(imports, projectEntryInfo.sourceId, dependencies, m_modulesStorage, projectEntryInfo.moduleId);
    addTypes(types,
             exportedTypes,
             projectEntryInfo,
             components,
             m_modulesStorage,
             componentNameWithoutNamespaces,
             isInsideProject,
             lastQmlModule);
}

#else

void QmlTypesParser::parse([[maybe_unused]] const QString &sourceContent,
                           [[maybe_unused]] Storage::Imports &imports,
                           [[maybe_unused]] Storage::Synchronization::Types &types,
                           [[maybe_unused]] Storage::Synchronization::ExportedTypes &exportedTypes,
                           [[maybe_unused]] const Storage::Synchronization::ProjectEntryInfo &projectEntryInfo,
                           [[maybe_unused]] IsInsideProject isInsideProject)
{}

#endif

} // namespace QmlDesigner

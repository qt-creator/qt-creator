
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldocumentparser.h"

#include "projectstorage.h"
#include "projectstorageexceptions.h"
#include "projectstoragetracing.h"

#include <sourcepathstorage/sourcepathcache.h>
#include <sqlitedatabase.h>


#ifdef QDS_BUILD_QMLPARSER
#include <private/qqmldomtop_p.h>
#endif

#include <QDateTime>
#include <QVarLengthArray>

#include <filesystem>

namespace QmlDesigner {

#ifdef QDS_BUILD_QMLPARSER

using NanotraceHR::keyValue;
using ProjectStorageTracing::category;
using Storage::IsInsideProject;
using Tracer = NanotraceHR::Tracer<ProjectStorageTracing::Category>;

namespace QmlDom = QQmlJS::Dom;
namespace Synchronization = Storage::Synchronization;

using namespace Qt::StringLiterals;

namespace {

using ImportAliases = QVarLengthArray<QString, 24>;

Storage::Version convertVersion(QmlDom::Version version)
{
    return Storage::Version::convertFromSignedInteger(version.majorVersion, version.minorVersion);
}

Utils::PathString createNormalizedPath(Utils::SmallStringView directoryPath,
                                       const QString &relativePath)
{
    std::filesystem::path modulePath{std::string_view{directoryPath},
                                     std::filesystem::path::format::generic_format};

    modulePath /= relativePath.toStdString();

    Utils::PathString normalizedPath = modulePath.lexically_normal().generic_string();

    if (normalizedPath[normalizedPath.size() - 1] == '/')
        normalizedPath.resize(normalizedPath.size() - 1);

    return normalizedPath;
}

Storage::Import createImport(const QmlDom::Import &qmlImport,
                             SourceId sourceId,
                             Utils::SmallStringView directoryPath,
                             ModulesStorage &modulesStorage)
{
    using Storage::ModuleKind;
    using QmlUriKind = QQmlJS::Dom::QmlUri::Kind;

    auto &&uri = qmlImport.uri;

    switch (uri.kind()) {
    case QmlUriKind::AbsolutePath:
    case QmlUriKind::DirectoryUrl: {
        NanotraceHR::Tracer tracer{"qml document create directory import", category()};

        auto moduleId = modulesStorage.moduleId(Utils::PathString{uri.toString()},
                                                ModuleKind::PathLibrary);
        auto import = Storage::Import(moduleId,
                                      convertVersion(qmlImport.version),
                                      sourceId,
                                      sourceId,
                                      qmlImport.importId);

        tracer.end(keyValue("import", import));

        return import;
    }
    case QmlUriKind::RelativePath: {
        NanotraceHR::Tracer tracer{"qml document create relative path import", category()};
        auto path = createNormalizedPath(directoryPath, uri.localPath());
        auto moduleId = modulesStorage.moduleId(createNormalizedPath(directoryPath, uri.localPath()),
                                                ModuleKind::PathLibrary);
        auto import = Storage::Import(moduleId, Storage::Version{}, sourceId, sourceId, qmlImport.importId);

        tracer.end(keyValue("import", import));

        return import;
    }
    case QmlUriKind::ModuleUri: {
        NanotraceHR::Tracer tracer{"qml document create module import", category()};

        auto moduleId = modulesStorage.moduleId(Utils::PathString{uri.moduleUri()},
                                                ModuleKind::QmlLibrary);
        auto import = Storage::Import(moduleId,
                                      convertVersion(qmlImport.version),
                                      sourceId,
                                      sourceId,
                                      qmlImport.importId);

        tracer.end(keyValue("import", import));

        return import;
    }
    case QmlUriKind::Invalid:
        return Storage::Import{};
    }

    return Storage::Import{};
}

ImportAliases createImportAliases(const QList<QmlDom::Import> &qmlImports)
{
    NanotraceHR::Tracer tracer{"create qualified imports", category()};

    ImportAliases importAliases;

    for (const QmlDom::Import &qmlImport : qmlImports) {
        if (!qmlImport.importId.isEmpty() && !qmlImport.implicit)
            importAliases.push_back(qmlImport.importId);
    }

    tracer.end(keyValue("qualified imports", importAliases));

    return importAliases;
}

void addImports(Storage::Imports &imports,
                const QList<QmlDom::Import> &qmlImports,
                SourceId sourceId,
                Utils::SmallStringView directoryPath,
                ModulesStorage &modulesStorage)
{
    int importCount = 0;
    for (const QmlDom::Import &qmlImport : qmlImports) {
        if (!qmlImport.implicit) {
            imports.push_back(createImport(qmlImport, sourceId, directoryPath, modulesStorage));
            ++importCount;
        }
    }

    using Storage::ModuleKind;

    auto localDirectoryModuleId = modulesStorage.moduleId(directoryPath, ModuleKind::PathLibrary);
    imports.emplace_back(localDirectoryModuleId, Storage::Version{}, sourceId, sourceId);
    ++importCount;

    auto qmlModuleId = modulesStorage.moduleId("QML", ModuleKind::QmlLibrary);
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId, sourceId);
    ++importCount;

    auto end = imports.end();
    auto begin = std::ranges::prev(end, importCount);

    std::ranges::sort(begin, end);
    auto removed = std::ranges::unique(begin, end);
    imports.erase(removed.begin(), removed.end());
}

Synchronization::ImportedTypeName createImportedTypeName(const QStringView rawtypeName,
                                                         const ImportAliases &importAliases)
{
    auto foundDot = std::ranges::find(rawtypeName, '.');

    QStringView alias(rawtypeName.begin(), foundDot);

    if (foundDot == rawtypeName.end() or not importAliases.contains(alias))
        return Synchronization::ImportedType{rawtypeName};

    QStringView typeName(std::next(foundDot), rawtypeName.end());

    return Storage::Synchronization::QualifiedImportedType{typeName, alias};
}

bool isListProperty(const QStringView rawtypeName)
{
    return rawtypeName.startsWith(u"list<") && rawtypeName.endsWith(u'>');
}

struct TypeNameViewAndTraits
{
    QStringView typeName;
    Storage::PropertyDeclarationTraits traits;
};

TypeNameViewAndTraits filteredListTypeName(const QStringView rawtypeName)
{
    if (!isListProperty(rawtypeName))
        return {rawtypeName, Storage::PropertyDeclarationTraits::None};

    return {rawtypeName.mid(5, rawtypeName.size() - 6),
            Storage::PropertyDeclarationTraits::IsList};
};

struct TypeNameAndTraits
{
    Synchronization::ImportedTypeName importedTypeName;
    Storage::PropertyDeclarationTraits traits;
};

TypeNameAndTraits createImportedTypeNameAndTypeTraits(const QStringView rawtypeName,
                                                      const ImportAliases &importAliases)
{
    auto [filteredTypeName, traits] = filteredListTypeName(rawtypeName);

    if (!filteredTypeName.contains('.'))
        return {Synchronization::ImportedType{Utils::SmallString{filteredTypeName}}, traits};

    return {createImportedTypeName(filteredTypeName, importAliases), traits};
}

std::pair<Utils::SmallString, Utils::SmallString> createAccessPaths(const QStringList &accessPath)
{
    if (accessPath.size() == 1)
        return {accessPath[0], {}};

    if (accessPath.size() == 2)
        return {accessPath[0], accessPath[1]};

    return {};
}

void addPropertyDeclarations(Storage::Synchronization::Type &type,
                             QmlDom::QmlObject &rootObject,
                             const ImportAliases &importAliases,
                             QmlDom::DomItem &fileItem)
{
    for (const QmlDom::PropertyDefinition &propertyDeclaration : rootObject.propertyDefs()) {
        if (propertyDeclaration.isAlias()) {
            auto rootObjectItem = fileItem.copy(&rootObject);
            auto property = rootObjectItem.bindings()
                                .key(propertyDeclaration.name)
                                .index(0)
                                .field(QmlDom::Fields::value);
            auto resolvedAlias = rootObject.resolveAlias(rootObjectItem,
                                                         property.ownerAs<QmlDom::ScriptExpression>());
            if (!resolvedAlias.valid())
                continue;

            auto [importedTypeName, traits] = createImportedTypeNameAndTypeTraits(resolvedAlias.typeName,
                                                                                  importAliases);

            if (resolvedAlias.accessedPath.empty()) {
                type.propertyDeclarations.emplace_back(Utils::SmallString{propertyDeclaration.name},
                                                       std::move(importedTypeName),
                                                       traits);
            } else {
                auto [aliasPropertyName, aliasPropertyNameTail] = createAccessPaths(
                    resolvedAlias.accessedPath);

                type.propertyDeclarations.emplace_back(Utils::SmallString{propertyDeclaration.name},
                                                       std::move(importedTypeName),
                                                       traits,
                                                       Synchronization::PropertyKind::Alias,
                                                       aliasPropertyName,
                                                       aliasPropertyNameTail);
            }
        } else {
            auto [importedTypeName, traits] = createImportedTypeNameAndTypeTraits(
                propertyDeclaration.typeName, importAliases);
            type.propertyDeclarations.emplace_back(Utils::SmallString{propertyDeclaration.name},
                                                   std::move(importedTypeName),
                                                   traits);
        }
    }
}

void addParameterDeclaration(Storage::Synchronization::ParameterDeclarations &parameterDeclarations,
                             const QList<QmlDom::MethodParameter> &parameters)
{
    for (const QmlDom::MethodParameter &parameter : parameters) {
        parameterDeclarations.emplace_back(Utils::SmallString{parameter.name},
                                           Utils::SmallString{parameter.typeName});
    }
}

void addFunctionAndSignalDeclarations(Storage::Synchronization::Type &type,
                                      const QmlDom::QmlObject &rootObject)
{
    for (const QmlDom::MethodInfo &methodInfo : rootObject.methods()) {
        if (methodInfo.methodType == QmlDom::MethodInfo::Method) {
            auto &functionDeclaration = type.functionDeclarations.emplace_back(
                Utils::SmallString{methodInfo.name},
                "",
                Storage::Synchronization::ParameterDeclarations{});
            addParameterDeclaration(functionDeclaration.parameters, methodInfo.parameters);
        } else {
            auto &signalDeclaration = type.signalDeclarations.emplace_back(
                Utils::SmallString{methodInfo.name});
            addParameterDeclaration(signalDeclaration.parameters, methodInfo.parameters);
        }
    }
}

Storage::Synchronization::EnumeratorDeclarations createEnumerators(const QmlDom::EnumDecl &enumeration)
{
    Storage::Synchronization::EnumeratorDeclarations enumeratorDeclarations;
    for (const QmlDom::EnumItem &enumerator : enumeration.values()) {
        enumeratorDeclarations.emplace_back(Utils::SmallString{enumerator.name()},
                                            static_cast<long long>(enumerator.value()));
    }
    return enumeratorDeclarations;
}

void addEnumeraton(Storage::Synchronization::Type &type, const QmlDom::Component &component)
{
    for (const QmlDom::EnumDecl &enumeration : component.enumerations()) {
        Storage::Synchronization::EnumeratorDeclarations enumeratorDeclarations = createEnumerators(
            enumeration);
        type.enumerationDeclarations.emplace_back(Utils::SmallString{enumeration.name()},
                                                  std::move(enumeratorDeclarations));
    }
}

bool isSingleton(const QmlDom::QmlFile *qmlFile)
{
    const auto &pragmas = qmlFile->pragmas();

    return std::ranges::find(pragmas, "Singleton"_L1, &QQmlJS::Dom::Pragma::name) != pragmas.end();
}

Storage::TypeTraits createTypeTraits(const QmlDom::QmlFile *qmlFile, IsInsideProject isInsideProject)
{
    Storage::TypeTraits traits = Storage::TypeTraitsKind::Reference;

    traits.isFileComponent = true;

    traits.isSingleton = isSingleton(qmlFile);

    traits.isInsideProject = isInsideProject == IsInsideProject::Yes;

    return traits;
}

} // namespace

Storage::Synchronization::Type QmlDocumentParser::parse(const QString &sourceContent,
                                                        Storage::Imports &imports,
                                                        SourceId sourceId,
                                                        Utils::SmallStringView directoryPath,
                                                        IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"qml document parser parse",
                               category(),
                               keyValue("sourceId", sourceId),
                               keyValue("directoryPath", directoryPath)};

    Storage::Synchronization::Type type;

    using Option = QmlDom::DomEnvironment::Option;
    using DomCreationOption = QQmlJS::Dom::DomCreationOption;

    auto environment = QmlDom::DomEnvironment::create({},
                                                      Option::SingleThreaded | Option::NoDependencies
                                                          | Option::WeakLoad,
                                                      DomCreationOption::Minimal);

    QmlDom::DomItem items;

    QString filePath{m_pathCache.sourcePath(sourceId)};

    environment->loadFile(
        QQmlJS::Dom::FileToLoad::fromMemory(environment, filePath, sourceContent),
        [&](QmlDom::Path, const QmlDom::DomItem &, const QmlDom::DomItem &newItems) {
            items = newItems;
        },
        QmlDom::DomType::QmlFile);

    environment->loadPendingDependencies();

    QmlDom::DomItem file = items.field(QmlDom::Fields::currentItem);
    const QmlDom::QmlFile *qmlFile = file.as<QmlDom::QmlFile>();
    const auto &components = qmlFile->components();
    qmlFile->pragmas();
    if (components.empty())
        return type;

    const auto &component = components.first();
    auto objects = component.objects();

    if (objects.empty())
        return type;

    QmlDom::QmlObject &qmlObject = objects.front();

    const auto qmlImports = qmlFile->imports();

    const auto importAliases = createImportAliases(qmlImports);

    type.traits = createTypeTraits(qmlFile, isInsideProject);
    type.prototype = createImportedTypeName(qmlObject.name(), importAliases);
    type.defaultPropertyName = qmlObject.localDefaultPropertyName();
    addImports(imports, qmlFile->imports(), sourceId, directoryPath, m_modulesStorage);

    addPropertyDeclarations(type, qmlObject, importAliases, file);
    addFunctionAndSignalDeclarations(type, qmlObject);
    addEnumeraton(type, component);

    return type;
}

#else

Storage::Synchronization::Type QmlDocumentParser::parse(
    [[maybe_unused]] const QString &sourceContent,
    [[maybe_unused]] Storage::Imports &imports,
    [[maybe_unused]] SourceId sourceId,
    [[maybe_unused]] Utils::SmallStringView directoryPath,
    [[maybe_unused]] IsInsideProject isInsideProject)
{
    return Storage::Synchronization::Type{};
}

#endif
} // namespace QmlDesigner

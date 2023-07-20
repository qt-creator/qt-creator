
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldocumentparser.h"

#include "projectstorage.h"
#include "projectstorageexceptions.h"
#include "sourcepathcache.h"

#include <sqlitedatabase.h>

#ifdef QDS_BUILD_QMLPARSER
#include <private/qqmldomtop_p.h>
#endif

#include <filesystem>
#include <QDateTime>

namespace QmlDesigner {

#ifdef QDS_BUILD_QMLPARSER

namespace QmlDom = QQmlJS::Dom;
namespace Synchronization = Storage::Synchronization;

namespace {

using QualifiedImports = std::map<QString, Storage::Import>;

int convertVersionNumber(qint32 versionNumber)
{
    return versionNumber < 0 ? -1 : versionNumber;
}

Storage::Version convertVersion(QmlDom::Version version)
{
    return Storage::Version{convertVersionNumber(version.majorVersion),
                                             convertVersionNumber(version.minorVersion)};
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
                             QmlDocumentParser::ProjectStorage &storage)
{
    using QmlUriKind = QQmlJS::Dom::QmlUri::Kind;

    auto &&uri = qmlImport.uri;

    if (uri.kind() == QmlUriKind::RelativePath) {
        auto path = createNormalizedPath(directoryPath, uri.localPath());
        auto moduleId = storage.moduleId(createNormalizedPath(directoryPath, uri.localPath()));
        return Storage::Import(moduleId, Storage::Version{}, sourceId);
    }

    if (uri.kind() == QmlUriKind::ModuleUri) {
        auto moduleId = storage.moduleId(Utils::PathString{uri.moduleUri()});
        return Storage::Import(moduleId, convertVersion(qmlImport.version), sourceId);
    }

    auto moduleId = storage.moduleId(Utils::PathString{uri.toString()});
    return Storage::Import(moduleId, convertVersion(qmlImport.version), sourceId);
}

QualifiedImports createQualifiedImports(const QList<QmlDom::Import> &qmlImports,
                                        SourceId sourceId,
                                        Utils::SmallStringView directoryPath,
                                        QmlDocumentParser::ProjectStorage &storage)
{
    QualifiedImports qualifiedImports;

    for (const QmlDom::Import &qmlImport : qmlImports) {
        if (!qmlImport.importId.isEmpty() && !qmlImport.implicit)
            qualifiedImports.try_emplace(qmlImport.importId,
                                         createImport(qmlImport, sourceId, directoryPath, storage));
    }

    return qualifiedImports;
}

void addImports(Storage::Imports &imports,
                const QList<QmlDom::Import> &qmlImports,
                SourceId sourceId,
                Utils::SmallStringView directoryPath,
                QmlDocumentParser::ProjectStorage &storage)
{
    int importCount = 0;
    for (const QmlDom::Import &qmlImport : qmlImports) {
        if (!qmlImport.implicit) {
            imports.push_back(createImport(qmlImport, sourceId, directoryPath, storage));
            ++importCount;
        }
    }

    auto localDirectoryModuleId = storage.moduleId(directoryPath);
    imports.emplace_back(localDirectoryModuleId, Storage::Version{}, sourceId);
    ++importCount;

    auto qmlModuleId = storage.moduleId("QML");
    imports.emplace_back(qmlModuleId, Storage::Version{}, sourceId);
    ++importCount;

    auto end = imports.end();
    auto begin = std::prev(end, importCount);

    std::sort(begin, end);
    imports.erase(std::unique(begin, end), end);
}

Synchronization::ImportedTypeName createImportedTypeName(const QStringView rawtypeName,
                                                         const QualifiedImports &qualifiedImports)
{
    auto foundDot = std::find(rawtypeName.begin(), rawtypeName.end(), '.');

    QStringView alias(rawtypeName.begin(), foundDot);

    auto foundImport = qualifiedImports.find(alias.toString());
    if (foundImport == qualifiedImports.end())
        return Synchronization::ImportedType{Utils::SmallString{rawtypeName}};

    QStringView typeName(std::next(foundDot), rawtypeName.end());

    return Storage::Synchronization::QualifiedImportedType{Utils::SmallString{typeName.toString()},
                                                           foundImport->second};
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
                                                      const QualifiedImports &qualifiedImports)
{
    auto [filteredTypeName, traits] = filteredListTypeName(rawtypeName);

    if (!filteredTypeName.contains('.'))
        return {Synchronization::ImportedType{Utils::SmallString{filteredTypeName}}, traits};

    return {createImportedTypeName(filteredTypeName, qualifiedImports), traits};
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
                             const QualifiedImports &qualifiedImports,
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
            if (resolvedAlias.valid()) {
                auto [aliasPropertyName, aliasPropertyNameTail] = createAccessPaths(
                    resolvedAlias.accessedPath);

                auto [importedTypeName, traits] = createImportedTypeNameAndTypeTraits(
                    resolvedAlias.typeName, qualifiedImports);

                type.propertyDeclarations.emplace_back(Utils::SmallString{propertyDeclaration.name},
                                                       std::move(importedTypeName),
                                                       traits,
                                                       aliasPropertyName,
                                                       aliasPropertyNameTail);
            }
        } else {
            auto [importedTypeName, traits] = createImportedTypeNameAndTypeTraits(
                propertyDeclaration.typeName, qualifiedImports);
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

} // namespace

Storage::Synchronization::Type QmlDocumentParser::parse(const QString &sourceContent,
                                                        Storage::Imports &imports,
                                                        SourceId sourceId,
                                                        Utils::SmallStringView directoryPath)
{
    Storage::Synchronization::Type type;

    using Option = QmlDom::DomEnvironment::Option;

    QmlDom::DomItem environment = QmlDom::DomEnvironment::create({},
                                                                 Option::SingleThreaded
                                                                     | Option::NoDependencies
                                                                     | Option::WeakLoad);

    QmlDom::DomItem items;

    QString filePath{m_pathCache.sourcePath(sourceId)};

    environment.loadFile(
        filePath,
        filePath,
        sourceContent,
        QDateTime{},
        [&](QmlDom::Path, const QmlDom::DomItem &, const QmlDom::DomItem &newItems) {
            items = newItems;
        },
        QmlDom::LoadOption::DefaultLoad,
        QmlDom::DomType::QmlFile);

    environment.loadPendingDependencies();

    QmlDom::DomItem file = items.field(QmlDom::Fields::currentItem);
    const QmlDom::QmlFile *qmlFile = file.as<QmlDom::QmlFile>();
    const auto &components = qmlFile->components();

    if (components.empty())
        return type;

    const auto &component = components.first();
    auto objects = component.objects();

    if (objects.empty())
        return type;

    QmlDom::QmlObject &qmlObject = objects.front();

    const auto qmlImports = qmlFile->imports();

    const auto qualifiedImports = createQualifiedImports(qmlImports,
                                                         sourceId,
                                                         directoryPath,
                                                         m_storage);

    type.prototype = createImportedTypeName(qmlObject.name(), qualifiedImports);

    addImports(imports, qmlFile->imports(), sourceId, directoryPath, m_storage);

    addPropertyDeclarations(type, qmlObject, qualifiedImports, file);
    addFunctionAndSignalDeclarations(type, qmlObject);
    addEnumeraton(type, component);

    return type;
}

#else

Storage::Synchronization::Type QmlDocumentParser::parse(
    [[maybe_unused]] const QString &sourceContent,
    [[maybe_unused]] Storage::Imports &imports,
    [[maybe_unused]] SourceId sourceId,
    [[maybe_unused]] Utils::SmallStringView directoryPath)
{
    return Storage::Synchronization::Type{};
}

#endif
} // namespace QmlDesigner

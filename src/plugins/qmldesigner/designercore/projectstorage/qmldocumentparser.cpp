
/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmldocumentparser.h"

#include "projectstorage.h"
#include "sourcepathcache.h"

#include <sqlitedatabase.h>

#include <qmldom/qqmldomtop_p.h>

#include <filesystem>
#include <QDateTime>

namespace QmlDesigner {

namespace QmlDom = QQmlJS::Dom;

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

Storage::ImportedTypeName createImportedTypeName(const QStringView rawtypeName,
                                                 const QualifiedImports &qualifiedImports)
{
    if (!rawtypeName.contains('.'))
        return Storage::ImportedType{Utils::SmallString{rawtypeName}};

    auto foundDot = std::find(rawtypeName.begin(), rawtypeName.end(), '.');

    QStringView alias(rawtypeName.begin(), foundDot);

    auto foundImport = qualifiedImports.find(alias.toString());
    if (foundImport == qualifiedImports.end())
        return Storage::ImportedType{Utils::SmallString{rawtypeName}};

    QStringView typeName(std::next(foundDot), rawtypeName.end());

    return Storage::QualifiedImportedType{Utils::SmallString{typeName.toString()},
                                          foundImport->second};
}

void addPropertyDeclarations(Storage::Type &type,
                             const QmlDom::QmlObject &rootObject,
                             const QualifiedImports &qualifiedImports)
{
    for (const QmlDom::PropertyDefinition &propertyDeclaration : rootObject.propertyDefs()) {
        type.propertyDeclarations.emplace_back(Utils::SmallString{propertyDeclaration.name},
                                               createImportedTypeName(propertyDeclaration.typeName,
                                                                      qualifiedImports),
                                               Storage::PropertyDeclarationTraits::None);
    }
}

void addParameterDeclaration(Storage::ParameterDeclarations &parameterDeclarations,
                             const QList<QmlDom::MethodParameter> &parameters)
{
    for (const QmlDom::MethodParameter &parameter : parameters) {
        parameterDeclarations.emplace_back(Utils::SmallString{parameter.name},
                                           Utils::SmallString{parameter.typeName});
    }
}

void addFunctionAndSignalDeclarations(Storage::Type &type, const QmlDom::QmlObject &rootObject)
{
    for (const QmlDom::MethodInfo &methodInfo : rootObject.methods()) {
        if (methodInfo.methodType == QmlDom::MethodInfo::Method) {
            auto &functionDeclaration = type.functionDeclarations.emplace_back(
                Utils::SmallString{methodInfo.name}, "", Storage::ParameterDeclarations{});
            addParameterDeclaration(functionDeclaration.parameters, methodInfo.parameters);
        } else {
            auto &signalDeclaration = type.signalDeclarations.emplace_back(
                Utils::SmallString{methodInfo.name});
            addParameterDeclaration(signalDeclaration.parameters, methodInfo.parameters);
        }
    }
}

Storage::EnumeratorDeclarations createEnumerators(const QmlDom::EnumDecl &enumeration)
{
    Storage::EnumeratorDeclarations enumeratorDeclarations;
    for (const QmlDom::EnumItem &enumerator : enumeration.values()) {
        enumeratorDeclarations.emplace_back(Utils::SmallString{enumerator.name()},
                                            static_cast<long long>(enumerator.value()));
    }
    return enumeratorDeclarations;
}

void addEnumeraton(Storage::Type &type, const QmlDom::Component &component)
{
    for (const QmlDom::EnumDecl &enumeration : component.enumerations()) {
        Storage::EnumeratorDeclarations enumeratorDeclarations = createEnumerators(enumeration);
        type.enumerationDeclarations.emplace_back(Utils::SmallString{enumeration.name()},
                                                  std::move(enumeratorDeclarations));
    }
}

} // namespace

Storage::Type QmlDocumentParser::parse(const QString &sourceContent,
                                       Storage::Imports &imports,
                                       SourceId sourceId,
                                       Utils::SmallStringView directoryPath)
{
    Storage::Type type;

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
    const auto &objects = component.objects();

    if (objects.empty())
        return type;

    const QmlDom::QmlObject &qmlObject = objects.front();

    const auto qmlImports = qmlFile->imports();

    const auto qualifiedImports = createQualifiedImports(qmlImports,
                                                         sourceId,
                                                         directoryPath,
                                                         m_storage);

    type.prototype = createImportedTypeName(qmlObject.name(), qualifiedImports);

    addImports(imports, qmlFile->imports(), sourceId, directoryPath, m_storage);

    addPropertyDeclarations(type, qmlObject, qualifiedImports);
    addFunctionAndSignalDeclarations(type, qmlObject);
    addEnumeraton(type, component);

    return type;
}

} // namespace QmlDesigner

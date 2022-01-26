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

int convertVersionNumber(qint32 versionNumber)
{
    return versionNumber < 0 ? -1 : versionNumber;
}

Storage::Version convertVersion(QmlDom::Version version)
{
    return Storage::Version{convertVersionNumber(version.majorVersion),
                            convertVersionNumber(version.minorVersion)};
}

Utils::PathString convertUri(const QString &uri)
{
    QStringView localPath{uri.begin() + 7, uri.end()};

    std::filesystem::path path{
        std::u16string_view{localPath.utf16(), static_cast<std::size_t>(localPath.size())}};

    auto x = std::filesystem::weakly_canonical(path);

    return Utils::PathString{x.generic_string()};
}

void addImports(Storage::Imports &imports,
                const QList<QmlDom::Import> &qmlImports,
                SourceId sourceId,
                Utils::SmallStringView directoryPath,
                QmlDocumentParser::ProjectStorage &storage)
{
    for (const QmlDom::Import &qmlImport : qmlImports) {
        if (qmlImport.uri == u"file://.") {
            auto moduleId = storage.moduleId(directoryPath);
            imports.emplace_back(moduleId, Storage::Version{}, sourceId);
        } else if (qmlImport.uri.startsWith(u"file://")) {
            auto x = convertUri(qmlImport.uri);
            auto moduleId = storage.moduleId(convertUri(qmlImport.uri));
            imports.emplace_back(moduleId, Storage::Version{}, sourceId);
        } else {
            auto moduleId = storage.moduleId(Utils::SmallString{qmlImport.uri});
            imports.emplace_back(moduleId, convertVersion(qmlImport.version), sourceId);
        }
    }
}

void addPropertyDeclarations(Storage::Type &type, const QmlDom::QmlObject &rootObject)
{
    for (const QmlDom::PropertyDefinition &propertyDeclaration : rootObject.propertyDefs()) {
        type.propertyDeclarations.emplace_back(Utils::SmallString{propertyDeclaration.name},
                                               Storage::ImportedType{
                                                   Utils::SmallString{propertyDeclaration.typeName}},
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
                                       SourceId sourceId)
{
    Storage::Type type;

    QmlDom::DomItem environment = QmlDom::DomEnvironment::create(
        {},
        QmlDom::DomEnvironment::Option::SingleThreaded
            | QmlDom::DomEnvironment::Option::NoDependencies);

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

    type.prototype = Storage::ImportedType{Utils::SmallString{qmlObject.name()}};

    auto directoryPath{m_pathCache.sourceContextPath(m_pathCache.sourceContextId(sourceId))};

    addImports(imports, qmlFile->imports(), sourceId, directoryPath, m_storage);

    addPropertyDeclarations(type, qmlObject);
    addFunctionAndSignalDeclarations(type, qmlObject);
    addEnumeraton(type, component);

    return type;
}

} // namespace QmlDesigner

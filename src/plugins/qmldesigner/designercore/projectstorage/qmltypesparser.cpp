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

#include "qmltypesparser.h"

#include "projectstorage.h"
#include "sourcepathcache.h"

#include <sqlitedatabase.h>

#include <qmlcompiler/qqmljstypedescriptionreader_p.h>
#include <qmldom/qqmldomtop_p.h>

#include <QDateTime>

#include <algorithm>
#include <tuple>

namespace QmlDesigner {

namespace QmlDom = QQmlJS::Dom;

namespace {

using ComponentWithoutNamespaces = QMap<QString, QString>;

ComponentWithoutNamespaces createComponentNameWithoutNamespaces(
    const QHash<QString, QQmlJSScope::Ptr> &objects)
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

void appendImports(Storage::Imports &imports,
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

    imports.emplace_back(cppModuleId, Storage::Version{}, sourceId);
}

void addImports(Storage::Imports &imports,
                SourceId sourceId,
                const QStringList &dependencies,
                QmlTypesParser::ProjectStorage &storage,
                ModuleId cppModuleId)
{
    for (const QString &dependency : dependencies)
        appendImports(imports, dependency, sourceId, storage);

    imports.emplace_back(cppModuleId, Storage::Version{}, sourceId);

    if (ModuleId qmlCppModuleId = storage.moduleId("QML-cppnative"); cppModuleId != qmlCppModuleId)
        imports.emplace_back(qmlCppModuleId, Storage::Version{}, sourceId);
}

Storage::TypeAccessSemantics createTypeAccessSemantics(QQmlJSScope::AccessSemantics accessSematics)
{
    switch (accessSematics) {
    case QQmlJSScope::AccessSemantics::Reference:
        return Storage::TypeAccessSemantics::Reference;
    case QQmlJSScope::AccessSemantics::Value:
        return Storage::TypeAccessSemantics::Value;
    case QQmlJSScope::AccessSemantics::None:
        return Storage::TypeAccessSemantics::None;
    case QQmlJSScope::AccessSemantics::Sequence:
        return Storage::TypeAccessSemantics::Sequence;
    }

    return Storage::TypeAccessSemantics::None;
}

Storage::Version createVersion(QTypeRevision qmlVersion)
{
    return Storage::Version{qmlVersion.majorVersion(), qmlVersion.minorVersion()};
}

Storage::ExportedTypes createExports(const QList<QQmlJSScope::Export> &qmlExports,
                                     Utils::SmallStringView interanalName,
                                     QmlTypesParser::ProjectStorage &storage,
                                     ModuleId cppModuleId)
{
    Storage::ExportedTypes exportedTypes;
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

Storage::ExportedTypes createCppEnumerationExports(Utils::SmallStringView typeName,
                                                   ModuleId cppModuleId,
                                                   Utils::SmallStringView enumerationName,
                                                   Utils::SmallStringView enumerationAlias)
{
    Storage::ExportedTypes exportedTypes;

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

Storage::PropertyDeclarationTraits createPropertyDeclarationTraits(const QQmlJSMetaProperty &qmlProperty)
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

Storage::PropertyDeclarations createProperties(
    const QHash<QString, QQmlJSMetaProperty> &qmlProperties,
    const EnumerationTypes &enumerationTypes,
    const ComponentWithoutNamespaces &componentNameWithoutNamespace)
{
    Storage::PropertyDeclarations propertyDeclarations;
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
                                          Storage::ImportedType{propertyTypeName},
                                          createPropertyDeclarationTraits(qmlProperty));
    }

    return propertyDeclarations;
}

Storage::ParameterDeclarations createParameters(
    const QQmlJSMetaMethod &qmlMethod, const ComponentWithoutNamespaces &componentNameWithoutNamespace)
{
    Storage::ParameterDeclarations parameterDeclarations;

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

std::tuple<Storage::FunctionDeclarations, Storage::SignalDeclarations> createFunctionAndSignals(
    const QMultiHash<QString, QQmlJSMetaMethod> &qmlMethods,
    const ComponentWithoutNamespaces &componentNameWithoutNamespace)
{
    std::tuple<Storage::FunctionDeclarations, Storage::SignalDeclarations> functionAndSignalDeclarations;
    Storage::FunctionDeclarations &functionsDeclarations{std::get<0>(functionAndSignalDeclarations)};
    functionsDeclarations.reserve(Utils::usize(qmlMethods));
    Storage::SignalDeclarations &signalDeclarations{std::get<1>(functionAndSignalDeclarations)};
    signalDeclarations.reserve(Utils::usize(qmlMethods));

    for (const QQmlJSMetaMethod &qmlMethod : qmlMethods) {
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

Storage::EnumeratorDeclarations createEnumeratorsWithValues(const QQmlJSMetaEnum &qmlEnumeration)
{
    Storage::EnumeratorDeclarations enumeratorDeclarations;

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

Storage::EnumeratorDeclarations createEnumeratorsWithoutValues(const QQmlJSMetaEnum &qmlEnumeration)
{
    Storage::EnumeratorDeclarations enumeratorDeclarations;

    for (const QString &key : qmlEnumeration.keys())
        enumeratorDeclarations.emplace_back(Utils::SmallString{key});

    return enumeratorDeclarations;
}

Storage::EnumeratorDeclarations createEnumerators(const QQmlJSMetaEnum &qmlEnumeration)
{
    if (qmlEnumeration.hasValues())
        return createEnumeratorsWithValues(qmlEnumeration);

    return createEnumeratorsWithoutValues(qmlEnumeration);
}

Storage::EnumerationDeclarations createEnumeration(const QHash<QString, QQmlJSMetaEnum> &qmlEnumerations)
{
    Storage::EnumerationDeclarations enumerationDeclarations;
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
                        Storage::Types &types,
                        Utils::SmallStringView typeName,
                        Utils::SmallStringView enumerationName,
                        SourceId sourceId,
                        ModuleId cppModuleId,
                        Utils::SmallStringView enumerationAlias)
{
    auto fullTypeName = addEnumerationType(enumerationTypes, typeName, enumerationName);
    types.emplace_back(fullTypeName,
                       Storage::ImportedType{TypeNameString{}},
                       Storage::TypeAccessSemantics::Value | Storage::TypeAccessSemantics::IsEnum,
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

EnumerationTypes addEnumerationTypes(Storage::Types &types,
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

void addType(Storage::Types &types,
             SourceId sourceId,
             ModuleId cppModuleId,
             const QQmlJSScope &component,
             QmlTypesParser::ProjectStorage &storage,
             const ComponentWithoutNamespaces &componentNameWithoutNamespace)
{
    auto [functionsDeclarations, signalDeclarations] = createFunctionAndSignals(
        component.ownMethods(), componentNameWithoutNamespace);
    TypeNameString typeName{component.internalName()};
    auto enumerations = component.ownEnumerations();
    auto exports = component.exports();

    auto enumerationTypes = addEnumerationTypes(types, typeName, sourceId, cppModuleId, enumerations);
    types.emplace_back(Utils::SmallStringView{typeName},
                       Storage::ImportedType{TypeNameString{component.baseTypeName()}},
                       createTypeAccessSemantics(component.accessSemantics()),
                       sourceId,
                       createExports(exports, typeName, storage, cppModuleId),
                       createProperties(component.ownProperties(),
                                        enumerationTypes,
                                        componentNameWithoutNamespace),
                       std::move(functionsDeclarations),
                       std::move(signalDeclarations),
                       createEnumeration(enumerations));
}

void addTypes(Storage::Types &types,
              const Storage::ProjectData &projectData,
              const QHash<QString, QQmlJSScope::Ptr> &objects,
              QmlTypesParser::ProjectStorage &storage,
              const ComponentWithoutNamespaces &componentNameWithoutNamespaces)
{
    types.reserve(Utils::usize(objects) + types.size());

    for (const auto &object : objects)
        addType(types,
                projectData.sourceId,
                projectData.moduleId,
                *object.get(),
                storage,
                componentNameWithoutNamespaces);
}

} // namespace

void QmlTypesParser::parse(const QString &sourceContent,
                           Storage::Imports &imports,
                           Storage::Types &types,
                           const Storage::ProjectData &projectData)
{
    QQmlJSTypeDescriptionReader reader({}, sourceContent);
    QHash<QString, QQmlJSScope::Ptr> components;
    QStringList dependencies;
    bool isValid = reader(&components, &dependencies);
    if (!isValid)
        throw CannotParseQmlTypesFile{};

    auto componentNameWithoutNamespaces = createComponentNameWithoutNamespaces(components);

    addImports(imports, projectData.sourceId, dependencies, m_storage, projectData.moduleId);
    addTypes(types, projectData, components, m_storage, componentNameWithoutNamespaces);
}

} // namespace QmlDesigner

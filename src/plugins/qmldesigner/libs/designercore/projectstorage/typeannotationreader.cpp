// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "typeannotationreader.h"

#include "projectstorage.h"

#include <invalidmetainfoexception.h>

#include <utils/algorithm.h>

#include <QDebug>
#include <QFileInfo>
#include <QIcon>
#include <QString>

namespace QmlDesigner::Storage {
using namespace Qt::StringLiterals;

namespace {
constexpr auto rootElementName = "MetaInfo"_L1;
constexpr auto typeElementName = "Type"_L1;
constexpr auto itemLibraryEntryElementName = "ItemLibraryEntry"_L1;
constexpr auto hintsElementName = "Hints"_L1;
constexpr auto qmlSourceElementNamentName = "QmlSource"_L1;
constexpr auto propertyElementName = "Property"_L1;
constexpr auto extraFileElementName = "ExtraFile"_L1;
} // namespace

Synchronization::TypeAnnotations TypeAnnotationReader::parseTypeAnnotation(
    const QString &content, const QString &directoryPath, SourceId sourceId, SourceId directorySourceId)
{
    m_sourceId = sourceId;
    m_directorySourceId = directorySourceId;
    m_directoryPath = directoryPath;
    m_parserState = ParsingDocument;
    if (!SimpleAbstractStreamReader::readFromSource(content)) {
        m_parserState = Error;
        throw TypeAnnoationParsingError(errors());
    }

    if (!errors().isEmpty()) {
        m_parserState = Error;
        throw TypeAnnoationParsingError(errors());
    }

    return takeTypeAnnotations();
}

QStringList TypeAnnotationReader::errors()
{
    return QmlJS::SimpleAbstractStreamReader::errors();
}

void TypeAnnotationReader::elementStart(const QString &name,
                                        [[maybe_unused]] const QmlJS::SourceLocation &nameLocation)
{
    switch (parserState()) {
    case ParsingDocument:
        setParserState(readDocument(name));
        break;
    case ParsingMetaInfo:
        setParserState(readMetaInfoRootElement(name));
        break;
    case ParsingType:
        setParserState(readTypeElement(name));
        break;
    case ParsingItemLibrary:
        setParserState(readItemLibraryEntryElement(name));
        break;
    case ParsingProperty:
        setParserState(readPropertyElement(name));
        break;
    case ParsingQmlSource:
        setParserState(readQmlSourceElement(name));
        break;
    case ParsingExtraFile:
        setParserState(readExtraFileElement(name));
        break;
    case ParsingHints:
    case Finished:
    case Undefined:
        setParserState(Error);
        addError(TypeAnnotationReader::tr("Illegal state while parsing."), currentSourceLocation());
        [[fallthrough]];
    case Error:
        break;
    }
}

void TypeAnnotationReader::elementEnd()
{
    switch (parserState()) {
    case ParsingMetaInfo:
        setParserState(Finished);
        break;
    case ParsingType:
        if (m_itemLibraryEntries.size())
            m_typeAnnotations.back().itemLibraryJson = to_string(m_itemLibraryEntries);
        setParserState(ParsingMetaInfo);
        break;
    case ParsingItemLibrary:
        setParserState((ParsingType));
        break;
    case ParsingHints:
        addHints();
        setParserState(ParsingType);
        break;
    case ParsingProperty:
        insertProperty();
        setParserState(ParsingItemLibrary);
        break;
    case ParsingQmlSource:
        setParserState(ParsingItemLibrary);
        break;
    case ParsingExtraFile:
        setParserState(ParsingItemLibrary);
        break;
    case ParsingDocument:
    case Finished:
    case Undefined:
        setParserState(Error);
        addError(TypeAnnotationReader::tr("Illegal state while parsing."), currentSourceLocation());
        [[fallthrough]];
    case Error:
        break;
    }
}

void TypeAnnotationReader::propertyDefinition(const QString &name,
                                              [[maybe_unused]] const QmlJS::SourceLocation &nameLocation,
                                              const QVariant &value,
                                              [[maybe_unused]] const QmlJS::SourceLocation &valueLocation)
{
    switch (parserState()) {
    case ParsingType:
        readTypeProperty(name, value);
        break;
    case ParsingItemLibrary:
        readItemLibraryEntryProperty(name, value);
        break;
    case ParsingProperty:
        readPropertyProperty(name, value);
        break;
    case ParsingQmlSource:
        readQmlSourceProperty(name, value);
        break;
    case ParsingExtraFile:
        readExtraFileProperty(name, value);
        break;
    case ParsingMetaInfo:
        addError(TypeAnnotationReader::tr("No property definition allowed."), currentSourceLocation());
        break;
    case ParsingDocument:
    case ParsingHints:
        readHint(name, value);
        break;
    case Finished:
    case Undefined:
        setParserState(Error);
        addError(TypeAnnotationReader::tr("Illegal state while parsing."), currentSourceLocation());
        [[fallthrough]];
    case Error:
        break;
    }
}

TypeAnnotationReader::ParserSate TypeAnnotationReader::readDocument(const QString &name)
{
    if (name == rootElementName) {
        return ParsingMetaInfo;
    } else {
        addErrorInvalidType(name);
        return Error;
    }
}

TypeAnnotationReader::ParserSate TypeAnnotationReader::readMetaInfoRootElement(const QString &name)
{
    if (name == typeElementName) {
        auto &annotation = m_typeAnnotations.emplace_back(m_sourceId, m_directorySourceId);
        annotation.traits.canBeDroppedInFormEditor = FlagIs::True;
        annotation.traits.canBeDroppedInNavigator = FlagIs::True;
        annotation.traits.isMovable = FlagIs::True;
        annotation.traits.isResizable = FlagIs::True;
        annotation.traits.hasFormEditorItem = FlagIs::True;
        annotation.traits.visibleInLibrary = FlagIs::True;
        m_itemLibraryEntries = json::array();

        return ParsingType;
    } else {
        addErrorInvalidType(name);
        return Error;
    }
}

TypeAnnotationReader::ParserSate TypeAnnotationReader::readTypeElement(const QString &name)
{
    if (name == itemLibraryEntryElementName) {
        m_itemLibraryEntries.push_back({});

        return ParsingItemLibrary;
    } else if (name == hintsElementName) {
        return ParsingHints;
    } else {
        addErrorInvalidType(name);
        return Error;
    }
}

TypeAnnotationReader::ParserSate TypeAnnotationReader::readItemLibraryEntryElement(const QString &name)
{
    if (name == qmlSourceElementNamentName) {
        return ParsingQmlSource;
    } else if (name == propertyElementName) {
        m_currentProperty = {};
        return ParsingProperty;
    } else if (name == extraFileElementName) {
        return ParsingExtraFile;
    } else {
        addError(TypeAnnotationReader::tr("Invalid type %1").arg(name), currentSourceLocation());
        return Error;
    }
}

TypeAnnotationReader::ParserSate TypeAnnotationReader::readPropertyElement(const QString &name)
{
    addError(TypeAnnotationReader::tr("Invalid type %1").arg(name), currentSourceLocation());
    return Error;
}

TypeAnnotationReader::ParserSate TypeAnnotationReader::readQmlSourceElement(const QString &name)
{
    addError(TypeAnnotationReader::tr("Invalid type %1").arg(name), currentSourceLocation());
    return Error;
}

TypeAnnotationReader::ParserSate TypeAnnotationReader::readExtraFileElement(const QString &name)
{
    addError(TypeAnnotationReader::tr("Invalid type %1").arg(name), currentSourceLocation());
    return Error;
}

namespace {
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wunneeded-internal-declaration")

std::pair<Utils::SmallStringView, Utils::SmallStringView> decomposeTypePath(Utils::SmallStringView typeName)
{
    auto found = std::find(typeName.rbegin(), typeName.rend(), '.');

    if (found == typeName.rend())
        return {{}, typeName};

    return {{typeName.begin(), std::prev(found.base())}, {found.base(), typeName.end()}};
}

QT_WARNING_POP
} // namespace

void TypeAnnotationReader::readTypeProperty(QStringView name, const QVariant &value)
{
    if (name == "name"_L1) {
        Utils::PathString fullTypeName = value.toString();
        auto [moduleName, typeName] = decomposeTypePath(fullTypeName);

        m_typeAnnotations.back().typeName = typeName;
        m_typeAnnotations.back().moduleId = m_projectStorage.moduleId(moduleName,
                                                                      ModuleKind::QmlLibrary);

    } else if (name == "icon"_L1) {
        m_typeAnnotations.back().iconPath = absoluteFilePathForDocument(value.toString());
    } else {
        addError(TypeAnnotationReader::tr("Unknown property for Type %1").arg(name),
                 currentSourceLocation());
        setParserState(Error);
    }
}

void TypeAnnotationReader::readItemLibraryEntryProperty(QStringView name, const QVariant &variant)
{
    auto value = variant.toString().toStdString();
    if (name == "name"_L1) {
        m_itemLibraryEntries.back()["name"] = value;
    } else if (name == "category"_L1) {
        m_itemLibraryEntries.back()["category"] = value;
    } else if (name == "libraryIcon"_L1) {
        m_itemLibraryEntries.back()["iconPath"] = absoluteFilePathForDocument(variant.toString());
    } else if (name == "version"_L1) {
        //   setVersion(value.toString());
    } else if (name == "requiredImport"_L1) {
        m_itemLibraryEntries.back()["import"] = value;
    } else if (name == "toolTip"_L1) {
        m_itemLibraryEntries.back()["toolTip"] = value;
    } else {
        addError(TypeAnnotationReader::tr("Unknown property for ItemLibraryEntry %1").arg(name),
                 currentSourceLocation());
        setParserState(Error);
    }
}

namespace {
QString deEscape(const QString &value)
{
    QString result = value;

    result.replace(R"(\")"_L1, R"(")"_L1);
    result.replace(R"(\\")"_L1, R"(\)"_L1);

    return result;
}

QVariant deEscapeVariant(const QVariant &value)
{
    if (value.typeId() == QMetaType::QString)
        return deEscape(value.toString());
    return value;
}
} // namespace

void TypeAnnotationReader::readPropertyProperty(QStringView name, const QVariant &value)
{
    if (name == "name"_L1) {
        m_currentProperty.name = value.toByteArray();
    } else if (name == "type"_L1) {
        m_currentProperty.type = value.toString();
    } else if (name == "value"_L1) {
        m_currentProperty.value = deEscapeVariant(value);
    } else {
        addError(TypeAnnotationReader::tr("Unknown property for Property %1").arg(name),
                 currentSourceLocation());
        setParserState(Error);
    }
}

void TypeAnnotationReader::readQmlSourceProperty(QStringView name, const QVariant &value)
{
    if (name == "source"_L1) {
        m_itemLibraryEntries.back()["templatePath"] = absoluteFilePathForDocument(value.toString());
    } else {
        addError(TypeAnnotationReader::tr("Unknown property for QmlSource %1").arg(name),
                 currentSourceLocation());
        setParserState(Error);
    }
}

void TypeAnnotationReader::readExtraFileProperty(QStringView name, const QVariant &value)
{
    if (name == "source"_L1) {
        m_itemLibraryEntries.back()["extraFilePaths"].push_back(
            absoluteFilePathForDocument(value.toString()));
    } else {
        addError(TypeAnnotationReader::tr("Unknown property for ExtraFile %1").arg(name),
                 currentSourceLocation());
        setParserState(Error);
    }
}

namespace {
FlagIs createFlag(QStringView expression)
{
    using namespace Qt::StringLiterals;
    if (expression == "true"_L1)
        return FlagIs::True;

    if (expression == "false"_L1)
        return FlagIs::False;

    return FlagIs::Set;
}

template<typename... Entries>
void setTrait(QStringView name, FlagIs flag, Storage::TypeTraits &traits)
{
    using namespace Qt::StringLiterals;

    if (name == "canBeContainer"_L1) {
        traits.canBeContainer = flag;
    } else if (name == "forceClip"_L1) {
        traits.forceClip = flag;
    } else if (name == "doesLayoutChildren"_L1) {
        traits.doesLayoutChildren = flag;
    } else if (name == "canBeDroppedInFormEditor"_L1) {
        traits.canBeDroppedInFormEditor = flag;
    } else if (name == "canBeDroppedInNavigator"_L1) {
        traits.canBeDroppedInNavigator = flag;
    } else if (name == "canBeDroppedInView3D"_L1) {
        traits.canBeDroppedInView3D = flag;
    } else if (name == "isMovable"_L1) {
        traits.isMovable = flag;
    } else if (name == "isResizable"_L1) {
        traits.isResizable = flag;
    } else if (name == "hasFormEditorItem"_L1) {
        traits.hasFormEditorItem = flag;
    } else if (name == "isStackedContainer"_L1) {
        traits.isStackedContainer = flag;
    } else if (name == "takesOverRenderingOfChildren"_L1) {
        traits.takesOverRenderingOfChildren = flag;
    } else if (name == "visibleInNavigator"_L1) {
        traits.visibleInNavigator = flag;
    } else if (name == "visibleInLibrary"_L1) {
        traits.visibleInLibrary = flag;
    } else if (name == "hideInNavigator"_L1) {
        traits.hideInNavigator = flag;
    }
}

void setComplexHint(nlohmann::json &hints, const QString &name, const QString &expression)
{
    hints[name.toStdString()] = expression.toStdString();
}

} // namespace

void TypeAnnotationReader::readHint(const QString &name, const QVariant &value)
{
    auto expression = value.toString();

    auto flag = createFlag(expression);
    setTrait(name, flag, m_typeAnnotations.back().traits);
    if (flag == FlagIs::Set)
        setComplexHint(m_hints, name, expression);
}

void TypeAnnotationReader::addHints()
{
    if (m_hints.size()) {
        m_typeAnnotations.back().hintsJson = to_string(m_hints);
        m_hints.clear();
    }
}

void TypeAnnotationReader::setVersion(const QString &versionNumber)
{
    //    const TypeName typeName = m_currentEntry.typeName();
    int major = 1;
    int minor = 0;

    if (!versionNumber.isEmpty()) {
        int val = -1;
        bool ok = false;
        if (versionNumber.contains('.'_L1)) {
            val = versionNumber.split('.'_L1).constFirst().toInt(&ok);
            major = ok ? val : major;
            val = versionNumber.split('.'_L1).constLast().toInt(&ok);
            minor = ok ? val : minor;
        } else {
            val = versionNumber.toInt(&ok);
            major = ok ? val : major;
        }
    }
    // m_currentEntry.setType(typeName, major, minor);
}

TypeAnnotationReader::ParserSate TypeAnnotationReader::parserState() const
{
    return m_parserState;
}

void TypeAnnotationReader::setParserState(ParserSate newParserState)
{
    m_parserState = newParserState;
}

using json = nlohmann::json;

[[maybe_unused]] static void to_json(json &out, const TypeAnnotationReader::Property &property)
{
    out = json::array({});
    out.push_back(property.name);
    out.push_back(property.type);
    if (property.value.typeId() == QMetaType::QString)
        out.push_back(Utils::PathString{property.value.toString()});
    else if (property.value.typeId() == QMetaType::Int || property.value.typeId() == QMetaType::LongLong)
        out.push_back(property.value.toLongLong());
    else
        out.push_back(property.value.toDouble());
}

void TypeAnnotationReader::insertProperty()
{
    m_itemLibraryEntries.back()["properties"].push_back(m_currentProperty);
}

void TypeAnnotationReader::addErrorInvalidType(const QString &typeName)
{
    addError(TypeAnnotationReader::tr("Invalid type %1").arg(typeName), currentSourceLocation());
}

Utils::PathString TypeAnnotationReader::absoluteFilePathForDocument(Utils::PathString relativeFilePath)
{
    return Utils::PathString::join({m_directoryPath, "/", relativeFilePath});
}

const char *TypeAnnoationParsingError::what() const noexcept
{
    return "TypeAnnoationParsingError";
}

} // namespace QmlDesigner::Storage

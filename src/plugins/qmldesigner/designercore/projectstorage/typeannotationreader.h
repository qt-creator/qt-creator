// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"

#include "projectstoragetypes.h"

#include <modelfwd.h>

#include <3rdparty/json/json.hpp>
#include <utils/smallstring.h>
#include <qmljs/qmljssimplereader.h>

#include <QCoreApplication>
#include <QString>
#include <QVariant>

#include <exception>
#include <variant>

namespace QmlDesigner::Storage {

class ItemLibraryEntry;

class TypeAnnoationParsingError : public std::exception
{
public:
    TypeAnnoationParsingError(QStringList errors)
        : errors{std::move(errors)}
    {}

    const char *what() const noexcept override;

    QStringList errors;
};

class TypeAnnotationReader : protected QmlJS::SimpleAbstractStreamReader
{
    Q_DECLARE_TR_FUNCTIONS(QmlDesigner::Internal::TypeAnnotationReader)

    using json = nlohmann::json;

public:
    TypeAnnotationReader(ProjectStorageType &projectStorage)
        : m_projectStorage{projectStorage}
    {}

    Synchronization::TypeAnnotations parseTypeAnnotation(const QString &content,
                                                         const QString &directoryPath,
                                                         SourceId sourceId,
                                                         SourceId directorySourceId);

    QStringList errors();

    void setQualifcation(const TypeName &qualification);

    struct Property
    {
        Utils::SmallString name;
        Utils::SmallString type;
        QVariant value;
    };

protected:
    Synchronization::TypeAnnotations takeTypeAnnotations() { return std::move(m_typeAnnotations); }

    void elementStart(const QString &name, const QmlJS::SourceLocation &nameLocation) override;
    void elementEnd() override;
    void propertyDefinition(const QString &name,
                            const QmlJS::SourceLocation &nameLocation,
                            const QVariant &value,
                            const QmlJS::SourceLocation &valueLocation) override;

private:
    enum ParserSate {
        Error,
        Finished,
        Undefined,
        ParsingDocument,
        ParsingMetaInfo,
        ParsingType,
        ParsingItemLibrary,
        ParsingHints,
        ParsingProperty,
        ParsingQmlSource,
        ParsingExtraFile
    };

    ParserSate readDocument(const QString &name);

    ParserSate readMetaInfoRootElement(const QString &name);
    ParserSate readTypeElement(const QString &name);
    ParserSate readItemLibraryEntryElement(const QString &name);
    ParserSate readPropertyElement(const QString &name);
    ParserSate readQmlSourceElement(const QString &name);
    ParserSate readExtraFileElement(const QString &name);

    void readTypeProperty(QStringView name, const QVariant &value);
    void readItemLibraryEntryProperty(QStringView name, const QVariant &value);
    void readPropertyProperty(QStringView name, const QVariant &value);
    void readQmlSourceProperty(QStringView name, const QVariant &value);
    void readExtraFileProperty(QStringView name, const QVariant &value);
    void readHint(const QString &name, const QVariant &value);
    void addHints();

    void setVersion(const QString &versionNumber);

    ParserSate parserState() const;
    void setParserState(ParserSate newParserState);

    void insertProperty();

    void addErrorInvalidType(const QString &typeName);

    Utils::PathString absoluteFilePathForDocument(Utils::PathString relativeFilePath);

private:
    ProjectStorageType &m_projectStorage;
    Utils::PathString m_directoryPath;
    ParserSate m_parserState = Undefined;
    Synchronization::TypeAnnotations m_typeAnnotations;
    json m_hints;
    json m_itemLibraryEntries;
    Property m_currentProperty;
    SourceId m_sourceId;
    SourceId m_directorySourceId;
};

} // namespace QmlDesigner::Storage

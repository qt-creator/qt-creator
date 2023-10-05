// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"
#include <metainfo.h>

#include <qmljs/qmljssimplereader.h>

#include <QCoreApplication>
#include <QString>


namespace QmlDesigner {

class ItemLibraryEntry;

namespace Internal {

class MetaInfoReader : protected QmlJS::SimpleAbstractStreamReader
{
    Q_DECLARE_TR_FUNCTIONS(QmlDesigner::Internal::MetaInfoReader)

public:
    MetaInfoReader(const MetaInfo &metaInfo);

    void readMetaInfoFile(const QString &path, bool overwriteDuplicates = false);


    QStringList errors();

    void setQualifcation(const TypeName &qualification);

protected:
    void elementStart(const QString &name, const QmlJS::SourceLocation &nameLocation) override;
    void elementEnd() override;
    void propertyDefinition(const QString &name,
                            const QmlJS::SourceLocation &nameLocation,
                            const QVariant &value,
                            const QmlJS::SourceLocation &valueLocation) override;

private:
    enum ParserSate { Error,
                      Finished,
                      Undefined,
                      ParsingDocument,
                      ParsingMetaInfo,
                      ParsingType,
                      ParsingImports,
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

    void readTypeProperty(const QString &name, const QVariant &value);
    void readItemLibraryEntryProperty(const QString &name, const QVariant &value);
    void readPropertyProperty(const QString &name, const QVariant &value);
    void readQmlSourceProperty(const QString &name, const QVariant &value);
    void readExtraFileProperty(const QString &name, const QVariant &value);
    void readHint(const QString &name, const QVariant &value);

    void setVersion(const QString &versionNumber);

    ParserSate parserState() const;
    void setParserState(ParserSate newParserState);

    void syncItemLibraryEntries();
    void keepCurrentItemLibraryEntry();
    void insertProperty();

    void addErrorInvalidType(const QString &typeName);

    QString absoluteFilePathForDocument(const QString &relativeFilePath);

    QString m_documentPath;
    ParserSate m_parserState;
    MetaInfo m_metaInfo;

    TypeName m_currentClassName;
    QString m_currentIcon;
    QHash<QString, QString> m_currentHints;
    QString m_currentSource;
    ItemLibraryEntry m_currentEntry;
    QList<ItemLibraryEntry> m_bufferedEntries;

    PropertyName m_currentPropertyName;
    QString m_currentPropertyType;
    QVariant m_currentPropertyValue;

    bool m_overwriteDuplicates;

    TypeName m_qualication;
};

}
}

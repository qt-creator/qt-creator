/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef METAINFOREADER_H
#define METAINFOREADER_H

#include "qmldesignercorelib_global.h"
#include <metainfo.h>

#include <qmljs/qmljssimplereader.h>

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
    virtual void elementStart(const QString &name);
    virtual void elementEnd();
    virtual void propertyDefinition(const QString &name, const QVariant &value);

private:
    enum ParserSate { Error,
                      Finished,
                      Undefined,
                      ParsingDocument,
                      ParsingMetaInfo,
                      ParsingType,
                      ParsingItemLibrary,
                      ParsingProperty,
                      ParsingQmlSource
                    };

    ParserSate readDocument(const QString &name);

    ParserSate readMetaInfoRootElement(const QString &name);
    ParserSate readTypeElement(const QString &name);
    ParserSate readItemLibraryEntryElement(const QString &name);
    ParserSate readPropertyElement(const QString &name);
    ParserSate readQmlSourceElement(const QString &name);

    void readTypeProperty(const QString &name, const QVariant &value);
    void readItemLibraryEntryProperty(const QString &name, const QVariant &value);
    void readPropertyProperty(const QString &name, const QVariant &value);
    void readQmlSourceProperty(const QString &name, const QVariant &value);

    void setVersion(const QString &versionNumber);

    ParserSate parserState() const;
    void setParserState(ParserSate newParserState);

    void insertItemLibraryEntry();
    void insertProperty();

    void addErrorInvalidType(const QString &typeName);

    QString absoluteFilePathForDocument(const QString &relativeFilePath);

    QString m_documentPath;
    ParserSate m_parserState;
    MetaInfo m_metaInfo;

    TypeName m_currentClassName;
    QString m_currentIcon;
    QString m_currentSource;
    ItemLibraryEntry m_currentEntry;

    PropertyName m_currentPropertyName;
    QString m_currentPropertyType;
    QVariant m_currentPropertyValue;

    bool m_overwriteDuplicates;

    TypeName m_qualication;
};

}
}
#endif // METAINFOREADER_H

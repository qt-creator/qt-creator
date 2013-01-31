/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "metainforeader.h"
#include "metainfo.h"

#include <propertyparser.h>
#include <QXmlStreamReader>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QIcon>

namespace QmlDesigner {
namespace Internal {

enum {
    debug = false
};

const char rootElementName[] = "MetaInfo";
const char typeElementName[] = "Type";
const char ItemLibraryEntryElementName[] = "ItemLibraryEntry";
const char QmlSourceElementName[] = "QmlSource";
const char PropertyElementName[] = "Property";

MetaInfoReader::MetaInfoReader(const MetaInfo &metaInfo)
        : m_parserState(Undefined),
          m_metaInfo(metaInfo)
{
    m_overwriteDuplicates = false;
}

void MetaInfoReader::readMetaInfoFile(const QString &path, bool overwriteDuplicates)
{
    m_documentPath = path;
    m_overwriteDuplicates = overwriteDuplicates;
    m_parserState = ParsingDocument;
    if (!SimpleAbstractStreamReader::readFile(path)) {
        qWarning() << "readMetaInfoFile()" << path;
        qWarning() << errors();
        m_parserState = Error;
        throw InvalidMetaInfoException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (!errors().isEmpty()) {
        qWarning() << "readMetaInfoFile()" << path;
        qWarning() << errors();
        m_parserState = Error;
        throw InvalidMetaInfoException(__LINE__, __FUNCTION__, __FILE__);
    }
}

QStringList MetaInfoReader::errors()
{
    return QmlJS::SimpleAbstractStreamReader::errors();
}

void MetaInfoReader::setQualifcation(const QString &qualification)
{
    m_qualication = qualification;
}

void MetaInfoReader::elementStart(const QString &name)
{
    switch (parserState()) {
    case ParsingDocument: setParserState(readDocument(name)); break;
    case ParsingMetaInfo: setParserState(readMetaInfoRootElement(name)); break;
    case ParsingType: setParserState(readTypeElement(name)); break;
    case ParsingItemLibrary: setParserState(readItemLibraryEntryElement(name)); break;
    case ParsingProperty: setParserState(readPropertyElement(name)); break;
    case ParsingQmlSource: setParserState(readQmlSourceElement(name)); break;
    case Finished:
    case Undefined: setParserState(Error);
        addError(tr("Illegal state while parsing"), currentSourceLocation());
    case Error:
    default: return;
    }
}

void MetaInfoReader::elementEnd()
{
    switch (parserState()) {
    case ParsingMetaInfo: setParserState(Finished); break;
    case ParsingType: setParserState(ParsingMetaInfo); break;
    case ParsingItemLibrary: insertItemLibraryEntry(); setParserState((ParsingType)); break;
    case ParsingProperty: insertProperty(); setParserState(ParsingItemLibrary);  break;
    case ParsingQmlSource: setParserState(ParsingItemLibrary); break;
    case ParsingDocument:
    case Finished:
    case Undefined: setParserState(Error);
        addError(tr("Illegal state while parsing"), currentSourceLocation());
    case Error:
    default: return;
    }
}

void MetaInfoReader::propertyDefinition(const QString &name, const QVariant &value)
{
    switch (parserState()) {
    case ParsingType: readTypeProperty(name, value); break;
    case ParsingItemLibrary: readItemLibraryEntryProperty(name, value); break;
    case ParsingProperty: readPropertyProperty(name, value); break;
    case ParsingQmlSource: readQmlSourceProperty(name, value); break;
    case ParsingMetaInfo: addError(tr("No property definition allowed"), currentSourceLocation()); break;
    case ParsingDocument:
    case Finished:
    case Undefined: setParserState(Error);
        addError(tr("Illegal state while parsing"), currentSourceLocation());
    case Error:
    default: return;
    }
}

MetaInfoReader::ParserSate MetaInfoReader::readDocument(const QString &name)
{
    if (name == QLatin1String(rootElementName)) {
        m_currentClassName = QString();
        m_currentIcon = QString();
        return ParsingMetaInfo;
    } else {
        addErrorInvalidType(name);
        return Error;
    }
}

MetaInfoReader::ParserSate MetaInfoReader::readMetaInfoRootElement(const QString &name)
{
    if (name == QLatin1String(typeElementName)) {
        m_currentClassName = QString();
        m_currentIcon = QString();
        return ParsingType;
    } else {
        addErrorInvalidType(name);
        return Error;
    }
}

MetaInfoReader::ParserSate MetaInfoReader::readTypeElement(const QString &name)
{
    if (name == QLatin1String(ItemLibraryEntryElementName)) {
        m_currentEntry = ItemLibraryEntry();
        m_currentEntry.setForceImport(false);
        m_currentEntry.setType(m_currentClassName, -1, -1);
        m_currentEntry.setIcon(QIcon(m_currentIcon));
        return ParsingItemLibrary;
    } else {
        addErrorInvalidType(name);
        return Error;
    }
}

MetaInfoReader::ParserSate MetaInfoReader::readItemLibraryEntryElement(const QString &name)
{
    if (name == QmlSourceElementName) {
        return ParsingQmlSource;
    } else if (name == PropertyElementName) {
        m_currentPropertyName = QString();
        m_currentPropertyType = QString();
        m_currentPropertyValue = QVariant();
        return ParsingProperty;
    } else {
        addError(tr("Invalid type %1").arg(name), currentSourceLocation());
        return Error;
    }
}

MetaInfoReader::ParserSate MetaInfoReader::readPropertyElement(const QString &name)
{
    addError(tr("Invalid type %1").arg(name), currentSourceLocation());
    return Error;
}

MetaInfoReader::ParserSate MetaInfoReader::readQmlSourceElement(const QString &name)
{
    addError(tr("Invalid type %1").arg(name), currentSourceLocation());
    return Error;
}

void MetaInfoReader::readTypeProperty(const QString &name, const QVariant &value)
{
    if (name == QLatin1String("name")) {
        m_currentClassName = value.toString();
        if (!m_qualication.isEmpty()) //prepend qualification
            m_currentClassName = m_qualication + QLatin1String(".") + m_currentClassName;
    } else if (name == QLatin1String("icon")) {
        m_currentIcon = absoluteFilePathForDocument(value.toString());
    } else {
        addError(tr("Unknown property for Type %1").arg(name), currentSourceLocation());
        setParserState(Error);
    }
}

void MetaInfoReader::readItemLibraryEntryProperty(const QString &name, const QVariant &value)
{
    if (name == QLatin1String("name")) {
        m_currentEntry.setName(value.toString());
    } else if (name == QLatin1String("category")) {
        m_currentEntry.setCategory(value.toString());
    } else if (name == QLatin1String("libraryIcon")) {
        m_currentEntry.setIconPath(absoluteFilePathForDocument(value.toString()));
    } else if (name == QLatin1String("version")) {
        setVersion(value.toString());
    } else if (name == QLatin1String("requiredImport")) {
        m_currentEntry.setRequiredImport(value.toString());
    } else if (name == QLatin1String("forceImport")) {
        m_currentEntry.setForceImport(value.toBool());
    } else {
        addError(tr("Unknown property for ItemLibraryEntry %1").arg(name), currentSourceLocation());
        setParserState(Error);
    }
}

void MetaInfoReader::readPropertyProperty(const QString &name, const QVariant &value)
{
    if (name == QLatin1String("name")) {
       m_currentPropertyName = value.toString();
    } else if (name == QLatin1String("type")) {
        m_currentPropertyType = value.toString();
    } else if (name == QLatin1String("value")) {
        m_currentPropertyValue = value;
    } else {
        addError(tr("Unknown property for Property %1").arg(name), currentSourceLocation());
        setParserState(Error);
    }
}

void MetaInfoReader::readQmlSourceProperty(const QString &name, const QVariant &value)
{
    if (name == QLatin1String("source")) {
        m_currentEntry.setQml(value.toString());
    } else {
        addError(tr("Unknown property for QmlSource %1").arg(name), currentSourceLocation());
        setParserState(Error);
    }
}

void MetaInfoReader::setVersion(const QString &versionNumber)
{
    const QString typeName = m_currentEntry.typeName();
    int major = 1;
    int minor = 0;

    if (!versionNumber.isEmpty()) {
        int val;
        bool ok;
        if (versionNumber.contains('.')) {
            val = versionNumber.split('.').first().toInt(&ok);
            major = ok ? val : major;
            val = versionNumber.split('.').last().toInt(&ok);
            minor = ok ? val : minor;
        } else {
            val = versionNumber.toInt(&ok);
            major = ok ? val : major;
        }
    }
    m_currentEntry.setType(typeName, major, minor);
}

MetaInfoReader::ParserSate MetaInfoReader::parserState() const
{
    return m_parserState;
}

void MetaInfoReader::setParserState(ParserSate newParserState)
{
    if (debug && newParserState == Error)
        qDebug() << "Error";

    m_parserState = newParserState;
}

void MetaInfoReader::insertItemLibraryEntry()
{
    if (debug) {
        qDebug() << "insertItemLibraryEntry()";
        qDebug() << m_currentEntry;
    }

    try {
        m_metaInfo.itemLibraryInfo()->addEntry(m_currentEntry, m_overwriteDuplicates);
    } catch (InvalidMetaInfoException &) {
        addError(tr("Invalid or duplicate item library entry %1").arg(m_currentEntry.name()), currentSourceLocation());
    }
}

void MetaInfoReader::insertProperty()
{
    m_currentEntry.addProperty(m_currentPropertyName, m_currentPropertyType, m_currentPropertyValue);
}

void MetaInfoReader::addErrorInvalidType(const QString &typeName)
{
    addError(tr("Invalid type %1").arg(typeName), currentSourceLocation());
}

QString MetaInfoReader::absoluteFilePathForDocument(const QString &relativeFilePath)
{

    QFileInfo fileInfo(relativeFilePath);
    if (fileInfo.isAbsolute() && fileInfo.exists())
        return relativeFilePath;

    return QFileInfo(QFileInfo(m_documentPath).absolutePath() + QLatin1String("/") + relativeFilePath).absoluteFilePath();
}

} //Internal
} //QmlDesigner

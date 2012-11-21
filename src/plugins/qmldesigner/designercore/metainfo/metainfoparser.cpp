/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "metainfoparser.h"
#include "metainfo.h"

#include <propertyparser.h>
#include <QXmlStreamReader>
#include <QString>
#include <QFile>
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

MetaInfoParser::MetaInfoParser(const MetaInfo &metaInfo)
        : m_parserState(Undefined),
          m_metaInfo(metaInfo)
{
}

void MetaInfoParser::readMetaInfoFile(const QString &path)
{
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

QStringList MetaInfoParser::errors()
{
    return QmlJS::SimpleAbstractStreamReader::errors();
}

void MetaInfoParser::elementStart(const QString &name)
{
    switch (parserState()) {
    case ParsingDocument: setParserState(readDocument(name)); break;
    case ParsingMetaInfo: setParserState(readMetaInfoRootElement(name)); break;
    case ParsingType: setParserState(readTypeElement(name)); break;
    case ParsingItemLibrary: setParserState(readItemLibraryEntryElement(name)); break;
    case ParsingProperty: setParserState(readPropertyElement(name)); break;
    case ParsingQmlSource: setParserState(readQmlSourceElement(name)); break;
    case Finished: setParserState(Error);
    case Undefined: setParserState(Error);
        addError(tr("Illegal state while parsing"), currentSourceLocation());
    case Error:
    default: return;
    }
}

void MetaInfoParser::elementEnd()
{
    switch (parserState()) {
    case ParsingMetaInfo: setParserState(ParsingDocument); break;
    case ParsingType: setParserState(ParsingMetaInfo); break;
    case ParsingItemLibrary: insertItemLibraryEntry(); setParserState((ParsingType)); break;
    case ParsingProperty: insertProperty(); setParserState(ParsingItemLibrary);  break;
    case ParsingQmlSource: setParserState(ParsingItemLibrary); break;
    case ParsingDocument: setParserState(Finished);
    case Finished: setParserState(Error);
    case Undefined: setParserState(Error);
        addError(tr("Illegal state while parsing"), currentSourceLocation());
    case Error:
    default: return;
    }
}

void MetaInfoParser::propertyDefinition(const QString &name, const QVariant &value)
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

MetaInfoParser::ParserSate MetaInfoParser::readDocument(const QString &name)
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

MetaInfoParser::ParserSate MetaInfoParser::readMetaInfoRootElement(const QString &name)
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

MetaInfoParser::ParserSate MetaInfoParser::readTypeElement(const QString &name)
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

MetaInfoParser::ParserSate MetaInfoParser::readItemLibraryEntryElement(const QString &name)
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

MetaInfoParser::ParserSate MetaInfoParser::readPropertyElement(const QString &name)
{
    addError(tr("Invalid type %1").arg(name), currentSourceLocation());
    return Error;
}

MetaInfoParser::ParserSate MetaInfoParser::readQmlSourceElement(const QString &name)
{
    addError(tr("Invalid type %1").arg(name), currentSourceLocation());
    return Error;
}

void MetaInfoParser::readTypeProperty(const QString &name, const QVariant &value)
{
    if (name == QLatin1String("name")) {
        m_currentClassName = value.toString();
    } else if (name == QLatin1String("icon")) {
        m_currentIcon = value.toString();
    } else {
        addError(tr("Unknown property for Type %1").arg(name), currentSourceLocation());
        setParserState(Error);
    }
}

void MetaInfoParser::readItemLibraryEntryProperty(const QString &name, const QVariant &value)
{
    if (name == QLatin1String("name")) {
        m_currentEntry.setName(value.toString());
    } else if (name == QLatin1String("category")) {
        m_currentEntry.setCategory(value.toString());
    } else if (name == QLatin1String("libraryIcon")) {
        m_currentEntry.setIconPath(value.toString());
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

void MetaInfoParser::readPropertyProperty(const QString &name, const QVariant &value)
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

void MetaInfoParser::readQmlSourceProperty(const QString &name, const QVariant &value)
{
    if (name == QLatin1String("source")) {
        m_currentEntry.setQml(value.toString());
    } else {
        addError(tr("Unknown property for QmlSource %1").arg(name), currentSourceLocation());
        setParserState(Error);
    }
}

void MetaInfoParser::setVersion(const QString &versionNumber)
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

MetaInfoParser::ParserSate MetaInfoParser::parserState() const
{
    return m_parserState;
}

void MetaInfoParser::setParserState(ParserSate newParserState)
{
    if (debug && newParserState == Error)
        qDebug() << "Error";

    m_parserState = newParserState;
}

void MetaInfoParser::insertItemLibraryEntry()
{
    if (debug) {
        qDebug() << "insertItemLibraryEntry()";
        qDebug() << m_currentEntry;
    }

    try {
        m_metaInfo.itemLibraryInfo()->addEntry(m_currentEntry);
    } catch (InvalidMetaInfoException &) {
        addError(tr("Invalid or duplicate item library entry %1").arg(m_currentEntry.name()), currentSourceLocation());
    }
}

void MetaInfoParser::insertProperty()
{
    m_currentEntry.addProperty(m_currentPropertyName, m_currentPropertyType, m_currentPropertyValue);
}

void MetaInfoParser::addErrorInvalidType(const QString &typeName)
{
    addError(tr("Invalid type %1").arg(typeName), currentSourceLocation());
}

} //Internal
} //QmlDesigner

/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "metainfoparser.h"
#include "metainfo.h"

#include "propertymetainfo.h"
#include "model/propertyparser.h"
#include <QXmlStreamReader>
#include <QString>
#include <QFile>
#include <QtDebug>
#include <QIcon>

namespace QmlDesigner {
namespace Internal {

static bool stringToBool(const QString &boolString)
{
    QString lowerString(boolString.toLower());
    if (lowerString == "true" || lowerString == "1")
        return true;

    return false;
}

MetaInfoParser::MetaInfoParser(const MetaInfo &metaInfo)
        : m_metaInfo(metaInfo)
{
}

void MetaInfoParser::parseFile(const QString &path)
{
    QFile file;
    file.setFileName(path);
    if (!file.open(QIODevice::ReadOnly))
        throw new InvalidMetaInfoException(__LINE__, __FUNCTION__, __FILE__);

    QXmlStreamReader reader;
    reader.setDevice(&file);

    while (!reader.atEnd()) {
        reader.readNext();
        tokenHandler(reader);
    }
    errorHandling(reader, file);
}

void MetaInfoParser::tokenHandler(QXmlStreamReader &reader)
{
    if (reader.isStartElement() && reader.name() == "metainfo")
        handleMetaInfoElement(reader);
}

void MetaInfoParser::handleMetaInfoElement(QXmlStreamReader &reader)
{
    while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == "metainfo")) {
        reader.readNext();
        metaInfoHandler(reader);
    }
}

void MetaInfoParser::metaInfoHandler(QXmlStreamReader &reader)
{
    if (reader.isStartElement())
    {
        if (reader.name() == "enumerator")
            handleEnumElement(reader);

        if (reader.name() == "flag")
            handleFlagElement(reader);

        if (reader.name() == "node")
            handleNodeElement(reader);
    }
}

void MetaInfoParser::handleEnumElement(QXmlStreamReader &reader)
{
    QString enumeratorName = reader.attributes().value("name").toString();
    QString enumeratorScope = reader.attributes().value("scope").toString();
    EnumeratorMetaInfo enumeratorMetaInfo;
    if (m_metaInfo.hasEnumerator(enumeratorName)) {
        enumeratorMetaInfo = m_metaInfo.enumerator(enumeratorName);
    } else {
        enumeratorMetaInfo = m_metaInfo.addEnumerator(enumeratorScope, enumeratorName);
    }

    while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == "enumerator")) {

        reader.readNext();
        handleEnumElementElement(reader, enumeratorMetaInfo);
    }
}

void MetaInfoParser::handleFlagElement(QXmlStreamReader &reader)
{
    QString enumeratorName = reader.attributes().value("name").toString();
    QString enumeratorScope = reader.attributes().value("scope").toString();
    EnumeratorMetaInfo enumeratorMetaInfo = m_metaInfo.addFlag(enumeratorScope, enumeratorName);

    while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == "flag")) {

        reader.readNext();
        handleFlagElementElement(reader, enumeratorMetaInfo);
    }
}

void MetaInfoParser::handleEnumElementElement(QXmlStreamReader &reader, EnumeratorMetaInfo &enumeratorMetaInfo)
{
    if (reader.isStartElement() && reader.name() == "element")
    {
        bool isIntType;
        enumeratorMetaInfo.addElement(reader.attributes().value("name").toString(),
                                     reader.attributes().value("value").toString().toInt(&isIntType));
        Q_ASSERT(isIntType);
    }
}

void MetaInfoParser::handleFlagElementElement(QXmlStreamReader &reader, EnumeratorMetaInfo &enumeratorMetaInfo)
{
    if (reader.isStartElement() && reader.name() == "element")
    {
        bool isIntType;
        enumeratorMetaInfo.addElement(reader.attributes().value("name").toString(),
                                     reader.attributes().value("value").toString().toInt(&isIntType));
        Q_ASSERT(isIntType);
    }
}

void MetaInfoParser::handleNodeElement(QXmlStreamReader &reader)
{
    const QXmlStreamAttributes attributes = reader.attributes();

    const QString className = attributes.value("name").toString();
    if (className.isEmpty()) {
        reader.raiseError("Invalid element 'node' - mandatory attribute 'name' is missing");
        return;
    }

    NodeMetaInfo nodeMetaInfo;
    if (m_metaInfo.hasNodeMetaInfo(className)) {
        nodeMetaInfo = m_metaInfo.nodeMetaInfo(className);
    } else {
        qWarning() << "Metainfo: " << className << " does not exist";
        while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == "node"))
            reader.readNext();
        return;
    }

    if (attributes.hasAttribute("isContainer")) {
        const QString isContainer = attributes.value("isContainer").toString();
        nodeMetaInfo.setIsContainer(stringToBool(isContainer));
    }

    if (attributes.hasAttribute("showInItemLibrary")) {
        const QString showInItemLibrary = attributes.value("showInItemLibrary").toString();
        nodeMetaInfo.setIsVisibleToItemLibrary(stringToBool(showInItemLibrary));
    }

    if (attributes.hasAttribute("category")) {
        const QString category = attributes.value("category").toString();
        nodeMetaInfo.setCategory(category);
    }

    if (attributes.hasAttribute("icon")) {
        const QString iconPath = reader.attributes().value("icon").toString();
        nodeMetaInfo.setIcon(QIcon(iconPath));
    }

    while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == "node")) {
        reader.readNext();

        handleNodeInheritElement(reader, className);
        handleAbstractPropertyElement(reader, nodeMetaInfo);
        handleAbstractPropertyDefaultValueElement(reader, nodeMetaInfo);
        handleNodeItemLibraryRepresentationElement(reader, className);
    }
}

void MetaInfoParser::handleNodeItemLibraryRepresentationElement(QXmlStreamReader &reader, const QString & className)
{
    if (reader.isStartElement() && reader.name() == "itemlibraryrepresentation")
    {
        QString name = reader.attributes().value("name").toString();
        ItemLibraryInfo ItemLibraryInfo = m_metaInfo.addItemLibraryInfo(m_metaInfo.nodeMetaInfo(className), name);

        QString iconPath = reader.attributes().value("icon").toString();
        if (!iconPath.isEmpty())
            ItemLibraryInfo.setIcon(QIcon(iconPath));

        while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == "itemlibraryrepresentation")) {
            reader.readNext();
            handleItemLibraryInfoPropertyElement(reader, ItemLibraryInfo);
        }
    }
}

void MetaInfoParser::handleNodeInheritElement(QXmlStreamReader &reader, const QString & className)
{
    if (reader.isStartElement() && reader.name() == "inherits")
    {
        QString superClassName = reader.attributes().value("name").toString();
        Q_ASSERT(!superClassName.isEmpty());
        m_metaInfo.addSuperClassRelationship(superClassName, className);
        reader.readNext();
    }
}

void MetaInfoParser::handleItemLibraryInfoPropertyElement(QXmlStreamReader &reader, ItemLibraryInfo &ItemLibraryInfo)
{
    if (reader.isStartElement() && reader.name() == "property")
    {
        QXmlStreamAttributes attributes(reader.attributes());
        QString name = attributes.value("name").toString();
        QString type = attributes.value("type").toString();
        QString value = attributes.value("value").toString();
        ItemLibraryInfo.addProperty(name, type, value);

        reader.readNext();
    }
}

void MetaInfoParser::handleAbstractPropertyDefaultValueElement(QXmlStreamReader &reader, NodeMetaInfo &nodeMetaInfoArg)
{
    if (reader.isStartElement() && reader.name() == "propertyDefaultValue")
    {
        const QXmlStreamAttributes attributes(reader.attributes());
        Q_ASSERT(attributes.hasAttribute("name"));
        Q_ASSERT(attributes.hasAttribute("type"));
        Q_ASSERT(attributes.hasAttribute("defaultValue"));
        const QString propertyName = attributes.value("name").toString();
        const QString propertyType = attributes.value("type").toString();
        const QString defaultValueString = attributes.value("defaultValue").toString();
        QVariant defaultValue = Internal::PropertyParser::read(propertyType,
                                                               defaultValueString,
                                                               m_metaInfo);

        QList<NodeMetaInfo> nodeMetaInfoList(nodeMetaInfoArg.superClasses());
        nodeMetaInfoList.prepend(nodeMetaInfoArg);
        foreach(const NodeMetaInfo &nodeMetaInfo, nodeMetaInfoList) {
            if (nodeMetaInfo.hasLocalProperty(propertyName)) {
                nodeMetaInfo.property(propertyName).setDefaultValue(nodeMetaInfoArg, defaultValue);
                break;
            }
        }

        reader.readNext();
    }
}

void MetaInfoParser::handleAbstractPropertyElement(QXmlStreamReader &reader, NodeMetaInfo &nodeMetaInfo)
{
    if (reader.isStartElement() && reader.name() == "property")
    {
        const QXmlStreamAttributes attributes(reader.attributes());


        const QString propertyName = attributes.value("name").toString();

        if (propertyName.isEmpty()) {
            reader.raiseError("Invalid element 'property' - attribute 'name' is missing or empty");
            return;
        }

        PropertyMetaInfo propertyMetaInfo;
        if (nodeMetaInfo.hasLocalProperty(propertyName)) {
            propertyMetaInfo = nodeMetaInfo.property(propertyName);
        } else {
            propertyMetaInfo.setName(propertyName);
        }
        propertyMetaInfo.setValid(true);

        //propertyMetaInfo.setReadable(stringToBool(attributes.value("isReadable").toString()));
        //propertyMetaInfo.setWritable(stringToBool(attributes.value("isWritable").toString()));
        //propertyMetaInfo.setResettable(stringToBool(attributes.value("isResetable").toString()));
        if (attributes.hasAttribute("isEnumType"))
            propertyMetaInfo.setEnumType(stringToBool(attributes.value("isEnumType").toString()));
        if (attributes.hasAttribute("isFlagType"))
            propertyMetaInfo.setFlagType(stringToBool(attributes.value("isFlagType").toString()));
        if (attributes.hasAttribute("showInPropertyEditor"))
            propertyMetaInfo.setIsVisibleToPropertyEditor(stringToBool(attributes.value("showInPropertyEditor").toString()));

        if (propertyMetaInfo.isEnumType()) {
            propertyMetaInfo.setType(QString("%1::%2").arg(attributes.value("enumeratorScope").toString(),
                                                           attributes.value("enumeratorName").toString()));
            propertyMetaInfo.setEnumerator(m_metaInfo.enumerator(propertyMetaInfo.type()));
        } else {
            const QString type = attributes.value("type").toString();
            if (type.isEmpty()) {
                reader.raiseError("Invalid element 'property' - attribute 'type' is missing or empty");
                return;
            }
            propertyMetaInfo.setType(attributes.value("type").toString());
        }

        if (attributes.hasAttribute("defaultValue")) {
            QVariant defaultValue = Internal::PropertyParser::read(propertyMetaInfo.type(),
                                                                   attributes.value("defaultValue").toString(),
                                                                   m_metaInfo);

            propertyMetaInfo.setDefaultValue(nodeMetaInfo, defaultValue);
        }

        nodeMetaInfo.addProperty(propertyMetaInfo);

        reader.readNext();
    }
}

void MetaInfoParser::errorHandling(QXmlStreamReader &reader, QFile &file)
{
    if (!reader.hasError())
        return;

    qDebug() << QString("Error at %1, %2:%3: %4")
            .arg(file.fileName())
            .arg(reader.lineNumber())
            .arg(reader.columnNumber())
            .arg(reader.errorString());

    file.reset();

    QString fileString = file.readAll();
    QString snippetString;
    int lineCount = 0;
    int position = reader.characterOffset();
    while (position >= 0)
    {
        if (fileString[position] == '\n') {
            if (lineCount > 3)
                break;
            lineCount++;
        }

        snippetString.prepend(fileString[position]);
        position--;
    }

    lineCount = 0;
    position = reader.characterOffset();
    while (position >= 0)
    {
        position++;
        if (fileString[position] == '\n') {
            if (lineCount > 1)
                break;
            lineCount++;
        }

        snippetString.append(fileString[position]);
    }

    qDebug() << snippetString;

}


}
}

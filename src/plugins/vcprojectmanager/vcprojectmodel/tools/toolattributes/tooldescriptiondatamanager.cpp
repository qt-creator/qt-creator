/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#include "tooldescriptiondatamanager.h"

#include "attributedescriptiondataitem.h"
#include "toolattributeoption.h"
#include "tooldescription.h"
#include "booltoolattribute.h"
#include "integertoolattribute.h"
#include "stringtoolattribute.h"
#include "stringlisttoolattribute.h"
#include "vcschemamanager.h"
#include "../toolsectiondescription.h"

#include <QFile>
#include <QMessageBox>

namespace VcProjectManager {
namespace Internal {

namespace {
ToolAttributeOption* processOptionValueAttributes(const QDomNode &node)
{
    QDomNamedNodeMap namedNodeMap = node.attributes();
    QDomNode domNode = namedNodeMap.item(0);

    if (domNode.nodeType() == QDomNode::AttributeNode) {
        QDomAttr domElement = domNode.toAttr();

        if (domElement.name() == QLatin1String("ValueText"))
            return new ToolAttributeOption(domElement.value(), node.toElement().text());
    }

    return 0;
}

ToolAttributeOption* processOptionValues(const QDomNode &node)
{
    QDomNode temp = node;
    ToolAttributeOption *option = 0;
    ToolAttributeOption *firstOption = 0;

    while (!temp.isNull()) {
        ToolAttributeOption *newOption = processOptionValueAttributes(temp);

        if (!option)
            firstOption = newOption;
        else
            option->appendOption(newOption);

        option = newOption;
        temp = temp.nextSibling();
    }

    return firstOption;
}
} // unnamed namespace

ToolDescriptionDataManager* ToolDescriptionDataManager::m_instance = 0;

ToolDescriptionDataManager *ToolDescriptionDataManager::instance()
{
    return m_instance;
}

void ToolDescriptionDataManager::readToolXMLFiles()
{
    VcSchemaManager *vcSM = VcSchemaManager::instance();
    QList<QString> toolFiles = vcSM->toolXMLFilePaths();

    foreach (const QString &toolFile, toolFiles)
        readAttributeDataFromFile(toolFile);
}

ToolDescriptionDataManager::~ToolDescriptionDataManager()
{
    qDeleteAll(m_toolDescriptions);
}

int ToolDescriptionDataManager::toolDescriptionCount() const
{
    return m_toolDescriptions.size();
}

ToolDescription *ToolDescriptionDataManager::toolDescription(int index) const
{
    if (0 <= index && index < m_toolDescriptions.size())
        return m_toolDescriptions[index];
    return 0;
}

ToolDescription *ToolDescriptionDataManager::toolDescription(const QString &toolKey) const
{
    foreach (ToolDescription *toolDesc, m_toolDescriptions) {
        if (toolDesc->toolKey() == toolKey)
            return toolDesc;
    }

    return 0;
}

ToolInfo ToolDescriptionDataManager::readToolInfo(const QString &filePath, QString *errorMsg, int *errorLine, int *errorColumn)
{
    ToolInfo toolInfo;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return toolInfo;

    QDomDocument xmlDoc(filePath);
    if (!xmlDoc.setContent(&file, errorMsg, errorLine, errorColumn)) {
        file.close();
        return toolInfo;
    }

    if (file.isOpen())
        file.close();

    QDomNode nextSibling = xmlDoc.firstChild().nextSibling();

    while (nextSibling.nodeName() != QLatin1String("ToolInfo"))
        nextSibling = nextSibling.nextSibling();

    if (nextSibling.isNull())
        return toolInfo;

    QDomNamedNodeMap namedNodeMap = nextSibling.attributes();

    for (int i = 0; i < namedNodeMap.count(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("Name"))
                toolInfo.m_displayName = domElement.value();
            else if (domElement.name() == QLatin1String("Key"))
                toolInfo.m_key = domElement.value();
        }
    }

    return toolInfo;
}

ToolDescriptionDataManager::ToolDescriptionDataManager()
{
    m_instance = this;
    readToolXMLFiles();
}

void ToolDescriptionDataManager::readAttributeDataFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QDomDocument xmlDoc(filePath);
    if (!xmlDoc.setContent(&file)) {
        file.close();
        return;
    }

    if (file.isOpen())
        file.close();

    processXMLDoc(xmlDoc);
}

void ToolDescriptionDataManager::processXMLDoc(const QDomDocument &xmlDoc)
{
    // skip xml description part at the beginning of the file
    QDomNode nextSibling = xmlDoc.firstChild().nextSibling();
    processDomNode(nextSibling);
}

void ToolDescriptionDataManager::processDomNode(const QDomNode &node)
{
    ToolDescription *toolDesc = readToolDescription(node);
    m_toolDescriptions.append(toolDesc);

    if (node.hasChildNodes())
        processToolSectionNode(toolDesc, node.firstChild());
}

void ToolDescriptionDataManager::processToolSectionNode(ToolDescription *toolDescription, const QDomNode &domNode)
{
    if (domNode.nodeName() == QLatin1String("Section")) {
        ToolSectionDescription *toolSectionDesc = new ToolSectionDescription;
        toolDescription->addSectionDescription(toolSectionDesc);
        QDomNamedNodeMap namedNodeMap = domNode.attributes();

        for (int i = 0; i < namedNodeMap.count(); ++i) {
            QDomNode domNode = namedNodeMap.item(i);

            if (domNode.nodeType() == QDomNode::AttributeNode) {
                QDomAttr domElement = domNode.toAttr();

                if (domElement.name() == QLatin1String("Name")) {
                    toolSectionDesc->setName(domElement.value());
                }
            }
        }

        if (domNode.hasChildNodes())
            processToolAttributeDescriptions(toolSectionDesc, domNode.firstChild());

        QDomNode nextSibling = domNode.nextSibling();
        if (!nextSibling.isNull())
            processToolSectionNode(toolDescription, nextSibling);
    }
}

void ToolDescriptionDataManager::processToolAttributeDescriptions(ToolSectionDescription *toolSectDesc, const QDomNode &domNode)
{
    if (domNode.nodeName() == QLatin1String("String")) {
        QDomNamedNodeMap namedNodeMap = domNode.attributes();
        QString key;
        QString displayName;
        QString defaultValue;
        QString description;

        for (int i = 0; i < namedNodeMap.count(); ++i) {
            QDomNode domNode = namedNodeMap.item(i);

            if (domNode.nodeType() == QDomNode::AttributeNode) {
                QDomAttr domElement = domNode.toAttr();

                if (domElement.name() == QLatin1String("Name"))
                    displayName = domElement.value();

                else if (domElement.name() == QLatin1String("Key"))
                    key = domElement.value();

                else if (domElement.name() == QLatin1String("DefaultValue"))
                    defaultValue = domElement.value();

                else if (domElement.name() == QLatin1String("Description"))
                    description = domElement.value();
            }
        }
        IAttributeDescriptionDataItem *dataItem = new AttributeDescriptionDataItem(QLatin1String("String"),
                                                                                              key,
                                                                                              displayName,
                                                                                              description,
                                                                                              defaultValue);
        toolSectDesc->addAttributeDescription(dataItem);
    }

    else if (domNode.nodeName() == QLatin1String("StringList")) {
        QDomNamedNodeMap namedNodeMap = domNode.attributes();
        QString key;
        QString displayName;
        QString defaultValue;
        QString description;
        QString separator;

        for (int i = 0; i < namedNodeMap.count(); ++i) {
            QDomNode domNode = namedNodeMap.item(i);

            if (domNode.nodeType() == QDomNode::AttributeNode) {
                QDomAttr domElement = domNode.toAttr();

                if (domElement.name() == QLatin1String("Name"))
                    displayName = domElement.value();

                else if (domElement.name() == QLatin1String("Key"))
                    key = domElement.value();

                else if (domElement.name() == QLatin1String("DefaultValue"))
                    defaultValue = domElement.value();

                else if (domElement.name() == QLatin1String("Description"))
                    description = domElement.value();

                else if (domElement.name() == QLatin1String("Separator"))
                    separator = domElement.value();
            }
        }
        IAttributeDescriptionDataItem *dataItem = new AttributeDescriptionDataItem(QLatin1String("StringList"),
                                                                                   key,
                                                                                   displayName,
                                                                                   description,
                                                                                   defaultValue);
        dataItem->setOptionalValue(QLatin1String("separator"), separator);
        toolSectDesc->addAttributeDescription(dataItem);
    }

    else if (domNode.nodeName() == QLatin1String("Integer")) {
        QDomNamedNodeMap namedNodeMap = domNode.attributes();
        QString key;
        QString displayName;
        QString defaultValue;
        QString description;

        for (int i = 0; i < namedNodeMap.count(); ++i) {
            QDomNode domNode = namedNodeMap.item(i);

            if (domNode.nodeType() == QDomNode::AttributeNode) {
                QDomAttr domElement = domNode.toAttr();

                if (domElement.name() == QLatin1String("Name"))
                    displayName = domElement.value();

                else if (domElement.name() == QLatin1String("Key"))
                    key = domElement.value();

                else if (domElement.name() == QLatin1String("DefaultValue"))
                    defaultValue = domElement.value();

                else if (domElement.name() == QLatin1String("Description"))
                    description = domElement.value();
            }
        }

        ToolAttributeOption *option = processOptionValues(domNode.firstChild());
        IAttributeDescriptionDataItem *dataItem = new AttributeDescriptionDataItem(QLatin1String("Integer"),
                                                                                   key,
                                                                                   displayName,
                                                                                   description,
                                                                                   defaultValue);
        dataItem->setFirstOption(option);
        toolSectDesc->addAttributeDescription(dataItem);
    }

    else if (domNode.nodeName() == QLatin1String("Boolean")) {
        QDomNamedNodeMap namedNodeMap = domNode.attributes();
        QString key;
        QString displayName;
        QString defaultValue;
        QString description;

        for (int i = 0; i < namedNodeMap.count(); ++i) {
            QDomNode domNode = namedNodeMap.item(i);

            if (domNode.nodeType() == QDomNode::AttributeNode) {
                QDomAttr domElement = domNode.toAttr();

                if (domElement.name() == QLatin1String("Name"))
                    displayName = domElement.value();

                else if (domElement.name() == QLatin1String("Key"))
                    key = domElement.value();

                else if (domElement.name() == QLatin1String("DefaultValue"))
                    defaultValue = domElement.value();

                else if (domElement.name() == QLatin1String("Description"))
                    description = domElement.value();
            }
        }

        ToolAttributeOption* option = processOptionValues(domNode.firstChild());
        IAttributeDescriptionDataItem *dataItem = new AttributeDescriptionDataItem(QLatin1String("Boolean"),
                                                                                   key,
                                                                                   displayName,
                                                                                   description,
                                                                                   defaultValue);
        dataItem->setFirstOption(option);
        toolSectDesc->addAttributeDescription(dataItem);
    }

    QDomNode nextSibling = domNode.nextSibling();
    if (!nextSibling.isNull())
        processToolAttributeDescriptions(toolSectDesc, nextSibling);
}

ToolDescription *ToolDescriptionDataManager::readToolDescription(const QDomNode &domNode)
{
    QDomNode tempNode = domNode;

    while (tempNode.nodeName() != QLatin1String("ToolInfo"))
        tempNode = tempNode.nextSibling();

    if (tempNode.isNull())
        return 0;

    ToolDescription *toolDesc = new ToolDescription();
    QDomNamedNodeMap namedNodeMap = tempNode.attributes();

    for (int i = 0; i < namedNodeMap.count(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("Name"))
                toolDesc->setToolDisplayName(domElement.value());
            else if (domElement.name() == QLatin1String("Key"))
                toolDesc->setToolKey(domElement.value());
        }
    }

    return toolDesc;
}

} // namespace Internal
} // namespace VcProjectManager

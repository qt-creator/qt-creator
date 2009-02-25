/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "proxml.h"
#include "proitems.h"

using namespace Qt4ProjectManager::Internal;

QString ProXmlParser::itemToString(ProItem *item)
{
    ProXmlParser xmlparser;
    QDomDocument doc("ProItem");
    QDomNode itemNode = xmlparser.createItemNode(doc, item);

    if (itemNode.isNull())
        return QString();

    doc.appendChild(itemNode);

    return doc.toString();
}

QDomNode ProXmlParser::createItemNode(QDomDocument doc, ProItem *item) const
{
    
    QDomElement tag;
    if (item->kind() == ProItem::ValueKind) {
        tag = doc.createElement(QLatin1String("value"));
        ProValue *v = static_cast<ProValue*>(item);
        QDomText text = doc.createTextNode(v->value());
        tag.appendChild(text);
    }

    if (item->kind() == ProItem::FunctionKind) {
        tag = doc.createElement(QLatin1String("function"));
        ProFunction *v = static_cast<ProFunction*>(item);
        QDomText text = doc.createTextNode(v->text());
        tag.appendChild(text);
    }

    if (item->kind() == ProItem::ConditionKind) {
        tag = doc.createElement(QLatin1String("condition"));
        ProCondition *v = static_cast<ProCondition*>(item);
        QDomText text = doc.createTextNode(v->text());
        tag.appendChild(text);
    }

    if (item->kind() == ProItem::OperatorKind) {
        tag = doc.createElement(QLatin1String("operator"));
        ProOperator *v = static_cast<ProOperator*>(item);
        tag.setAttribute(QLatin1String("type"), (int)v->operatorKind());
    }

    if (tag.isNull() && item->kind() != ProItem::BlockKind) {
        qDebug() << "*** Warning: Found unknown item!";
        return tag;
    }

    if (tag.isNull()) {
        ProBlock *block = static_cast<ProBlock*>(item);

        if (block->blockKind() & ProBlock::ProFileKind) {
            tag = doc.createElement(QLatin1String("file"));
        } else if (block->blockKind() & ProBlock::VariableKind) {
            tag = doc.createElement(QLatin1String("variable"));
            ProVariable *v = static_cast<ProVariable*>(block);
            tag.setAttribute(QLatin1String("name"), QString(v->variable()));
            tag.setAttribute(QLatin1String("type"), (int)v->variableOperator());
        } else if (block->blockKind() & ProBlock::ScopeKind) {
            tag = doc.createElement(QLatin1String("scope"));
        } else if (block->blockKind() & ProBlock::ScopeContentsKind) {
            tag = doc.createElement(QLatin1String("scopecontents"));
        } else {
            tag = doc.createElement(QLatin1String("block"));
        }

        foreach (ProItem *child, block->items()) {
            QDomNode childNode = createItemNode(doc, child);
            if (!childNode.isNull())
                tag.appendChild(childNode);
        }
    }
    
    QString comment = item->comment();
    comment = comment.replace('\\', QLatin1String("\\\\"));
    comment = comment.replace('\n', QLatin1String("\\n"));

    if (!comment.isEmpty())
        tag.setAttribute(QLatin1String("comment"), comment);
    
    return tag;
}

ProItem *ProXmlParser::parseItemNode(QDomDocument doc, QDomNode node) const
{
    QDomElement tag = node.toElement();
    if (tag.isNull()) {
        qDebug() << "*** Warning: Failed while parsing XML";
        return 0;
    }

    ProItem *item = 0;
    if (tag.tagName() == QLatin1String("value")) {
        item = new ProValue(tag.text(), 0);
    } else if (tag.tagName() == QLatin1String("function")) {
        item = new ProFunction(tag.text());
    } else if (tag.tagName() == QLatin1String("condition")) {
        item = new ProCondition(tag.text());
    } else if (tag.tagName() == QLatin1String("operator")) {
        int optype = tag.attribute(QLatin1String("type")).toInt();
        item = new ProOperator((ProOperator::OperatorKind)optype);
    } else if (tag.tagName() == QLatin1String("variable")) {
        QString name = tag.attribute(QLatin1String("name"));
        int vartype = tag.attribute(QLatin1String("type")).toInt();
        ProVariable::VariableOperator varop = ProVariable::VariableOperator(vartype);
        ProVariable *v = new ProVariable(name, 0);
        v->setVariableOperator(varop);
        item = v;
    } else if (tag.tagName() == QLatin1String("file")) {
        ProFile *v = new ProFile(QString());
        item = v;
    } else if (tag.tagName() == QLatin1String("scope")) {
        ProBlock *v = new ProBlock(0);
        v->setBlockKind(ProBlock::ScopeKind);
        item = v;
    } else if (tag.tagName() == QLatin1String("scopecontents")) {
        ProBlock *v = new ProBlock(0);
        v->setBlockKind(ProBlock::ScopeContentsKind);
        item = v;
    } else if (tag.tagName() == QLatin1String("block")) {
        item = new ProBlock(0);
    }

    if (!item) {
        qDebug() << "*** Warning: Could not create item!";
        return 0;
    }

    QString comment = tag.attribute(QLatin1String("comment"));
    if (!comment.isEmpty()) {
        //### fix multiple lines
        item->setComment(comment);
    }

    if (item->kind() != ProItem::BlockKind)
        return item;

    ProBlock *block = static_cast<ProBlock *>(item);
    ProVariable *variable = 0;
    if (block->blockKind() & ProBlock::VariableKind)
        variable = static_cast<ProVariable *>(block);

    QDomNodeList children = tag.childNodes();
    for (int i=0; i<children.count(); ++i) {
        ProItem *childItem = parseItemNode(doc, children.at(i));
        if (!childItem)
            continue;

        if (variable && childItem->kind() == ProItem::ValueKind)
            static_cast<ProValue*>(childItem)->setVariable(variable);
        else if (childItem->kind() == ProItem::BlockKind)
            static_cast<ProBlock*>(childItem)->setParent(block);

        block->appendItem(childItem);
    }

    return item;
}

ProItem *ProXmlParser::stringToItem(const QString &xml)
{
    ProXmlParser xmlparser;
    QDomDocument doc("ProItem");

    doc.setContent(xml);

    return xmlparser.parseItemNode(doc, doc.documentElement());
}

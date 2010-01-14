/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QDebug>

#include "addarraymembervisitor.h"
#include "addobjectvisitor.h"
#include "addpropertyvisitor.h"
#include "changeobjecttypevisitor.h"
#include "changepropertyvisitor.h"
#include "moveobjectvisitor.h"
#include "moveobjectbeforeobjectvisitor.h"
#include "qmlrefactoring.h"
#include "removepropertyvisitor.h"
#include "removeuiobjectmembervisitor.h"

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

QmlRefactoring::QmlRefactoring(const QmlDocument::Ptr &doc, QmlDesigner::TextModifier &modifier, const QStringList &propertyOrder):
        qmlDocument(doc),
        textModifier(&modifier),
        m_propertyOrder(propertyOrder)
{
}

bool QmlRefactoring::reparseDocument()
{
    const QString newSource = textModifier->text();

//    qDebug() << "QmlRefactoring::reparseDocument() new QML source:" << newSource;

    QmlDesigner::QmlDocument::Ptr tmpDocument(QmlDesigner::QmlDocument::create("<ModelToTextMerger>"));
    tmpDocument->setSource(newSource);

    if (tmpDocument->parse()) {
        qmlDocument = tmpDocument;
        return true;
    } else {
        qWarning() << "*** Possible problem: QML file wasn't parsed correctly.";
        qDebug() << "*** QML text:" << textModifier->text();
        return false;
    }
}

bool QmlRefactoring::changeImports(const QSet<QmlDesigner::Import> &/*addedImports*/, const QSet<QmlDesigner::Import> &/*removedImports*/)
{
//    ChangeImportsVisitor visit(*textModifier, addedImports, removedImports, qmlDocument->source());
//    visit(qmlDocument->program());
    return false;
}

bool QmlRefactoring::addToArrayMemberList(int parentLocation, const QString &propertyName, const QString &content)
{
    Q_ASSERT(parentLocation >= 0);

    AddArrayMemberVisitor visit(*textModifier, (quint32) parentLocation, propertyName, content);
    visit.setConvertObjectBindingIntoArrayBinding(true);
    return visit(qmlDocument->program());
}

bool QmlRefactoring::addToObjectMemberList(int parentLocation, const QString &content)
{
    Q_ASSERT(parentLocation >= 0);

    AddObjectVisitor visit(*textModifier, (quint32) parentLocation, content, m_propertyOrder);
    return visit(qmlDocument->program());
}

bool QmlRefactoring::addProperty(int parentLocation, const QString &name, const QString &value, PropertyType propertyType)
{
    Q_ASSERT(parentLocation >= 0);

    AddPropertyVisitor visit(*textModifier, (quint32) parentLocation, name, value, propertyType, m_propertyOrder);
    return visit(qmlDocument->program());
}

bool QmlRefactoring::changeProperty(int parentLocation, const QString &name, const QString &value, PropertyType propertyType)
{
    Q_ASSERT(parentLocation >= 0);

    ChangePropertyVisitor visit(*textModifier, (quint32) parentLocation, name, value, propertyType);
    return visit(qmlDocument->program());
}

bool QmlRefactoring::changeObjectType(int nodeLocation, const QString &newType)
{
    Q_ASSERT(nodeLocation >= 0);
    Q_ASSERT(!newType.isEmpty());

    ChangeObjectTypeVisitor visit(*textModifier, (quint32) nodeLocation, newType);
    return visit(qmlDocument->program());
}

bool QmlRefactoring::moveObject(int objectLocation, const QString &targetPropertyName, bool targetIsArrayBinding, int targetParentObjectLocation)
{
    Q_ASSERT(objectLocation >= 0);
    Q_ASSERT(targetParentObjectLocation >= 0);

    MoveObjectVisitor visit(*textModifier, (quint32) objectLocation, targetPropertyName, targetIsArrayBinding, (quint32) targetParentObjectLocation, m_propertyOrder);
    return visit(qmlDocument->program());
}

bool QmlRefactoring::moveObjectBeforeObject(int movingObjectLocation, int beforeObjectLocation)
{
    Q_ASSERT(movingObjectLocation >= 0);
    Q_ASSERT(beforeObjectLocation >= -1);

    if (beforeObjectLocation == -1) {
        MoveObjectBeforeObjectVisitor visit(*textModifier, movingObjectLocation);
        return visit(qmlDocument->program());
    } else {
        MoveObjectBeforeObjectVisitor visit(*textModifier, movingObjectLocation, beforeObjectLocation);
        return visit(qmlDocument->program());
    }
    return false;
}

bool QmlRefactoring::removeObject(int nodeLocation)
{
    Q_ASSERT(nodeLocation >= 0);

    RemoveUIObjectMemberVisitor visit(*textModifier, (quint32) nodeLocation);
    return visit(qmlDocument->program());
}

bool QmlRefactoring::removeProperty(int parentLocation, const QString &name)
{
    Q_ASSERT(parentLocation >= 0);
    Q_ASSERT(!name.isEmpty());

    RemovePropertyVisitor visit(*textModifier, (quint32) parentLocation, name);
    return visit(qmlDocument->program());
}

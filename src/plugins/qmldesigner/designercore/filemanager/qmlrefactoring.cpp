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

#include <QDebug>

#include "addarraymembervisitor.h"
#include "addobjectvisitor.h"
#include "addpropertyvisitor.h"
#include "changeimportsvisitor.h"
#include "changeobjecttypevisitor.h"
#include "changepropertyvisitor.h"
#include "moveobjectvisitor.h"
#include "moveobjectbeforeobjectvisitor.h"
#include "qmlrefactoring.h"
#include "removepropertyvisitor.h"
#include "removeuiobjectmembervisitor.h"

using namespace QmlJS;
using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

QmlRefactoring::QmlRefactoring(const Document::Ptr &doc, TextModifier &modifier, const QStringList &propertyOrder):
        qmlDocument(doc),
        textModifier(&modifier),
        m_propertyOrder(propertyOrder)
{
}

bool QmlRefactoring::reparseDocument()
{
    const QString newSource = textModifier->text();

//    qDebug() << "QmlRefactoring::reparseDocument() new QML source:" << newSource;

    Document::MutablePtr tmpDocument(Document::create("<ModelToTextMerger>", Document::QmlLanguage));
    tmpDocument->setSource(newSource);

    if (tmpDocument->parseQml()) {
        qmlDocument = tmpDocument;
        return true;
    } else {
        qWarning() << "*** Possible problem: QML file wasn't parsed correctly.";
        qDebug() << "*** QML text:" << textModifier->text();
        return false;
    }
}

bool QmlRefactoring::addImport(const Import &import)
{
    ChangeImportsVisitor visitor(*textModifier, qmlDocument->source());
    return visitor.add(qmlDocument->qmlProgram(), import);
}

bool QmlRefactoring::removeImport(const Import &import)
{
    ChangeImportsVisitor visitor(*textModifier, qmlDocument->source());
    return visitor.remove(qmlDocument->qmlProgram(), import);
}

bool QmlRefactoring::addToArrayMemberList(int parentLocation, const QString &propertyName, const QString &content)
{
    if (parentLocation < 0)
        return false;

    AddArrayMemberVisitor visit(*textModifier, (quint32) parentLocation, propertyName, content);
    visit.setConvertObjectBindingIntoArrayBinding(true);
    return visit(qmlDocument->qmlProgram());
}

bool QmlRefactoring::addToObjectMemberList(int parentLocation, const QString &content)
{
    if (parentLocation < 0)
        return false;

    AddObjectVisitor visit(*textModifier, (quint32) parentLocation, content, m_propertyOrder);
    return visit(qmlDocument->qmlProgram());
}

bool QmlRefactoring::addProperty(int parentLocation, const QString &name, const QString &value, PropertyType propertyType)
{
    if (parentLocation < 0)
        return false;

    AddPropertyVisitor visit(*textModifier, (quint32) parentLocation, name, value, propertyType, m_propertyOrder);
    return visit(qmlDocument->qmlProgram());
}

bool QmlRefactoring::changeProperty(int parentLocation, const QString &name, const QString &value, PropertyType propertyType)
{
    if (parentLocation < 0)
        return false;

    ChangePropertyVisitor visit(*textModifier, (quint32) parentLocation, name, value, propertyType);
    return visit(qmlDocument->qmlProgram());
}

bool QmlRefactoring::changeObjectType(int nodeLocation, const QString &newType)
{
    if (nodeLocation < 0 || newType.isEmpty())
        return false;

    ChangeObjectTypeVisitor visit(*textModifier, (quint32) nodeLocation, newType);
    return visit(qmlDocument->qmlProgram());
}

bool QmlRefactoring::moveObject(int objectLocation, const QString &targetPropertyName, bool targetIsArrayBinding, int targetParentObjectLocation)
{
    if (objectLocation < 0 || targetParentObjectLocation < 0)
        return false;

    MoveObjectVisitor visit(*textModifier, (quint32) objectLocation, targetPropertyName, targetIsArrayBinding, (quint32) targetParentObjectLocation, m_propertyOrder);
    return visit(qmlDocument->qmlProgram());
}

bool QmlRefactoring::moveObjectBeforeObject(int movingObjectLocation, int beforeObjectLocation, bool inDefaultProperty)
{
    if (movingObjectLocation < 0 || beforeObjectLocation < -1)
        return false;

    if (beforeObjectLocation == -1) {
        MoveObjectBeforeObjectVisitor visit(*textModifier, movingObjectLocation, inDefaultProperty);
        return visit(qmlDocument->qmlProgram());
    } else {
        MoveObjectBeforeObjectVisitor visit(*textModifier, movingObjectLocation, beforeObjectLocation, inDefaultProperty);
        return visit(qmlDocument->qmlProgram());
    }
    return false;
}

bool QmlRefactoring::removeObject(int nodeLocation)
{
    if (nodeLocation < 0)
        return false;

    RemoveUIObjectMemberVisitor visit(*textModifier, (quint32) nodeLocation);
    return visit(qmlDocument->qmlProgram());
}

bool QmlRefactoring::removeProperty(int parentLocation, const QString &name)
{
    if (parentLocation < 0 || name.isEmpty())
        return false;

    RemovePropertyVisitor visit(*textModifier, (quint32) parentLocation, name);
    return visit(qmlDocument->qmlProgram());
}

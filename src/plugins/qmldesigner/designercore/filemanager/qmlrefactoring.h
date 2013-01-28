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

#ifndef QMLREFACTORING_H
#define QMLREFACTORING_H

#include <import.h>
#include <textmodifier.h>
#include <qmljs/qmljsdocument.h>
#include <QSet>
#include <QString>

namespace QmlDesigner {

class QmlRefactoring
{
    QmlRefactoring(const QmlRefactoring &);
    QmlRefactoring &operator=(const QmlRefactoring &);

public:
    enum PropertyType {
        ArrayBinding = 1,
        ObjectBinding = 2,
        ScriptBinding = 3
    };

public:
    QmlRefactoring(const QmlJS::Document::Ptr &doc, QmlDesigner::TextModifier &modifier, const QStringList &propertyOrder);

    bool reparseDocument();

    bool addImport(const Import &import);
    bool removeImport(const Import &import);

    bool addToArrayMemberList(int parentLocation, const QString &propertyName, const QString &content);
    bool addToObjectMemberList(int parentLocation, const QString &content);
    bool addProperty(int parentLocation, const QString &name, const QString &value, PropertyType propertyType);
    bool changeProperty(int parentLocation, const QString &name, const QString &value, PropertyType propertyType);
    bool changeObjectType(int nodeLocation, const QString &newType);

    bool moveObject(int objectLocation, const QString &targetPropertyName, bool targetIsArray, int targetParentObjectLocation);
    bool moveObjectBeforeObject(int movingObjectLocation, int beforeObjectLocation, bool inDefaultProperty);

    bool removeObject(int nodeLocation);
    bool removeProperty(int parentLocation, const QString &name);

private:
    QmlJS::Document::Ptr qmlDocument;
    TextModifier *textModifier;
    QStringList m_propertyOrder;
};

} // namespace QmlDesigner

#endif // QMLREFACTORING_H

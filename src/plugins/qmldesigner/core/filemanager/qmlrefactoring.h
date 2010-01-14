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

#ifndef QMLREFACTORING_H
#define QMLREFACTORING_H

#include <import.h>
#include <textmodifier.h>
#include <filemanager/qmldocument.h>
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
    QmlRefactoring(const QmlDocument::Ptr &doc, QmlDesigner::TextModifier &modifier, const QStringList &propertyOrder);

    bool reparseDocument();

    bool changeImports(const QSet<QmlDesigner::Import> &addedImports, const QSet<QmlDesigner::Import> &removedImports);

    bool addToArrayMemberList(int parentLocation, const QString &propertyName, const QString &content);
    bool addToObjectMemberList(int parentLocation, const QString &content);
    bool addProperty(int parentLocation, const QString &name, const QString &value, PropertyType propertyType);
    bool changeProperty(int parentLocation, const QString &name, const QString &value, PropertyType propertyType);
    bool changeObjectType(int nodeLocation, const QString &newType);

    bool moveObject(int objectLocation, const QString &targetPropertyName, bool targetIsArray, int targetParentObjectLocation);
    bool moveObjectBeforeObject(int movingObjectLocation, int beforeObjectLocation);

    bool removeObject(int nodeLocation);
    bool removeProperty(int parentLocation, const QString &name);

private:
    QmlDocument::Ptr qmlDocument;
    QmlDesigner::TextModifier *textModifier;
    QStringList m_propertyOrder;
};

} // namespace QmlDesigner

#endif // QMLREFACTORING_H

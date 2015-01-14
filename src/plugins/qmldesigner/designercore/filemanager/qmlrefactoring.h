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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLREFACTORING_H
#define QMLREFACTORING_H

#include <import.h>
#include <textmodifier.h>
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
    QmlRefactoring(const QmlJS::Document::Ptr &doc, QmlDesigner::TextModifier &modifier, const PropertyNameList &propertyOrder);

    bool reparseDocument();

    bool addImport(const Import &import);
    bool removeImport(const Import &import);

    bool addToArrayMemberList(int parentLocation, const PropertyName &propertyName, const QString &content);
    bool addToObjectMemberList(int parentLocation, const QString &content);
    bool addProperty(int parentLocation,
                     const PropertyName &name,
                     const QString &value,
                     PropertyType propertyType,
                     const TypeName &dynamicTypeName = TypeName());
    bool changeProperty(int parentLocation, const PropertyName &name, const QString &value, PropertyType propertyType);
    bool changeObjectType(int nodeLocation, const QString &newType);

    bool moveObject(int objectLocation, const PropertyName &targetPropertyName, bool targetIsArray, int targetParentObjectLocation);
    bool moveObjectBeforeObject(int movingObjectLocation, int beforeObjectLocation, bool inDefaultProperty);

    bool removeObject(int nodeLocation);
    bool removeProperty(int parentLocation, const PropertyName &name);

private:
    QmlJS::Document::Ptr qmlDocument;
    TextModifier *textModifier;
    PropertyNameList m_propertyOrder;
};

} // namespace QmlDesigner

#endif // QMLREFACTORING_H

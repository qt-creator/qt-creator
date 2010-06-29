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

#ifndef QMLJSDELTA_H
#define QMLJSDELTA_H

#include "qmljsprivateapi.h"

#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsastfwd_p.h>

namespace QmlJSInspector {
namespace Internal {

class Delta
{
public:
    void operator()(QmlJS::Document::Ptr doc, QmlJS::Document::Ptr previousDoc);

private:
    void updateScriptBinding(const QDeclarativeDebugObjectReference &objectReference,
                             QmlJS::AST::UiScriptBinding *scriptBinding,
                             const QString &propertyName,
                             const QString &scriptCode);

    bool compare(QmlJS::AST::UiQualifiedId *id, QmlJS::AST::UiQualifiedId *other);
    QmlJS::AST::UiObjectMemberList *objectMembers(QmlJS::AST::UiObjectMember *object);
};

} // namespace Internal
} // namespace QmlJSInspector

#endif // QMLJSDELTA_H


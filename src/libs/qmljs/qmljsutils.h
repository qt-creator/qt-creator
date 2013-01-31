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

#ifndef QMLJS_QMLJSUTILS_H
#define QMLJS_QMLJSUTILS_H

#include "qmljs_global.h"
#include "parser/qmljsastfwd_p.h"
#include "parser/qmljsengine_p.h"

#include <QColor>

namespace QmlJS {

QMLJS_EXPORT QColor toQColor(const QString &qmlColorString);
QMLJS_EXPORT QString toString(AST::UiQualifiedId *qualifiedId,
                              const QChar delimiter = QLatin1Char('.'));

QMLJS_EXPORT AST::SourceLocation locationFromRange(const AST::SourceLocation &start,
                                                   const AST::SourceLocation &end);

QMLJS_EXPORT AST::SourceLocation fullLocationForQualifiedId(AST::UiQualifiedId *);

QMLJS_EXPORT QString idOfObject(AST::Node *object, AST::UiScriptBinding **idBinding = 0);

QMLJS_EXPORT AST::UiObjectInitializer *initializerOfObject(AST::Node *object);

QMLJS_EXPORT AST::UiQualifiedId *qualifiedTypeNameId(AST::Node *node);

QMLJS_EXPORT bool isValidBuiltinPropertyType(const QString &name);

QMLJS_EXPORT DiagnosticMessage errorMessage(const AST::SourceLocation &loc,
                                            const QString &message);

template <class T>
AST::SourceLocation locationFromRange(const T *node)
{
    return locationFromRange(node->firstSourceLocation(), node->lastSourceLocation());
}

template <class T>
DiagnosticMessage errorMessage(const T *node, const QString &message)
{
    return DiagnosticMessage(DiagnosticMessage::Error,
                             locationFromRange(node),
                             message);
}

} // namespace QmlJS

#endif // QMLJS_QMLJSUTILS_H

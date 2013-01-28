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

#ifndef QMLJSCOMPLETIONCONTEXTFINDER_H
#define QMLJSCOMPLETIONCONTEXTFINDER_H

#include "qmljs_global.h"
#include <qmljs/qmljslineinfo.h>

#include <QStringList>
#include <QTextCursor>

namespace QmlJS {

class QMLJS_EXPORT CompletionContextFinder : public LineInfo
{
public:
    CompletionContextFinder(const QTextCursor &cursor);

    QStringList qmlObjectTypeName() const;
    bool isInQmlContext() const;

    bool isInLhsOfBinding() const;
    bool isInRhsOfBinding() const;

    bool isAfterOnInLhsOfBinding() const;
    QStringList bindingPropertyName() const;

    bool isInStringLiteral() const;
    bool isInImport() const;

private:
    int findOpeningBrace(int startTokenIndex);
    void getQmlObjectTypeName(int startTokenIndex);
    void checkBinding();
    void checkImport();

    QTextCursor m_cursor;
    QStringList m_qmlObjectTypeName;
    QStringList m_bindingPropertyName;
    int m_startTokenIndex;
    int m_colonCount;
    bool m_behaviorBinding;
    bool m_inStringLiteral;
    bool m_inImport;
};

} // namespace QmlJS

#endif // QMLJSCOMPLETIONCONTEXTFINDER_H

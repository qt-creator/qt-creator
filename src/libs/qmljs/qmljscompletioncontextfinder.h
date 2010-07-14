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

#ifndef QMLJSCOMPLETIONCONTEXTFINDER_H
#define QMLJSCOMPLETIONCONTEXTFINDER_H

#include "qmljs_global.h"
#include <qmljs/qmljslineinfo.h>

#include <QtCore/QStringList>
#include <QtGui/QTextCursor>

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

private:
    int findOpeningBrace(int startTokenIndex);
    void getQmlObjectTypeName(int startTokenIndex);
    void checkBinding();

    QTextCursor m_cursor;
    QStringList m_qmlObjectTypeName;
    QStringList m_bindingPropertyName;
    int m_startTokenIndex;
    int m_colonCount;
    bool m_behaviorBinding;
};

} // namespace QmlJS

#endif // QMLJSCOMPLETIONCONTEXTFINDER_H

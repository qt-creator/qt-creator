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

#ifndef QMLJSINDENTER_H
#define QMLJSINDENTER_H

#include <qmljs/qmljs_global.h>
#include <qmljs/qmljsscanner.h>
#include <qmljs/qmljslineinfo.h>

#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <QtGui/QTextBlock>

namespace QmlJS {

class QMLJS_EXPORT QmlJSIndenter : public LineInfo
{
    Q_DISABLE_COPY(QmlJSIndenter)

public:
    QmlJSIndenter();
    ~QmlJSIndenter();

    void setTabSize(int size);
    void setIndentSize(int size);

    int indentForBottomLine(QTextBlock firstBlock, QTextBlock lastBlock, QChar typedIn);

private:
    bool isOnlyWhiteSpace(const QString &t) const;
    int columnForIndex(const QString &t, int index) const;
    int indentOfLine(const QString &t) const;

    void eraseChar(QString &t, int k, QChar ch) const;
    QChar lastParen() const;
    bool okay(QChar typedIn, QChar okayCh) const;

    int indentWhenBottomLineStartsInMultiLineComment();
    int indentForContinuationLine();
    int indentForStandaloneLine();

private:
    int ppHardwareTabSize;
    int ppIndentSize;
    int ppContinuationIndentSize;
    int ppCommentOffset;

private:
    QRegExp caseOrDefault;
};

} // namespace QmlJS

#endif // QMLJSINDENTER_H


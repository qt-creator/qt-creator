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

#ifndef QMLJSINDENTER_H
#define QMLJSINDENTER_H

#include <qmljs/qmljs_global.h>
#include <qmljs/qmljslineinfo.h>

#include <QRegExp>

QT_FORWARD_DECLARE_CLASS(QTextBlock)

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


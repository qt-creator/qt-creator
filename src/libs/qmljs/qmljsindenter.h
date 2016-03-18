/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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

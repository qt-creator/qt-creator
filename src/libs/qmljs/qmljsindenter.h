/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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


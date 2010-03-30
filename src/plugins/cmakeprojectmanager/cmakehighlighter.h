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

#ifndef CMAKEHIGHLIGHTER_H
#define CMAKEHIGHLIGHTER_H

#include <QtCore/QtAlgorithms>
#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextCharFormat>

namespace CMakeProjectManager {
namespace Internal {

class CMakeHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    enum CMakeFormats {
        CMakeVariableFormat,
        CMakeFunctionFormat,
        CMakeCommentFormat,
        CMakeStringFormat,
        NumCMakeFormats
    };

    CMakeHighlighter(QTextDocument *document = 0);
    virtual void highlightBlock(const QString &text);

    // Set formats from a sequence of type QTextCharFormat
    template <class InputIterator>
        void setFormats(InputIterator begin, InputIterator end) {
            qCopy(begin, end, m_formats);
        }

private:
    QTextCharFormat m_formats[NumCMakeFormats];
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // PROFILEHIGHLIGHTER_H

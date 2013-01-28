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

#ifndef HTMLDOCEXTRACTOR_H
#define HTMLDOCEXTRACTOR_H

#include "utils_global.h"

#include <QString>

namespace Utils {

class QTCREATOR_UTILS_EXPORT HtmlDocExtractor
{
public:
    HtmlDocExtractor();

    enum Mode {
        FirstParagraph,
        Extended
    };

    void setMode(Mode mode);
    void applyFormatting(const bool format);

    QString getClassOrNamespaceBrief(const QString &html, const QString &mark) const;
    QString getClassOrNamespaceDescription(const QString &html, const QString &mark) const;
    QString getEnumDescription(const QString &html, const QString &mark) const;
    QString getTypedefDescription(const QString &html, const QString &mark) const;
    QString getMacroDescription(const QString &html, const QString &mark) const;
    QString getFunctionDescription(const QString &html,
                                   const QString &mark,
                                   const bool mainOverload = true) const;
    QString getQmlComponentDescription(const QString &html, const QString &mark) const;
    QString getQmlPropertyDescription(const QString &html, const QString &mark) const;
    QString getQMakeVariableOrFunctionDescription(const QString &html,
                                                  const QString &mark) const;
    QString getQMakeFunctionId(const QString &html,
                               const QString &mark) const;

private:
    QString getClassOrNamespaceMemberDescription(const QString &html,
                                                 const QString &startMark,
                                                 const QString &endMark) const;
    QString getContentsByMarks(const QString &html,
                               QString startMark,
                               QString endMark) const;

    void processOutput(QString *html) const;

    static void stripAllHtml(QString *html);
    static void stripHeadings(QString *html);
    static void stripLinks(QString *html);
    static void stripHorizontalLines(QString *html);
    static void stripDivs(QString *html);
    static void stripTagsStyles(QString *html);
    static void stripTeletypes(QString *html);
    static void stripImagens(QString *html);
    static void stripBold(QString *html);
    static void stripEmptyParagraphs(QString *html);
    static void replaceNonStyledHeadingsForBold(QString *html);
    static void replaceTablesForSimpleLines(QString *html);
    static void replaceListsForSimpleLines(QString *html);

    bool m_formatContents;
    Mode m_mode;
};

} // namespace Utils

#endif // HTMLDOCEXTRACTOR_H

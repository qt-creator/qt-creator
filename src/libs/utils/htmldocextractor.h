/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef HTMLDOCEXTRACTOR_H
#define HTMLDOCEXTRACTOR_H

#include "utils_global.h"

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

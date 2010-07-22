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

#ifndef HTMLDOCEXTRACTOR_H
#define HTMLDOCEXTRACTOR_H

#include "utils_global.h"

#include <QtCore/QString>

namespace Utils {

class QTCREATOR_UTILS_EXPORT HtmlDocExtractor
{
public:
    HtmlDocExtractor();

    void extractFirstParagraphOnly();
    void extractExtendedContents(const int lengthReference, const bool truncateAtParagraph);

    void applyFormatting(const bool format);

    QString getClassOrNamespaceBrief(const QString &html, const QString &mark) const;
    QString getClassOrNamespaceDescription(const QString &html, const QString &mark) const;
    QString getEnumDescription(const QString &html, const QString &mark) const;
    QString getTypedefDescription(const QString &html, const QString &mark) const;
    QString getMacroDescription(const QString &html, const QString &mark) const;
    QString getFunctionDescription(const QString &html,
                                   const QString &mark,
                                   const bool mainOverload = true) const;

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

    int m_lengthReference;
    bool m_truncateAtParagraph;
    bool m_formatContents;
    bool m_extendedExtraction;
};

} // namespace Utils

#endif // HTMLDOCEXTRACTOR_H

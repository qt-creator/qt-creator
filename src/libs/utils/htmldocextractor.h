// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

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

    bool m_formatContents = true;
    Mode m_mode = FirstParagraph;
};

} // namespace Utils

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

#include "htmldocextractor.h"

#include <QtCore/QLatin1String>
#include <QtCore/QLatin1Char>
#include <QtCore/QStringList>
#include <QtCore/QRegExp>

using namespace Utils;

namespace {
    QRegExp createMinimalExp(const QString &pattern) {
        QRegExp exp(pattern);
        exp.setMinimal(true);
        return exp;
    }
}

HtmlDocExtractor::HtmlDocExtractor() :
    m_formatContents(true),
    m_mode(FirstParagraph)
{}

void HtmlDocExtractor::setMode(Mode mode)
{ m_mode = mode; }

void HtmlDocExtractor::applyFormatting(const bool format)
{ m_formatContents = format; }

QString HtmlDocExtractor::getClassOrNamespaceBrief(const QString &html, const QString &mark) const
{
    QString contents = getContentsByMarks(html, mark + QLatin1String("-brief"), mark);
    if (!contents.isEmpty() && m_formatContents)
        contents.remove(QLatin1String("<a href=\"#details\">More...</a>"));
    processOutput(&contents);

    return contents;
}

QString HtmlDocExtractor::getClassOrNamespaceDescription(const QString &html,
                                                         const QString &mark) const
{
    if (m_mode == FirstParagraph)
        return getClassOrNamespaceBrief(html, mark);

    QString contents = getContentsByMarks(html, mark + QLatin1String("-description"), mark);
    if (!contents.isEmpty() && m_formatContents)
        contents.remove(QLatin1String("Detailed Description"));
    processOutput(&contents);

    return contents;
}

QString HtmlDocExtractor::getEnumDescription(const QString &html, const QString &mark) const
{
    return getClassOrNamespaceMemberDescription(html, mark, mark);
}

QString HtmlDocExtractor::getTypedefDescription(const QString &html, const QString &mark) const
{
    return getClassOrNamespaceMemberDescription(html, mark, mark);
}

QString HtmlDocExtractor::getMacroDescription(const QString &html,
                                              const QString &mark) const
{
    return getClassOrNamespaceMemberDescription(html, mark, mark);
}

QString HtmlDocExtractor::getFunctionDescription(const QString &html,
                                                 const QString &mark,
                                                 const bool mainOverload) const
{
    QString cleanMark = mark;
    QString startMark = mark;
    const int parenthesis = mark.indexOf(QLatin1Char('('));
    if (parenthesis != -1) {
        startMark = mark.left(parenthesis);
        cleanMark = startMark;
        if (mainOverload) {
            startMark.append(QLatin1String("[overload1]"));
        } else {
            QString complement = mark.right(mark.length() - parenthesis);
            complement.remove(QRegExp(QLatin1String("[\\(\\), ]")));
            startMark.append(complement);
        }
    }

    QString contents = getClassOrNamespaceMemberDescription(html, startMark, cleanMark);
    if (contents.isEmpty()) {
        // Maybe this is a property function, which is documented differently. Besides
        // setX/isX/hasX there are other (not so usual) names for them. A few examples of those:
        //   - toPlainText / Prop. plainText from QPlainTextEdit.
        //   - resize / Prop. size from QWidget.
        //   - move / Prop. pos from QWidget (nothing similar in the names in this case).
        // So I try to find the link to this property in the list of properties, extract its
        // anchor and then follow by the name found.
        const QString &pattern =
            QString(QLatin1String("<a href=\"[a-z\\.]+#([A-Za-z]+)-prop\">%1</a>")).arg(cleanMark);
        QRegExp exp = createMinimalExp(pattern);
        if (exp.indexIn(html) != -1) {
            const QString &prop = exp.cap(1);
            contents = getClassOrNamespaceMemberDescription(html,
                                                            prop + QLatin1String("-prop"),
                                                            prop);
        }
    }

    return contents;
}

QString HtmlDocExtractor::getQMLItemDescription(const QString &html, const QString &mark) const
{
    return getClassOrNamespaceDescription(html, mark);
}

QString HtmlDocExtractor::getClassOrNamespaceMemberDescription(const QString &html,
                                                               const QString &startMark,
                                                               const QString &endMark) const
{
    QString contents = getContentsByMarks(html, startMark, endMark);
    processOutput(&contents);

    return contents;
}

QString HtmlDocExtractor::getContentsByMarks(const QString &html,
                                             QString startMark,
                                             QString endMark) const
{
    startMark.prepend(QLatin1String("$$$"));
    endMark.prepend(QLatin1String("<!-- @@@"));

    QString contents;
    int start = html.indexOf(startMark);
    if (start != -1) {
        start = html.indexOf(QLatin1String("-->"), start);
        if (start != -1) {
            int end = html.indexOf(endMark, start);
            if (end != -1) {
                start += 3;
                contents = html.mid(start, end - start);
            }
        }
    }
    return contents;
}

void HtmlDocExtractor::processOutput(QString *html) const
{
    if (html->isEmpty())
        return;

    if (m_mode == FirstParagraph) {
        int index = html->indexOf(QLatin1String("</p>"));
        if (index > 0) {
            if (html->at(index - 1) == QLatin1Char('.')) {
                index += 4;
                html->truncate(index);
            } else {
                // <p>Paragraphs similar to this. Example:</p>
                index = html->lastIndexOf(QLatin1Char('.'), index);
                if (index > 0) {
                    html->truncate(index);
                    html->append(QLatin1String(".</p>"));
                }
            }
        } else {
            // Some enumerations don't have paragraphs and just a table with the items. In such
            // cases the the html is cleared to avoid showing more that desired.
            html->clear();
            return;
        }
    }

    if (m_formatContents) {
        stripBold(html);
        replaceNonStyledHeadingsForBold(html);
        replaceTablesForSimpleLines(html);
        replaceListsForSimpleLines(html);
        stripLinks(html);
        stripHorizontalLines(html);
        stripDivs(html);
        stripTagsStyles(html);
        stripHeadings(html);
        stripImagens(html);
        stripEmptyParagraphs(html);
    }
}

void HtmlDocExtractor::stripAllHtml(QString *html)
{
    html->remove(createMinimalExp(QLatin1String("<.*>")));
}

void HtmlDocExtractor::stripHeadings(QString *html)
{
    html->remove(createMinimalExp(QLatin1String("<h\\d{1}.*>|</h\\d{1}>")));
}

void HtmlDocExtractor::stripLinks(QString *html)
{
    html->remove(createMinimalExp(QLatin1String("<a\\s+.*>|</a>")));
}

void HtmlDocExtractor::stripHorizontalLines(QString *html)
{
    html->remove(createMinimalExp(QLatin1String("<hr\\s+/>")));
}

void HtmlDocExtractor::stripDivs(QString *html)
{
    html->remove(createMinimalExp(QLatin1String("<div\\s+.*>|</div>|<div\\s+.*/\\s*>")));
}

void HtmlDocExtractor::stripTagsStyles(QString *html)
{
    const QRegExp &exp = createMinimalExp(QLatin1String("<(.*\\s+)class=\".*\">"));
    html->replace(exp, QLatin1String("<\\1>"));
}

void HtmlDocExtractor::stripTeletypes(QString *html)
{
    html->remove(QLatin1String("<tt>"));
    html->remove(QLatin1String("</tt>"));
}

void HtmlDocExtractor::stripImagens(QString *html)
{
    html->remove(createMinimalExp(QLatin1String("<img.*>")));
}

void HtmlDocExtractor::stripBold(QString *html)
{
    html->remove(QLatin1String("<b>"));
    html->remove(QLatin1String("</b>"));
}

void HtmlDocExtractor::stripEmptyParagraphs(QString *html)
{
    html->remove(QLatin1String("<p></p>"));
}

void HtmlDocExtractor::replaceNonStyledHeadingsForBold(QString *html)
{
    const QRegExp &hStart = createMinimalExp(QLatin1String("<h\\d{1}>"));
    const QRegExp &hEnd = createMinimalExp(QLatin1String("</h\\d{1}>"));
    html->replace(hStart, QLatin1String("<p><b>"));
    html->replace(hEnd, QLatin1String("</b></p>"));
}

void HtmlDocExtractor::replaceTablesForSimpleLines(QString *html)
{
    html->replace(createMinimalExp(QLatin1String("(?:<p>)?<table.*>")), QLatin1String("<p>"));
    html->replace(QLatin1String("</table>"), QLatin1String("</p>"));
    html->remove(createMinimalExp(QLatin1String("<thead.*>")));
    html->remove(QLatin1String("</thead>"));
    html->remove(createMinimalExp(QLatin1String("<tfoot.*>")));
    html->remove(QLatin1String("</tfoot>"));
    html->remove(createMinimalExp(QLatin1String("<tr.*><th.*>.*</th></tr>")));
    html->replace(QLatin1String("</td><td"), QLatin1String("</td>&nbsp;<td"));
    html->remove(createMinimalExp(QLatin1String("<td.*><p>")));
    html->remove(createMinimalExp(QLatin1String("<td.*>")));
    html->remove(createMinimalExp(QLatin1String("(?:</p>)?</td>")));
    html->replace(createMinimalExp(QLatin1String("<tr.*>")),
                  QLatin1String("&nbsp;&nbsp;&nbsp;&nbsp;"));
    html->replace(QLatin1String("</tr>"), QLatin1String("<br />"));
}

void HtmlDocExtractor::replaceListsForSimpleLines(QString *html)
{
    html->remove(createMinimalExp(QLatin1String("<(?:ul|ol).*>")));
    html->remove(createMinimalExp(QLatin1String("</(?:ul|ol)>")));
    html->replace(QLatin1String("<li>"), QLatin1String("&nbsp;&nbsp;&nbsp;&nbsp;"));
    html->replace(QLatin1String("</li>"), QLatin1String("<br />"));
}

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

#include "htmldocextractor.h"

#include <QStringList>
#include <QRegExp>

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

QString HtmlDocExtractor::getQmlComponentDescription(const QString &html, const QString &mark) const
{
    return getClassOrNamespaceDescription(html, mark);
}

QString HtmlDocExtractor::getQmlPropertyDescription(const QString &html, const QString &mark) const
{
    QString startMark = QString::fromLatin1("<a name=\"%1-prop\">").arg(mark);
    int index = html.indexOf(startMark);
    if (index == -1) {
        startMark = QString::fromLatin1("<a name=\"%1-signal\">").arg(mark);
        index = html.indexOf(startMark);
    }
    if (index == -1)
        return QString();

    QString contents = html.mid(index + startMark.size());
    index = contents.indexOf(QLatin1String("<div class=\"qmldoc\"><p>"));
    if (index == -1)
        return QString();
    contents = contents.mid(index);
    processOutput(&contents);

    return contents;
}

QString HtmlDocExtractor::getQMakeVariableOrFunctionDescription(const QString &html,
                                                                const QString &mark) const
{
    const QString startMark = QString::fromLatin1("<a name=\"%1\"></a>").arg(mark);
    int index = html.indexOf(startMark);
    if (index == -1)
        return QString();

    QString contents = html.mid(index + startMark.size());
    index = contents.indexOf(QLatin1String("<!-- @@@qmake"));
    if (index == -1)
        return QString();
    contents = contents.left(index);
    processOutput(&contents);

    return contents;
}

QString HtmlDocExtractor::getQMakeFunctionId(const QString &html,
                                             const QString &mark) const
{
    const QString startMark = QString::fromLatin1("<a name=\"%1-").arg(mark);
    const int startIndex = html.indexOf(startMark);
    if (startIndex == -1)
        return QString();

    const int startKeyIndex = html.indexOf(mark, startIndex);

    const QString endMark = QLatin1String("\"></a>");
    const int endKeyIndex = html.indexOf(endMark, startKeyIndex);
    if (endKeyIndex == -1)
        return QString();

    return html.mid(startKeyIndex, endKeyIndex - startKeyIndex);
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
        // Try to get the entire first paragraph, but if one is not found or if its opening
        // tag is not in the very beginning (using an empirical value as the limit) the html
        // is cleared to avoid too much content. In case the first paragraph looks like:
        // <p><i>This is only used on the Maemo platform.</i></p>
        // or: <p><tt>This is used on Windows only.</tt></p>
        // or: <p>[Conditional]</p>
        // include also the next paragraph.
        int index = html->indexOf(QLatin1String("<p>"));
        if (index != -1 && index < 400) {
            if (html->indexOf(QLatin1String("<p><i>")) == index ||
                    html->indexOf(QLatin1String("<p><tt>")) == index ||
                    html->indexOf(QLatin1String("<p>[Conditional]</p>")) == index)
                index = html->indexOf(QLatin1String("<p>"), index + 6); // skip the first paragraph

            index = html->indexOf(QLatin1String("</p>"), index + 3);
            if (index != -1) {
                // Most paragraphs end with a period, but there are cases without punctuation
                // and cases like this: <p>This is a description. Example:</p>
                const int period = html->lastIndexOf(QLatin1Char('.'), index);
                if (period != -1) {
                    html->truncate(period + 1);
                    html->append(QLatin1String("</p>"));
                } else {
                    html->truncate(index + 4);
                }
            } else {
                html->clear();
            }
        } else {
            html->clear();
        }
    }

    if (!html->isEmpty() && m_formatContents) {
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

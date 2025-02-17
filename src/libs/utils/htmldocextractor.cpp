// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "htmldocextractor.h"

#include <QRegularExpression>

namespace Utils {

HtmlDocExtractor::HtmlDocExtractor() = default;

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
            static const QRegularExpression regex("[\\(\\), ]");
            complement.remove(regex);
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
            QString("<a href=\"[a-z\\.]+?#([A-Za-z]+?)-prop\">%1</a>").arg(cleanMark);
        const QRegularExpressionMatch match = QRegularExpression(pattern).match(html);
        if (match.hasMatch()) {
            const QString &prop = match.captured(1);
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
    static const QRegularExpression regexp("<.*?>");
    html->remove(regexp);
}

void HtmlDocExtractor::stripHeadings(QString *html)
{
    static const QRegularExpression regexp("<h\\d{1}.*?>|</h\\d{1}>");
    html->remove(regexp);
}

void HtmlDocExtractor::stripLinks(QString *html)
{
    static const QRegularExpression regexp("<a\\s.*?>|</a>");
    html->remove(regexp);
}

void HtmlDocExtractor::stripHorizontalLines(QString *html)
{
    static const QRegularExpression regexp("<hr\\s+/>");
    html->remove(regexp);
}

void HtmlDocExtractor::stripDivs(QString *html)
{
    static const QRegularExpression regexp("<div\\s.*?>|</div>|<div\\s.*?/\\s*>");
    html->remove(regexp);
}

void HtmlDocExtractor::stripTagsStyles(QString *html)
{
    static const QRegularExpression regexp("<(.*?\\s+)class=\".*?\">");
    html->replace(regexp, "<\\1>");
}

void HtmlDocExtractor::stripTeletypes(QString *html)
{
    html->remove(QLatin1String("<tt>"));
    html->remove(QLatin1String("</tt>"));
}

void HtmlDocExtractor::stripImagens(QString *html)
{
    static const QRegularExpression regexp("<img.*?>");
    html->remove(regexp);
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
    static const QRegularExpression hStart("<h\\d{1}>");
    static const QRegularExpression hEnd("</h\\d{1}>");
    html->replace(hStart, QLatin1String("<p><b>"));
    html->replace(hEnd, QLatin1String("</b></p>"));
}

void HtmlDocExtractor::replaceTablesForSimpleLines(QString *html)
{
    static const QRegularExpression regexp01("(?:<p>)?<table.*?>");
    html->replace(regexp01, QLatin1String("<p>"));
    static const QRegularExpression regexp02("</table>");
    html->replace(regexp02, QLatin1String("</p>"));
    static const QRegularExpression regexp03("<thead.*?>");
    html->remove(regexp03);
    html->remove(QLatin1String("</thead>"));
    static const QRegularExpression regexp04("<tfoot.*?>");
    html->remove(regexp04);
    html->remove(QLatin1String("</tfoot>"));
    static const QRegularExpression regexp05("<tr.*?><th.*?>.*?</th></tr>");
    html->remove(regexp05);
    html->replace(QLatin1String("</td><td"), QLatin1String("</td>&nbsp;<td"));
    static const QRegularExpression regexp06("<td.*?><p>");
    html->remove(regexp06);
    static const QRegularExpression regexp07("<td.*?>");
    html->remove(regexp07);
    static const QRegularExpression regexp08("(?:</p>)?</td>");
    html->remove(regexp08);
    static const QRegularExpression regexp09("<tr.*?>");
    html->replace(regexp09, QLatin1String("&nbsp;&nbsp;&nbsp;&nbsp;"));
    html->replace(QLatin1String("</tr>"), QLatin1String("<br />"));
}

void HtmlDocExtractor::replaceListsForSimpleLines(QString *html)
{
    static const QRegularExpression listStart("<(?:ul|ol).*?>");
    html->remove(listStart);
    static const QRegularExpression listEnd("</(?:ul|ol)>");
    html->remove(listEnd);
    html->replace(QLatin1String("<li>"), QLatin1String("&nbsp;&nbsp;&nbsp;&nbsp;"));
    html->replace(QLatin1String("</li>"), QLatin1String("<br />"));
}

} // namespace Utils

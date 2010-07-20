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
    m_lengthReference(-1),
    m_truncateAtParagraph(false),
    m_formatContents(true)
{}

void HtmlDocExtractor::setLengthReference(const int length, const bool truncateAtParagraph)
{
    m_lengthReference = length;
    m_truncateAtParagraph = truncateAtParagraph;
}

void HtmlDocExtractor::setFormatContents(const bool format)
{ m_formatContents = format; }

QString HtmlDocExtractor::assemble(const QString &elementAttr,
                                   const QString &elementTemplate) const
{
    const QString &cleanAttr = cleanReference(elementAttr);
    return QString(elementTemplate).arg(cleanAttr);
}

QString HtmlDocExtractor::getClassOrNamespaceBrief(const QString &html, const QString &name) const
{
    QString contents = getContentsByMarks(html, name + QLatin1String("-brief"), false);
    if (contents.isEmpty()) {
        QLatin1String pattern("<h1 class=\"title\">.*</p>");
        contents = findByPattern(html, pattern);
        if (!contents.isEmpty())
            contents.remove(QRegExp(QLatin1String("<h1.*</h1>")));
    }
    if (!contents.isEmpty()) {
        contents.remove(QLatin1String("<a href=\"#details\">More...</a>"));
        if (m_formatContents) {
               contents.prepend(QLatin1String("<nobr>"));
               contents.append(QLatin1String("</nobr>"));
        }
    }

    formatContents(&contents);
    return contents;
}

QString HtmlDocExtractor::getClassOrNamespaceDescription(const QString &html,
                                                         const QString &name) const
{
    QString contents = getContentsByMarks(html, name + QLatin1String("-description"), false);
    if (contents.isEmpty()) {
        QLatin1String pattern("<a name=\"details\"></a>.*<hr />.*<hr />");
        contents = findByPattern(html, pattern);
    }
    if (!contents.isEmpty())
        contents.remove(QLatin1String("Detailed Description"));

    formatContents(&contents);
    return contents;
}

QString HtmlDocExtractor::getEnumDescription(const QString &html, const QString &name) const
{
    const QString &enumm = name + QLatin1String("-enum");
    QString contents = getClassOrNamespaceMemberDescription(html, name, enumm, false);
    formatContents(&contents);
    return contents;
}

QString HtmlDocExtractor::getTypedefDescription(const QString &html, const QString &name) const
{
    const QString &typedeff = name + QLatin1String("-typedef");
    QString contents = getClassOrNamespaceMemberDescription(html, name, typedeff, false);
    formatContents(&contents);
    return contents;
}

QString HtmlDocExtractor::getVarDescription(const QString &html, const QString &name) const
{
    const QString &var = name + QLatin1String("-var");
    QString contents = getClassOrNamespaceMemberDescription(html, name, var, false);
    formatContents(&contents);
    return contents;
}

QString HtmlDocExtractor::getMacroDescription(const QString &html,
                                              const QString &mark,
                                              const QString &anchorName) const
{
    QString contents = getClassOrNamespaceMemberDescription(html, mark, anchorName, false, true);
    formatContents(&contents);
    return contents;
}

QString HtmlDocExtractor::getFunctionDescription(const QString &html,
                                                 const QString &mark,
                                                 const QString &anchorName,
                                                 const bool mainOverload) const
{
    QString contents = getClassOrNamespaceMemberDescription(html, mark, anchorName, mainOverload);
    if (contents.isEmpty()) {
        // Maybe marks are not present and/or this is a property. Besides setX/isX/hasX there are
        // other (not so usual) names for property based functions. A few examples of those:
        //   - toPlainText / Prop. plainText from QPlainTextEdit.
        //   - resize / Prop. size from QWidget.
        //   - move / Prop. pos from QWidget (nothing similar in the names in this case).
        // So I try to find the link to this property in the list of properties, extract its
        // anchor and then follow by the name found.
        QString pattern = assemble(anchorName,
                                   QLatin1String("<a href=\"[a-z\\.]+#([A-Za-z]+-prop)\">%1</a>"));
        QRegExp exp = createMinimalExp(pattern);
        if (exp.indexIn(html) != -1) {
            const QString &prop = exp.cap(1);
            contents = getClassOrNamespaceMemberDescription(html, prop, prop, false);
        }
    }
    formatContents(&contents);
    return contents;
}

QString HtmlDocExtractor::getClassOrNamespaceMemberDescription(const QString &html,
                                                               const QString &mark,
                                                               const QString &anchorName,
                                                               const bool mainOverload,
                                                               const bool relaxedMatch) const
{
    // Try with extraction marks (present in newer verions of the docs). If nothing is found try
    // with the anchor.
    QString contents;
    if (!mark.isEmpty())
        contents = getContentsByMarks(html, mark, mainOverload);
    if (contents.isEmpty())
        contents = getContentsByAnchor(html, anchorName, relaxedMatch);

    return contents;
}

QString HtmlDocExtractor::getContentsByAnchor(const QString &html,
                                              const QString &name,
                                              const bool relaxedMatch) const
{
    // This approach is not very accurate.
    QString pattern;
    if (relaxedMatch) {
        pattern = QLatin1String(
            "(?:<h3 class=\"[a-z]+\">)?<a name=\"%1.*(?:<h3 class|<p />|<hr />|</div>)");
    } else {
        // When there are duplicates the HTML generator incrementally appends 'x' to references.
        pattern = QLatin1String(
            "(?:<h3 class=\"[a-z]+\">)?<a name=\"%1x*\">.*(?:<h3 class|<p />|<hr />|</div>)");
    }

    return findByPattern(html, assemble(name, pattern));
}

QString HtmlDocExtractor::getContentsByMarks(const QString &html,
                                             const QString &id,
                                             const bool mainOverload) const
{
    QString endMark;
    QString startMark;
    if (id.contains(QLatin1Char('('))) {
        const int index = id.indexOf(QLatin1Char('('));
        startMark = id.left(index);
        endMark = startMark;
        if (mainOverload) {
            startMark.append(QLatin1String("[overload1]"));
        } else {
            QString complementaryId = id.right(id.length() - index);
            complementaryId.remove(QRegExp(QLatin1String("[\\(\\), ]")));
            startMark.append(complementaryId);
        }
    } else {
        startMark = id;
    }
    startMark.prepend(QLatin1String("$$$"));

    if (endMark.isEmpty()) {
        if (id.contains(QLatin1Char('-'))) {
            const int index = id.indexOf(QLatin1Char('-'));
            endMark = id.left(index);
        } else {
            endMark = id;
        }
    }
    endMark.prepend(QLatin1String("<!-- @@@"));

    return findByMarks(html, startMark, endMark);
}

QString HtmlDocExtractor::findByMarks(const QString &html,
                                      const QString &startMark,
                                      const QString &endMark) const
{
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

QString HtmlDocExtractor::findByPattern(const QString &html, const QString &pattern) const
{
    QRegExp exp(pattern);
    exp.setMinimal(true);
    const int match = exp.indexIn(html);
    if (match != -1)
        return html.mid(match, exp.matchedLength());
    return QString();
}

void HtmlDocExtractor::formatContents(QString *html) const
{
    if (html->isEmpty())
        return;

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
    }

    if (m_lengthReference > -1 && html->length() > m_lengthReference) {
        if (m_truncateAtParagraph) {
            const int nextBegin = html->indexOf(QLatin1String("<p>"), m_lengthReference);
            QRegExp exp = createMinimalExp(QLatin1String("</p>|<br />"));
            const int previousEnd = html->lastIndexOf(exp, m_lengthReference);
            if (nextBegin != -1 && previousEnd != -1)
                html->truncate(qMin(nextBegin, previousEnd + exp.matchedLength()));
            else if (nextBegin != -1 || previousEnd != -1)
                html->truncate((nextBegin != -1? nextBegin : previousEnd + exp.matchedLength()));
        } else {
            html->truncate(m_lengthReference);
        }
        if (m_formatContents) {
            if (html->endsWith(QLatin1String("<br />")))
                html->chop(6);
            html->append(QLatin1String("<p>...</p>"));
        }
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

/*
 @todo: We need to clean the anchor in the same way qtdoc does. Currently, this method is a
 duplicate of HtmlGenerator::cleanRef. It would be good to reuse the same code either by exposing
 parts of qtdocs or by refactoring the behavior to use some Qt component for example.
 */
QString HtmlDocExtractor::cleanReference(const QString &reference)
{
    QString clean;

    if (reference.isEmpty())
        return clean;

    clean.reserve(reference.size() + 20);
    const QChar c = reference[0];
    const uint u = c.unicode();

    if ((u >= QLatin1Char('a') && u <= QLatin1Char('z')) ||
         (u >= QLatin1Char('A') && u <= QLatin1Char('Z')) ||
         (u >= QLatin1Char('0') && u <= QLatin1Char('9'))) {
        clean += c;
    } else if (u == QLatin1Char('~')) {
        clean += QLatin1String("dtor.");
    } else if (u == QLatin1Char('_')) {
        clean += QLatin1String("underscore.");
    } else {
        clean += QLatin1String("A");
    }

    for (int i = 1; i < (int) reference.length(); i++) {
        const QChar c = reference[i];
        const uint u = c.unicode();
        if ((u >= QLatin1Char('a') && u <= QLatin1Char('z')) ||
             (u >= QLatin1Char('A') && u <= QLatin1Char('Z')) ||
             (u >= QLatin1Char('0') && u <= QLatin1Char('9')) || u == QLatin1Char('-') ||
             u == QLatin1Char('_') || u == QLatin1Char(':') || u == QLatin1Char('.')) {
            clean += c;
        } else if (c.isSpace()) {
            clean += QLatin1String("-");
        } else if (u == QLatin1Char('!')) {
            clean += QLatin1String("-not");
        } else if (u == QLatin1Char('&')) {
            clean += QLatin1String("-and");
        } else if (u == QLatin1Char('<')) {
            clean += QLatin1String("-lt");
        } else if (u == QLatin1Char('=')) {
            clean += QLatin1String("-eq");
        } else if (u == QLatin1Char('>')) {
            clean += QLatin1String("-gt");
        } else if (u == QLatin1Char('#')) {
            clean += QLatin1String("#");
        } else {
            clean += QLatin1String("-");
            clean += QString::number((int)u, 16);
        }
    }
    return clean;
}

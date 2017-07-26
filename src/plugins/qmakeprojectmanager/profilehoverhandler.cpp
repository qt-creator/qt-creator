/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "profilehoverhandler.h"
#include "profilecompletionassist.h"
#include "qmakeprojectmanagerconstants.h"

#include <coreplugin/helpmanager.h>
#include <texteditor/texteditor.h>
#include <utils/htmldocextractor.h>

#include <QTextBlock>
#include <QUrl>

using namespace Core;

namespace QmakeProjectManager {
namespace Internal {

ProFileHoverHandler::ProFileHoverHandler()
    : m_keywords(qmakeKeywords())
{
}

void ProFileHoverHandler::identifyMatch(TextEditor::TextEditorWidget *editorWidget, int pos)
{
    m_docFragment.clear();
    m_manualKind = UnknownManual;
    if (!editorWidget->extraSelectionTooltip(pos).isEmpty()) {
        setToolTip(editorWidget->extraSelectionTooltip(pos));
    } else {
        QTextDocument *document = editorWidget->document();
        QTextBlock block = document->findBlock(pos);
        identifyQMakeKeyword(block.text(), pos - block.position());

        if (m_manualKind != UnknownManual) {
            QUrl url(QString::fromLatin1("qthelp://org.qt-project.qmake/qmake/qmake-%1-reference.html#%2")
                     .arg(manualName()).arg(m_docFragment));
            setLastHelpItemIdentified(TextEditor::HelpItem(url.toString(),
                                                           m_docFragment, TextEditor::HelpItem::QMakeVariableOfFunction));
        } else {
            // General qmake manual will be shown outside any function or variable
            setLastHelpItemIdentified(TextEditor::HelpItem(QLatin1String("qmake"),
                                                           TextEditor::HelpItem::Unknown));
        }
    }
}

void ProFileHoverHandler::identifyQMakeKeyword(const QString &text, int pos)
{
    if (text.isEmpty())
        return;

    QString buf;

    for (int i = 0; i < text.length(); ++i) {
        const QChar c = text.at(i);
        bool checkBuffer = false;
        if (c.isLetter() || c == QLatin1Char('_') || c == QLatin1Char('.') || c.isDigit()) {
            buf += c;
            if (i == text.length() - 1)
                checkBuffer = true;
        } else {
            checkBuffer = true;
        }
        if (checkBuffer) {
            if (!buf.isEmpty()) {
                if ((i >= pos) && (i - buf.size() <= pos)) {
                    if (m_keywords.isFunction(buf))
                        identifyDocFragment(FunctionManual, buf);
                    else if (m_keywords.isVariable(buf))
                        identifyDocFragment(VariableManual, buf);
                    break;
                }
                buf.clear();
            } else {
                if (i >= pos)
                    break; // we are after the tooltip pos
            }
            if (c == QLatin1Char('#'))
                break; // comment start
        }
    }
}

QString ProFileHoverHandler::manualName() const
{
    if (m_manualKind == FunctionManual)
        return QLatin1String("function");
    else if (m_manualKind == VariableManual)
        return QLatin1String("variable");
    return QString();
}

void ProFileHoverHandler::identifyDocFragment(ProFileHoverHandler::ManualKind manualKind,
                                        const QString &keyword)
{
    m_manualKind = manualKind;
    m_docFragment = keyword.toLower();
    // Special case: _PRO_FILE_ and _PRO_FILE_PWD_ ids
    // don't have starting and ending '_'.
    if (m_docFragment.startsWith(QLatin1Char('_')))
        m_docFragment = m_docFragment.mid(1);
    if (m_docFragment.endsWith(QLatin1Char('_')))
        m_docFragment = m_docFragment.left(m_docFragment.size() - 1);
    m_docFragment.replace(QLatin1Char('.'), QLatin1Char('-'));
    m_docFragment.replace(QLatin1Char('_'), QLatin1Char('-'));

    if (m_manualKind == FunctionManual) {
        QUrl url(QString::fromLatin1("qthelp://org.qt-project.qmake/qmake/qmake-%1-reference.html").arg(manualName()));
        const QByteArray html = Core::HelpManager::fileData(url);

        Utils::HtmlDocExtractor htmlExtractor;
        htmlExtractor.setMode(Utils::HtmlDocExtractor::FirstParagraph);

        // Document fragment of qmake function is retrieved from docs.
        // E.g. in case of the keyword "find" the document fragment
        // parsed from docs is "find-variablename-substr".
        m_docFragment = htmlExtractor.getQMakeFunctionId(QString::fromUtf8(html), m_docFragment);
    }
}

} // namespace Internal
} // namespace QmakeProjectManager

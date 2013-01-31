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

#include "profilehoverhandler.h"
#include "profileeditor.h"
#include "profilecompletionassist.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/helpmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/helpitem.h>
#include <utils/htmldocextractor.h>

#include <QTextCursor>
#include <QTextBlock>
#include <QUrl>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using namespace Core;

ProFileHoverHandler::ProFileHoverHandler(QObject *parent)
  : BaseHoverHandler(parent),
    m_manualKind(UnknownManual)
{
    ProFileCompletionAssistProvider *pcap
            = ExtensionSystem::PluginManager::instance()->getObject<ProFileCompletionAssistProvider>();
    m_keywords = TextEditor::Keywords(pcap->variables(), pcap->functions(), QMap<QString, QStringList>());
}

ProFileHoverHandler::~ProFileHoverHandler()
{}

bool ProFileHoverHandler::acceptEditor(IEditor *editor)
{
    if (qobject_cast<ProFileEditor *>(editor) != 0)
        return true;
    return false;
}

void ProFileHoverHandler::identifyMatch(TextEditor::ITextEditor *editor, int pos)
{
    m_docFragment = QString();
    m_manualKind = UnknownManual;
    if (ProFileEditorWidget *proFileEditor = qobject_cast<ProFileEditorWidget *>(editor->widget())) {
        if (!proFileEditor->extraSelectionTooltip(pos).isEmpty()) {
            setToolTip(proFileEditor->extraSelectionTooltip(pos));
        } else {
            QTextDocument *document = proFileEditor->document();
            QTextBlock block = document->findBlock(pos);
            identifyQMakeKeyword(block.text(), pos - block.position());

            if (m_manualKind != UnknownManual) {
                QUrl url(QString::fromLatin1("qthelp://com.trolltech.qmake/qdoc/qmake-%1-reference.html#%2")
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
        QUrl url(QString::fromLatin1("qthelp://com.trolltech.qmake/qdoc/qmake-%1-reference.html").arg(manualName()));
        const QByteArray &html = Core::HelpManager::instance()->fileData(url);

        Utils::HtmlDocExtractor htmlExtractor;
        htmlExtractor.setMode(Utils::HtmlDocExtractor::FirstParagraph);

        // Document fragment of qmake function is retrieved from docs.
        // E.g. in case of the keyword "find" the document fragment
        // parsed from docs is "find-variablename-substr".
        m_docFragment = htmlExtractor.getQMakeFunctionId(QString::fromUtf8(html), m_docFragment);
    }
}


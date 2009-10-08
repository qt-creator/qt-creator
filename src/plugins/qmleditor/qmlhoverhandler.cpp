/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmleditor.h"
#include "qmlexpressionundercursor.h"
#include "qmlhoverhandler.h"
#include "qmllookupcontext.h"
#include "qmlresolveexpression.h"
#include "qmlsymbol.h"

#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <debugger/debuggerconstants.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtGui/QToolTip>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>
#include <QtHelp/QHelpEngineCore>

using namespace Core;
using namespace QmlEditor;
using namespace QmlEditor::Internal;

QmlHoverHandler::QmlHoverHandler(QObject *parent)
    : QObject(parent)
    , m_helpEngineNeedsSetup(false)
{
    m_modelManager = ExtensionSystem::PluginManager::instance()->getObject<QmlModelManagerInterface>();

    ICore *core = ICore::instance();
    QFileInfo fi(core->settings()->fileName());
    // FIXME shouldn't the help engine create the directory if it doesn't exist?
    QDir directory(fi.absolutePath()+"/qtcreator");
    if (!directory.exists())
        directory.mkpath(directory.absolutePath());

    m_helpEngine = new QHelpEngineCore(directory.absolutePath()
                                       + QLatin1String("/helpcollection.qhc"), this);
    //m_helpEngine->setAutoSaveFilter(false);
    if (!m_helpEngine->setupData())
        qWarning() << "Could not initialize help engine:" << m_helpEngine->error();
    m_helpEngine->setCurrentFilter(tr("Unfiltered"));
    m_helpEngineNeedsSetup = m_helpEngine->registeredDocumentations().count() == 0;

    // Listen for editor opened events in order to connect to tooltip/helpid requests
    connect(core->editorManager(), SIGNAL(editorOpened(Core::IEditor *)),
            this, SLOT(editorOpened(Core::IEditor *)));
}

void QmlHoverHandler::editorOpened(IEditor *editor)
{
    ScriptEditorEditable *qmlEditor = qobject_cast<ScriptEditorEditable *>(editor);
    if (!qmlEditor)
        return;

    connect(qmlEditor, SIGNAL(tooltipRequested(TextEditor::ITextEditor*, QPoint, int)),
            this, SLOT(showToolTip(TextEditor::ITextEditor*, QPoint, int)));

    connect(qmlEditor, SIGNAL(contextHelpIdRequested(TextEditor::ITextEditor*, int)),
            this, SLOT(updateContextHelpId(TextEditor::ITextEditor*, int)));
}

void QmlHoverHandler::showToolTip(TextEditor::ITextEditor *editor, const QPoint &point, int pos)
{
    if (! editor)
        return;

    ICore *core = ICore::instance();
    const int dbgcontext = core->uniqueIDManager()->uniqueIdentifier(Debugger::Constants::C_GDBDEBUGGER);

    if (core->hasContext(dbgcontext))
        return;

    updateHelpIdAndTooltip(editor, pos);

    if (m_toolTip.isEmpty())
        QToolTip::hideText();
    else {
        const QPoint pnt = point - QPoint(0,
#ifdef Q_WS_WIN
        24
#else
        16
#endif
        );

        QToolTip::showText(pnt, m_toolTip);
    }
}

void QmlHoverHandler::updateContextHelpId(TextEditor::ITextEditor *editor, int pos)
{
    updateHelpIdAndTooltip(editor, pos);
}

static QString buildHelpId(QmlSymbol *symbol)
{
    if (!symbol)
        return QString();

    const QString idTemplate(QLatin1String("QML %1 Element Reference"));

    return idTemplate.arg(symbol->name());
}

void QmlHoverHandler::updateHelpIdAndTooltip(TextEditor::ITextEditor *editor, int pos)
{
    m_helpId.clear();
    m_toolTip.clear();

    if (!m_modelManager)
        return;

    ScriptEditor *scriptEditor = qobject_cast<ScriptEditor *>(editor->widget());
    if (!scriptEditor)
        return;

    const Snapshot documents = m_modelManager->snapshot();
    const QString fileName = editor->file()->fileName();
    QmlDocument::Ptr doc = documents.value(fileName);
    if (!doc)
        return; // nothing to do

    QTextCursor tc(scriptEditor->document());
    tc.setPosition(pos);
    const unsigned lineNumber = tc.block().blockNumber() + 1;

    // We only want to show F1 if the tooltip matches the help id
    bool showF1 = true;

    foreach (const QmlJS::DiagnosticMessage &m, scriptEditor->diagnosticMessages()) {
        if (m.loc.startLine == lineNumber) {
            m_toolTip = m.message;
            showF1 = false;
            break;
        }
    }

    if (m_helpId.isEmpty()) {
        // Move to the end of a qualified name
        bool stop = false;
        while (!stop) {
            const QChar ch = editor->characterAt(tc.position());
            if (ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char('.')) {
                tc.setPosition(tc.position() + 1);
            } else {
                stop = true;
            }
        }

        // Fetch the expression's code
        QmlExpressionUnderCursor expressionUnderCursor;
        expressionUnderCursor(tc, doc);

        QmlLookupContext context(expressionUnderCursor.expressionScopes(), doc, m_modelManager->snapshot());
        QmlResolveExpression resolver(context);
        QmlSymbol *resolvedSymbol = resolver.typeOf(expressionUnderCursor.expressionNode());

        if (resolvedSymbol) {
            m_helpId = buildHelpId(resolvedSymbol);
        }
    }

    if (m_helpEngineNeedsSetup && m_helpEngine->registeredDocumentations().count() > 0) {
        m_helpEngine->setupData();
        m_helpEngineNeedsSetup = false;
    }

    if (!m_toolTip.isEmpty())
        m_toolTip = Qt::escape(m_toolTip);

    if (!m_helpId.isEmpty() && !m_helpEngine->linksForIdentifier(m_helpId).isEmpty()) {
        if (showF1) {
            m_toolTip = QString(QLatin1String("<table><tr><td valign=middle><nobr>%1</td>"
                                              "<td><img src=\":/cppeditor/images/f1.svg\"></td></tr></table>"))
                    .arg(m_toolTip);
        }
        editor->setContextHelpId(m_helpId);
    } else if (!m_toolTip.isEmpty()) {
        m_toolTip = QString(QLatin1String("<nobr>%1")).arg(m_toolTip);
    } else if (!m_helpId.isEmpty()) {
        m_toolTip = QString(QLatin1String("<nobr>No help available for \"%1\"")).arg(m_helpId);
    }
}

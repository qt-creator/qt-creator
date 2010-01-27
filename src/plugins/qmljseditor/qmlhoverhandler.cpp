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

#include "qmljseditor.h"
#include "qmlexpressionundercursor.h"
#include "qmlhoverhandler.h"

#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <debugger/debuggerconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljscheck.h>
#include <qmljs/qmljsinterpreter.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtGui/QToolTip>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>
#include <QtHelp/QHelpEngineCore>

using namespace Core;
using namespace QmlJS;
using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;

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
    QmlJSEditorEditable *qmlEditor = qobject_cast<QmlJSEditorEditable *>(editor);
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

void QmlHoverHandler::updateHelpIdAndTooltip(TextEditor::ITextEditor *editor, int pos)
{
    m_helpId.clear();
    m_toolTip.clear();

    if (!m_modelManager)
        return;

    QmlJSTextEditor *edit = qobject_cast<QmlJSTextEditor *>(editor->widget());
    if (!edit)
        return;

    const Snapshot snapshot = m_modelManager->snapshot();
    SemanticInfo semanticInfo = edit->semanticInfo();
    Document::Ptr qmlDocument = semanticInfo.document;
    if (qmlDocument.isNull())
        return;

    if (m_helpEngineNeedsSetup && m_helpEngine->registeredDocumentations().count() > 0) {
        m_helpEngine->setupData();
        m_helpEngineNeedsSetup = false;
    }

    QTextCursor tc(edit->document());
    tc.setPosition(pos);

    // We only want to show F1 if the tooltip matches the help id
    bool showF1 = true;

    foreach (const QTextEdit::ExtraSelection &sel, edit->extraSelections(TextEditor::BaseTextEditor::CodeWarningsSelection)) {
        if (pos >= sel.cursor.selectionStart() && pos <= sel.cursor.selectionEnd()) {
            showF1 = false;
            m_toolTip = sel.format.toolTip();
        }
    }

    QString symbolName = QLatin1String("<unknown>");
    if (m_helpId.isEmpty()) {
        // Move to the end of a qualified name
        bool stop = false;
        while (!stop) {
            const QChar ch = editor->characterAt(tc.position());
            if (ch.isLetterOrNumber() || ch == QLatin1Char('_')) {
                tc.setPosition(tc.position() + 1);
            } else {
                stop = true;
            }
        }

        // Fetch the expression's code
        QmlExpressionUnderCursor expressionUnderCursor;
        QmlJS::AST::ExpressionNode *expression = expressionUnderCursor(tc);

        AST::UiObjectMember *declaringMember = 0;

        foreach (const Range &range, semanticInfo.ranges) {
            if (pos >= range.begin.position() && pos <= range.end.position()) {
                declaringMember = range.ast;
            }
        }

        Interpreter::Engine interp;
        Bind bind(qmlDocument, snapshot, &interp);
        Interpreter::ObjectValue *scope = bind(declaringMember);
        Check check(&interp);
        const Interpreter::Value *value = check(expression, scope);
        QStringList baseClasses;
        m_toolTip = prettyPrint(value, &interp, &baseClasses);
        foreach (const QString &baseClass, baseClasses) {
            QString helpId = QLatin1String("QML.");
            helpId += baseClass;

            if (! m_helpEngine->linksForIdentifier(helpId).isEmpty()) {
                m_helpId = helpId;
                break;
            }
        }
    }

    if (!m_toolTip.isEmpty())
        m_toolTip = Qt::escape(m_toolTip);

    if (!m_helpId.isEmpty()) {
        if (showF1) {
            m_toolTip = QString::fromUtf8("<table><tr><td valign=middle><nobr>%1</td>"
                                            "<td><img src=\":/cppeditor/images/f1.svg\"></td></tr></table>")
                    .arg(m_toolTip);
        }
        editor->setContextHelpId(m_helpId);
    } else if (!m_toolTip.isEmpty()) {
        m_toolTip = QString::fromUtf8("<nobr>%1").arg(m_toolTip);
    } else if (!m_helpId.isEmpty()) {
        m_toolTip = QString::fromUtf8("<nobr>No help available for \"%1\"").arg(symbolName);
    }
}

QString QmlHoverHandler::prettyPrint(const QmlJS::Interpreter::Value *value, QmlJS::Interpreter::Engine *interp,
                                     QStringList *baseClasses) const
{
    if (!value)
        return QString();

    if (const Interpreter::ObjectValue *objectValue = value->asObjectValue()) {
        do {
            const QString className = objectValue->className();

            if (! className.isEmpty())
                baseClasses->append(className);

            objectValue = objectValue->prototype();
        } while (objectValue);

        if (! baseClasses->isEmpty())
            return baseClasses->first();
    }

    QString typeId = interp->typeId(value);

    if (typeId.isEmpty() || typeId == QLatin1String("undefined"))
        typeId = QLatin1String("Unknown");

    return typeId;
}

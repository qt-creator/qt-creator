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

#include "qmljseditorplugin.h"
#include "qmljshighlighter.h"
#include "qmljseditor.h"
#include "qmljseditorconstants.h"
#include "qmljseditorfactory.h"
#include "qmljscodecompletion.h"
#include "qmljshoverhandler.h"
#include "qmljsmodelmanager.h"
#include "qmlfilewizard.h"
#include "qmljsoutline.h"
#include "qmljspreviewrunner.h"
#include "qmljsquickfix.h"
#include "qmljs/qmljsicons.h"
#include "qmltaskmanager.h"

#include <qmldesigner/qmldesignerconstants.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/taskhub.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textfilewizard.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/completionsupport.h>
#include <utils/qtcassert.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtGui/QMenu>
#include <QtGui/QAction>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSEditor::Constants;

enum {
    QUICKFIX_INTERVAL = 20
};

QmlJSEditorPlugin *QmlJSEditorPlugin::m_instance = 0;

QmlJSEditorPlugin::QmlJSEditorPlugin() :
        m_modelManager(0),
    m_wizard(0),
    m_editor(0),
    m_actionHandler(0)
{
    m_instance = this;

    m_quickFixCollector = 0;
    m_quickFixTimer = new QTimer(this);
    m_quickFixTimer->setInterval(20);
    m_quickFixTimer->setSingleShot(true);
    connect(m_quickFixTimer, SIGNAL(timeout()), this, SLOT(quickFixNow()));
}

QmlJSEditorPlugin::~QmlJSEditorPlugin()
{
    removeObject(m_editor);
    delete m_actionHandler;
    m_instance = 0;
}

bool QmlJSEditorPlugin::initialize(const QStringList & /*arguments*/, QString *error_message)
{
    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/qmljseditor/QmlJSEditor.mimetypes.xml"), error_message))
        return false;

    m_modelManager = new ModelManager(this);
    addAutoReleasedObject(m_modelManager);

    Core::Context context(QmlJSEditor::Constants::C_QMLJSEDITOR_ID,
                          QmlDesigner::Constants::C_QT_QUICK_TOOLS_MENU);

    m_editor = new QmlJSEditorFactory(this);
    addObject(m_editor);

    Core::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);
    wizardParameters.setCategory(QLatin1String(Core::Constants::WIZARD_CATEGORY_QT));
    wizardParameters.setDisplayCategory(QCoreApplication::translate("Core", Core::Constants::WIZARD_TR_CATEGORY_QT));
    wizardParameters.setDescription(tr("Creates a Qt QML file."));
    wizardParameters.setDisplayName(tr("Qt QML File"));
    wizardParameters.setId(QLatin1String("Q.Qml"));
    addAutoReleasedObject(new QmlFileWizard(wizardParameters, core));

    m_actionHandler = new TextEditor::TextEditorActionHandler(QmlJSEditor::Constants::C_QMLJSEDITOR_ID,
          TextEditor::TextEditorActionHandler::Format
        | TextEditor::TextEditorActionHandler::UnCommentSelection
        | TextEditor::TextEditorActionHandler::UnCollapseAll);
    m_actionHandler->initializeActions();

    Core::ActionManager *am =  core->actionManager();
    Core::ActionContainer *contextMenu = am->createMenu(QmlJSEditor::Constants::M_CONTEXT);

    Core::ActionContainer *mtools = am->actionContainer(Core::Constants::M_TOOLS);
    Core::ActionContainer *menuQtQuick = am->createMenu(Constants::M_QTQUICK);
    menuQtQuick->menu()->setTitle(tr("Qt Quick"));
    mtools->addMenu(menuQtQuick);
    m_actionPreview = new QAction("&Preview", this);

    Core::Context toolsMenuContext(QmlDesigner::Constants::C_QT_QUICK_TOOLS_MENU);
    Core::Command *cmd = addToolAction(m_actionPreview,  am, toolsMenuContext,
                   QLatin1String("QtQuick.Preview"), menuQtQuick, tr("Ctrl+Alt+R"));
    connect(cmd->action(), SIGNAL(triggered()), SLOT(openPreview()));
    m_previewRunner = new QmlJSPreviewRunner(this);

    QAction *followSymbolUnderCursorAction = new QAction(tr("Follow Symbol Under Cursor"), this);
    cmd = am->registerAction(followSymbolUnderCursorAction, Constants::FOLLOW_SYMBOL_UNDER_CURSOR, context);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F2));
    connect(followSymbolUnderCursorAction, SIGNAL(triggered()), this, SLOT(followSymbolUnderCursor()));
    contextMenu->addAction(cmd);

    cmd = am->command(TextEditor::Constants::AUTO_INDENT_SELECTION);
    contextMenu->addAction(cmd);

    cmd = am->command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    CodeCompletion *completion = new CodeCompletion(m_modelManager);
    addAutoReleasedObject(completion);

    addAutoReleasedObject(new HoverHandler);

    // Set completion settings and keep them up to date
    TextEditor::TextEditorSettings *textEditorSettings = TextEditor::TextEditorSettings::instance();
    completion->setCompletionSettings(textEditorSettings->completionSettings());
    connect(textEditorSettings, SIGNAL(completionSettingsChanged(TextEditor::CompletionSettings)),
            completion, SLOT(setCompletionSettings(TextEditor::CompletionSettings)));

    error_message->clear();

    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    iconProvider->registerIconOverlayForSuffix(QIcon(":/qmljseditor/images/qmlfile.png"), "qml");

    m_quickFixCollector = new QmlJSQuickFixCollector;
    addAutoReleasedObject(m_quickFixCollector);
    QmlJSQuickFixCollector::registerQuickFixes(this);

    addAutoReleasedObject(new QmlJSOutlineWidgetFactory);

    m_qmlTaskManager = new QmlTaskManager;
    addAutoReleasedObject(m_qmlTaskManager);

    connect(m_modelManager, SIGNAL(documentChangedOnDisk(QmlJS::Document::Ptr)),
            m_qmlTaskManager, SLOT(documentChangedOnDisk(QmlJS::Document::Ptr)));
    connect(m_modelManager, SIGNAL(aboutToRemoveFiles(QStringList)),
            m_qmlTaskManager, SLOT(documentsRemoved(QStringList)));

    return true;
}

void QmlJSEditorPlugin::extensionsInitialized()
{
    ProjectExplorer::TaskHub *taskHub =
        ExtensionSystem::PluginManager::instance()->getObject<ProjectExplorer::TaskHub>();
    taskHub->addCategory(Constants::TASK_CATEGORY_QML, tr("QML"));
}

ExtensionSystem::IPlugin::ShutdownFlag QmlJSEditorPlugin::aboutToShutdown()
{
    delete QmlJS::Icons::instance(); // delete object held by singleton

    return IPlugin::aboutToShutdown();
}

void QmlJSEditorPlugin::openPreview()
{
    Core::EditorManager *em = Core::EditorManager::instance();

    if (em->currentEditor() && em->currentEditor()->id() == Constants::C_QMLJSEDITOR_ID)
        m_previewRunner->run(em->currentEditor()->file()->fileName());

}

void QmlJSEditorPlugin::initializeEditor(QmlJSEditor::Internal::QmlJSTextEditor *editor)
{
    QTC_ASSERT(m_instance, /**/);

    m_actionHandler->setupActions(editor);

    TextEditor::TextEditorSettings::instance()->initializeEditor(editor);

    // auto completion
    connect(editor, SIGNAL(requestAutoCompletion(TextEditor::ITextEditable*, bool)),
            TextEditor::Internal::CompletionSupport::instance(), SLOT(autoComplete(TextEditor::ITextEditable*, bool)));

    // quick fix
    connect(editor, SIGNAL(requestQuickFix(TextEditor::ITextEditable*)),
            this, SLOT(quickFix(TextEditor::ITextEditable*)));
}

void QmlJSEditorPlugin::followSymbolUnderCursor()
{
    Core::EditorManager *em = Core::EditorManager::instance();

    if (QmlJSTextEditor *editor = qobject_cast<QmlJSTextEditor*>(em->currentEditor()->widget()))
        editor->followSymbolUnderCursor();
}

Core::Command *QmlJSEditorPlugin::addToolAction(QAction *a, Core::ActionManager *am,
                                          Core::Context &context, const QString &name,
                                          Core::ActionContainer *c1, const QString &keySequence)
{
    Core::Command *command = am->registerAction(a, name, context);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    c1->addAction(command);
    return command;
}

QmlJSQuickFixCollector *QmlJSEditorPlugin::quickFixCollector() const
{ return m_quickFixCollector; }

void QmlJSEditorPlugin::quickFix(TextEditor::ITextEditable *editable)
{
    m_currentTextEditable = editable;
    quickFixNow();
}

void QmlJSEditorPlugin::quickFixNow()
{
    if (! m_currentTextEditable)
        return;

    Core::EditorManager *em = Core::EditorManager::instance();
    QmlJSTextEditor *currentEditor = qobject_cast<QmlJSTextEditor*>(em->currentEditor()->widget());

    if (QmlJSTextEditor *editor = qobject_cast<QmlJSTextEditor*>(m_currentTextEditable->widget())) {
        if (currentEditor == editor) {
            if (editor->isOutdated()) {
                // qDebug() << "TODO: outdated document" << editor->editorRevision() << editor->semanticInfo().revision();
                // ### FIXME: m_quickFixTimer->start(QUICKFIX_INTERVAL);
                m_quickFixTimer->stop();
            } else {
                TextEditor::Internal::CompletionSupport::instance()->quickFix(m_currentTextEditable);
            }
        }
    }
}

Q_EXPORT_PLUGIN(QmlJSEditorPlugin)

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

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textfilewizard.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/completionsupport.h>
#include <help/helpmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtGui/QAction>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSEditor::Constants;

QmlJSEditorPlugin *QmlJSEditorPlugin::m_instance = 0;

QmlJSEditorPlugin::QmlJSEditorPlugin() :
        m_modelManager(0),
    m_wizard(0),
    m_editor(0),
    m_actionHandler(0),
    m_completion(0)
{
    m_instance = this;
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

    QList<int> context;
    context<< core->uniqueIDManager()->uniqueIdentifier(QmlJSEditor::Constants::C_QMLJSEDITOR_ID);

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

    Core::Command *cmd = 0;

    QAction *followSymbolUnderCursorAction = new QAction(tr("Follow Symbol Under Cursor"), this);
    cmd = am->registerAction(followSymbolUnderCursorAction, Constants::FOLLOW_SYMBOL_UNDER_CURSOR, context);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F2));
    connect(followSymbolUnderCursorAction, SIGNAL(triggered()), this, SLOT(followSymbolUnderCursor()));
    contextMenu->addAction(cmd);

    cmd = am->command(TextEditor::Constants::AUTO_INDENT_SELECTION);
    contextMenu->addAction(cmd);

    cmd = am->command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    m_completion = new CodeCompletion(m_modelManager);
    addAutoReleasedObject(m_completion);

    addAutoReleasedObject(new HoverHandler());

    // Restore settings
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(QLatin1String("CppTools")); // ### FIXME:
    settings->beginGroup(QLatin1String("Completion"));
    const bool caseSensitive = settings->value(QLatin1String("CaseSensitive"), true).toBool();
    m_completion->setCaseSensitivity(caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
    settings->endGroup();
    settings->endGroup();

    error_message->clear();

    return true;
}

void QmlJSEditorPlugin::extensionsInitialized()
{
    //
    // Explicitly register qml.qch if located in creator directory.
    //
    // This is only needed for the creator-qml package, were we
    // want to ship the documentation without a qt development version.
    //

    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    Help::HelpManager *helpManager = pluginManager->getObject<Help::HelpManager>();

    Q_ASSERT(helpManager);

    const QString qmlHelpFile =
            QDir::cleanPath(QCoreApplication::applicationDirPath()
#if defined(Q_OS_MAC)
            + QLatin1String("/../Resources/doc/qml.qch"));
#else
            + QLatin1String("../../share/doc/qtcreator/qml.qch"));
#endif

    helpManager->registerDocumentation(QStringList(qmlHelpFile));
}

void QmlJSEditorPlugin::initializeEditor(QmlJSEditor::Internal::QmlJSTextEditor *editor)
{
    QTC_ASSERT(m_instance, /**/);

    m_actionHandler->setupActions(editor);

    TextEditor::TextEditorSettings::instance()->initializeEditor(editor);

    // auto completion
    connect(editor, SIGNAL(requestAutoCompletion(TextEditor::ITextEditable*, bool)),
            TextEditor::Internal::CompletionSupport::instance(), SLOT(autoComplete(TextEditor::ITextEditable*, bool)));
}

void QmlJSEditorPlugin::followSymbolUnderCursor()
{
    Core::EditorManager *em = Core::EditorManager::instance();

    if (QmlJSTextEditor *editor = qobject_cast<QmlJSTextEditor*>(em->currentEditor()->widget()))
        editor->followSymbolUnderCursor();
}

Q_EXPORT_PLUGIN(QmlJSEditorPlugin)

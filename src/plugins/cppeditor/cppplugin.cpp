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

#include "cppplugin.h"
#include "cppclasswizard.h"
#include "cppeditor.h"
#include "cppeditorconstants.h"
#include "cppeditorenums.h"
#include "cppfilewizard.h"
#include "cpphoverhandler.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/completionsupport.h>
#include <texteditor/fontsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorplugin.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/texteditorconstants.h>
#include <cpptools/cpptoolsconstants.h>

#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtGui/QMenu>
#include <QtGui/QAction>

using namespace CppEditor::Internal;

//////////////////////////// CppEditorFactory /////////////////////////////

CppEditorFactory::CppEditorFactory(CppPlugin *owner) :
    m_kind(QLatin1String(CppEditor::Constants::CPPEDITOR_KIND)),
    m_owner(owner)
{
    m_mimeTypes << QLatin1String(CppEditor::Constants::C_SOURCE_MIMETYPE)
            << QLatin1String(CppEditor::Constants::C_HEADER_MIMETYPE)
            << QLatin1String(CppEditor::Constants::CPP_SOURCE_MIMETYPE)
            << QLatin1String(CppEditor::Constants::CPP_HEADER_MIMETYPE);

#ifndef Q_WS_MAC
    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    Core::MimeDatabase *mimeDatabase = Core::ICore::instance()->mimeDatabase();
    iconProvider->registerIconOverlayForMimeType(QIcon(":/cppeditor/images/qt_cpp.png"),
                                                 mimeDatabase->findByType(QLatin1String(CppEditor::Constants::CPP_SOURCE_MIMETYPE)));
    iconProvider->registerIconOverlayForMimeType(QIcon(":/cppeditor/images/qt_c.png"),
                                                 mimeDatabase->findByType(QLatin1String(CppEditor::Constants::C_SOURCE_MIMETYPE)));
    iconProvider->registerIconOverlayForMimeType(QIcon(":/cppeditor/images/qt_h.png"),
                                                 mimeDatabase->findByType(QLatin1String(CppEditor::Constants::CPP_HEADER_MIMETYPE)));
#endif
}

QString CppEditorFactory::kind() const
{
    return m_kind;
}

Core::IFile *CppEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, kind());
    return iface ? iface->file() : 0;
}

Core::IEditor *CppEditorFactory::createEditor(QWidget *parent)
{
    CPPEditor *editor = new CPPEditor(parent);
    editor->setRevisionsVisible(true);
    editor->setMimeType(CppEditor::Constants::CPP_SOURCE_MIMETYPE);
    m_owner->initializeEditor(editor);
    return editor->editableInterface();
}

QStringList CppEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

///////////////////////////////// CppPlugin //////////////////////////////////

CppPlugin *CppPlugin::m_instance = 0;

CppPlugin::CppPlugin() :
    m_actionHandler(0),
    m_sortedMethodOverview(false)
{
    m_instance = this;
}

CppPlugin::~CppPlugin()
{
    delete m_actionHandler;
    m_instance = 0;
}

CppPlugin *CppPlugin::instance()
{
    return m_instance;
}

void CppPlugin::initializeEditor(CPPEditor *editor)
{
    m_actionHandler->setupActions(editor);

    TextEditor::TextEditorSettings::instance()->initializeEditor(editor);

    // auto completion
    connect(editor, SIGNAL(requestAutoCompletion(ITextEditable*, bool)),
            TextEditor::Internal::CompletionSupport::instance(), SLOT(autoComplete(ITextEditable*, bool)));

    // quick fix
    connect(editor, SIGNAL(requestQuickFix(ITextEditable*)),
            TextEditor::Internal::CompletionSupport::instance(), SLOT(quickFix(ITextEditable*)));

    // method combo box sorting
    connect(this, SIGNAL(methodOverviewSortingChanged(bool)),
            editor, SLOT(setSortedMethodOverview(bool)));
}

void CppPlugin::setSortedMethodOverview(bool sorted)
{
    m_sortedMethodOverview = sorted;
    emit methodOverviewSortingChanged(sorted);
}

bool CppPlugin::sortedMethodOverview() const
{
    return m_sortedMethodOverview;
}

bool CppPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    Core::ICore *core = Core::ICore::instance();

    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/cppeditor/CppEditor.mimetypes.xml"), errorMessage))
        return false;

    addAutoReleasedObject(new CppEditorFactory(this));
    addAutoReleasedObject(new CppHoverHandler);

    CppFileWizard::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);

    wizardParameters.setCategory(QLatin1String("C++"));
    wizardParameters.setTrCategory(tr("C++"));
    wizardParameters.setDescription(tr("Creates a C++ header file."));
    wizardParameters.setName(tr("C++ Header File"));
    addAutoReleasedObject(new CppFileWizard(wizardParameters, Header, core));

    wizardParameters.setDescription(tr("Creates a C++ source file."));
    wizardParameters.setName(tr("C++ Source File"));
    addAutoReleasedObject(new CppFileWizard(wizardParameters, Source, core));

    wizardParameters.setKind(Core::IWizard::ClassWizard);
    wizardParameters.setName(tr("C++ Class"));
    wizardParameters.setDescription(tr("Creates a header and a source file for a new class."));
    addAutoReleasedObject(new CppClassWizard(wizardParameters, core));

    QList<int> context;
    context << core->uniqueIDManager()->uniqueIdentifier(CppEditor::Constants::C_CPPEDITOR);

    Core::ActionManager *am = core->actionManager();
    am->createMenu(CppEditor::Constants::M_CONTEXT);

    Core::Command *cmd;

    QAction *jumpToDefinition = new QAction(tr("Follow Symbol under Cursor"), this);
    cmd = am->registerAction(jumpToDefinition,
        Constants::JUMP_TO_DEFINITION, context);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F2));
    connect(jumpToDefinition, SIGNAL(triggered()),
            this, SLOT(jumpToDefinition()));
    am->actionContainer(CppEditor::Constants::M_CONTEXT)->addAction(cmd);
    am->actionContainer(CppTools::Constants::M_TOOLS_CPP)->addAction(cmd);

    QAction *switchDeclarationDefinition = new QAction(tr("Switch between Method Declaration/Definition"), this);
    cmd = am->registerAction(switchDeclarationDefinition,
        Constants::SWITCH_DECLARATION_DEFINITION, context);
    cmd->setDefaultKeySequence(QKeySequence("Shift+F2"));
    connect(switchDeclarationDefinition, SIGNAL(triggered()),
            this, SLOT(switchDeclarationDefinition()));
    am->actionContainer(CppEditor::Constants::M_CONTEXT)->addAction(cmd);
    am->actionContainer(CppTools::Constants::M_TOOLS_CPP)->addAction(cmd);

    QAction *renameSymbolUnderCursorAction = new QAction(tr("Rename Symbol under Cursor"), this);
    cmd = am->registerAction(renameSymbolUnderCursorAction,
        Constants::RENAME_SYMBOL_UNDER_CURSOR, context);
    cmd->setDefaultKeySequence(QKeySequence("CTRL+SHIFT+R"));
    connect(renameSymbolUnderCursorAction, SIGNAL(triggered()), this, SLOT(renameSymbolUnderCursor()));
    am->actionContainer(CppEditor::Constants::M_CONTEXT)->addAction(cmd);
    am->actionContainer(CppTools::Constants::M_TOOLS_CPP)->addAction(cmd);

    if (! qgetenv("QTCREATOR_REFERENCES").isEmpty()) {
        QAction *findReferencesAction = new QAction(tr("Find References"), this);
        cmd = am->registerAction(findReferencesAction,
                                 Constants::FIND_REFERENCES, context);
        connect(findReferencesAction, SIGNAL(triggered()), this, SLOT(findReferences()));
        am->actionContainer(CppEditor::Constants::M_CONTEXT)->addAction(cmd);
        am->actionContainer(CppTools::Constants::M_TOOLS_CPP)->addAction(cmd);
    }

    m_actionHandler = new TextEditor::TextEditorActionHandler(CppEditor::Constants::C_CPPEDITOR,
        TextEditor::TextEditorActionHandler::Format
        | TextEditor::TextEditorActionHandler::UnCommentSelection
        | TextEditor::TextEditorActionHandler::UnCollapseAll);

    m_actionHandler->initializeActions();

    cmd = am->command(TextEditor::Constants::AUTO_INDENT_SELECTION);
    am->actionContainer(CppEditor::Constants::M_CONTEXT)->addAction(cmd);

    cmd = am->command(TextEditor::Constants::UN_COMMENT_SELECTION);
    am->actionContainer(CppEditor::Constants::M_CONTEXT)->addAction(cmd);


    readSettings();
    return true;
}

void CppPlugin::readSettings()
{
    m_sortedMethodOverview = Core::ICore::instance()->settings()->value("CppTools/SortedMethodOverview", false).toBool();
}

void CppPlugin::writeSettings()
{
    Core::ICore::instance()->settings()->setValue("CppTools/SortedMethodOverview", m_sortedMethodOverview);
}

void CppPlugin::extensionsInitialized()
{
}

void CppPlugin::shutdown()
{
    writeSettings();
}

void CppPlugin::switchDeclarationDefinition()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditor *editor = qobject_cast<CPPEditor*>(em->currentEditor()->widget());
    if (editor)
        editor->switchDeclarationDefinition();
}

void CppPlugin::jumpToDefinition()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditor *editor = qobject_cast<CPPEditor*>(em->currentEditor()->widget());
    if (editor)
        editor->jumpToDefinition();
}

void CppPlugin::renameSymbolUnderCursor()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditor *editor = qobject_cast<CPPEditor*>(em->currentEditor()->widget());
    if (editor)
        editor->renameSymbolUnderCursor();
}

void CppPlugin::findReferences()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditor *editor = qobject_cast<CPPEditor*>(em->currentEditor()->widget());
    if (editor)
        editor->findReferences();
}

Q_EXPORT_PLUGIN(CppPlugin)

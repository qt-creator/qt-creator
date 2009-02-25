/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cppplugin.h"
#include "cppclasswizard.h"
#include "cppeditor.h"
#include "cppeditoractionhandler.h"
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
#include <texteditor/texteditorsettings.h>
#include <cpptools/cpptoolsconstants.h>

#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtGui/QMenu>
#include <QtGui/QAction>

static const char *headerSuffixKeyC = "CppEditor/HeaderSuffix";
static const char *sourceSuffixKeyC = "CppEditor/SourceSuffix";

using namespace CppEditor::Internal;

//////////////////////////// CppPluginEditorFactory /////////////////////////////

CppPluginEditorFactory::CppPluginEditorFactory(CppPlugin *owner) :
    m_kind(QLatin1String(CppEditor::Constants::CPPEDITOR_KIND)),
    m_owner(owner)
{
    m_mimeTypes << QLatin1String(CppEditor::Constants::C_SOURCE_MIMETYPE)
        << QLatin1String(CppEditor::Constants::C_HEADER_MIMETYPE)
        << QLatin1String(CppEditor::Constants::CPP_SOURCE_MIMETYPE)
        << QLatin1String(CppEditor::Constants::CPP_HEADER_MIMETYPE);
    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    iconProvider->registerIconOverlayForSuffix(QIcon(":/cppeditor/images/qt_cpp.png"),
                                        QLatin1String("cpp"));
    iconProvider->registerIconOverlayForSuffix(QIcon(":/cppeditor/images/qt_h.png"),
                                        QLatin1String("h"));
}

QString CppPluginEditorFactory::kind() const
{
    return m_kind;
}

Core::IFile *CppPluginEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, kind());
    return iface ? iface->file() : 0;
}

Core::IEditor *CppPluginEditorFactory::createEditor(QWidget *parent)
{
    CPPEditor *editor = new CPPEditor(parent);
    editor->setRevisionsVisible(true);
    editor->setMimeType(CppEditor::Constants::CPP_SOURCE_MIMETYPE);
    m_owner->initializeEditor(editor);
    return editor->editableInterface();
}

QStringList CppPluginEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

///////////////////////////////// CppPlugin //////////////////////////////////

CppPlugin *CppPlugin::m_instance = 0;

CppPlugin::CppPlugin() :
    m_actionHandler(0),
    m_factory(0)
{
    m_instance = this;
}

CppPlugin::~CppPlugin()
{
    removeObject(m_factory);
    delete m_factory;
    delete m_actionHandler;
    m_instance = 0;
}

CppPlugin *CppPlugin::instance()
{
    return m_instance;
}

void CppPlugin::initializeEditor(CPPEditor *editor)
{
    // common actions
    m_actionHandler->setupActions(editor);

    // settings
    TextEditor::TextEditorSettings *settings = TextEditor::TextEditorSettings::instance();
    connect(settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            editor, SLOT(setFontSettings(TextEditor::FontSettings)));
    connect(settings, SIGNAL(tabSettingsChanged(TextEditor::TabSettings)),
            editor, SLOT(setTabSettings(TextEditor::TabSettings)));
    connect(settings, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)),
            editor, SLOT(setStorageSettings(TextEditor::StorageSettings)));
    connect(settings, SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)),
            editor, SLOT(setDisplaySettings(TextEditor::DisplaySettings)));

    // tab settings rely on font settings
    editor->setFontSettings(settings->fontSettings());
    editor->setTabSettings(settings->tabSettings());
    editor->setStorageSettings(settings->storageSettings());
    editor->setDisplaySettings(settings->displaySettings());

    // auto completion
    connect(editor, SIGNAL(requestAutoCompletion(ITextEditable*, bool)),
            TextEditor::Internal::CompletionSupport::instance(), SLOT(autoComplete(ITextEditable*, bool)));
}

bool CppPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/cppeditor/CppEditor.mimetypes.xml"), errorMessage))
        return false;

    m_factory = new CppPluginEditorFactory(this);
    addObject(m_factory);

    addAutoReleasedObject(new CppHoverHandler);

    CppFileWizard::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);

    wizardParameters.setCategory(QLatin1String("C++"));
    wizardParameters.setTrCategory(tr("C++"));
    wizardParameters.setDescription(tr("Creates a new C++ header file."));
    wizardParameters.setName(tr("C++ Header File"));
    addAutoReleasedObject(new CppFileWizard(wizardParameters, Header, core));

    wizardParameters.setDescription(tr("Creates a new C++ source file."));
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

    m_actionHandler = new CPPEditorActionHandler(CppEditor::Constants::C_CPPEDITOR,
        TextEditor::TextEditorActionHandler::Format
        | TextEditor::TextEditorActionHandler::UnCommentSelection
        | TextEditor::TextEditorActionHandler::UnCollapseAll);

    // Check Suffixes
    if (const QSettings *settings = core->settings()) {
        const QString headerSuffixKey = QLatin1String(headerSuffixKeyC);
        if (settings->contains(headerSuffixKey)) {
            const QString headerSuffix = settings->value(headerSuffixKey, QString()).toString();
            if (!headerSuffix.isEmpty())
                core->mimeDatabase()->setPreferredSuffix(QLatin1String(Constants::CPP_HEADER_MIMETYPE), headerSuffix);
            const QString sourceSuffix = settings->value(QLatin1String(sourceSuffixKeyC), QString()).toString();
            if (!sourceSuffix.isEmpty())
                core->mimeDatabase()->setPreferredSuffix(QLatin1String(Constants::CPP_SOURCE_MIMETYPE), sourceSuffix);
        }
    }
    return true;
}

void CppPlugin::extensionsInitialized()
{
    m_actionHandler->initializeActions();
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

Q_EXPORT_PLUGIN(CppPlugin)

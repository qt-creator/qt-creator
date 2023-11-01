// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formeditorplugin.h"

#include "designerconstants.h"
#include "designertr.h"
#include "formeditorfactory.h"
#include "formeditor.h"
#include "formtemplatewizardpage.h"

#ifdef CPP_ENABLED
#  include "cpp/formclasswizard.h"
#endif

#include "settingspage.h"
#include "qtdesignerformclasscodegenerator.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/designmode.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <cppeditor/cppeditorconstants.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <utils/mimeutils.h>

#include <QAction>
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QMenu>
#include <QTranslator>

using namespace Core;
using namespace Designer::Constants;
using namespace Utils;

namespace Designer {
namespace Internal {

class FormEditorPluginPrivate
{
public:
    QAction actionSwitchSource{Tr::tr("Switch Source/Form"), nullptr};

    FormEditorFactory formEditorFactory;
    SettingsPageProvider settingsPageProvider;
    QtDesignerFormClassCodeGenerator formClassCodeGenerator;
    FormPageFactory formPageFactory;
};

FormEditorPlugin::~FormEditorPlugin()
{
    deleteInstance();
    delete d;
}

static void parseArguments(const QStringList &arguments)
{
    const auto doWithNext = [arguments](auto it, const std::function<void(QString)> &fun) {
        ++it;
        if (it != arguments.cend())
            fun(*it);
    };
    for (auto it = arguments.cbegin(); it != arguments.cend(); ++it) {
        if (*it == "-designer-qt-pluginpath")
            doWithNext(it, [](const QString &path) { setQtPluginPath(path); });
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
        // -designer-pluginpath is only supported when building with Qt >= 6.7.0, which added the
        // required API
        else if (*it == "-designer-pluginpath")
            doWithNext(it, [](const QString &path) { addPluginPath(path); });
#endif
    }
}

bool FormEditorPlugin::initialize([[maybe_unused]] const QStringList &arguments,
                                  [[maybe_unused]] QString *errorString)
{
    d = new FormEditorPluginPrivate;

#ifdef CPP_ENABLED
    IWizardFactory::registerFactoryCreator([]() -> IWizardFactory * {
        IWizardFactory *wizard = new FormClassWizard;
        wizard->setCategory(Core::Constants::WIZARD_CATEGORY_QT);
        wizard->setDisplayCategory(::Core::Tr::tr(Core::Constants::WIZARD_TR_CATEGORY_QT));
        wizard->setDisplayName(Tr::tr("Qt Designer Form Class"));
        wizard->setIcon({}, "ui/h");
        wizard->setId("C.FormClass");
        wizard->setDescription(Tr::tr("Creates a Qt Designer form along with a matching class (C++ header and source file) "
        "for implementation purposes. You can add the form and class to an existing Qt Widget Project."));

        return wizard;
    });
#endif

    // Ensure that loading designer translations is done before FormEditorW is instantiated
    const QString locale = ICore::userInterfaceLanguage();
    if (!locale.isEmpty()) {
        auto qtr = new QTranslator(this);
        const QString creatorTrPath = ICore::resourcePath("translations").toString();
        const QString qtTrPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
        const QString trFile = "designer_" + locale;
        if (qtr->load(trFile, qtTrPath) || qtr->load(trFile, creatorTrPath))
            QCoreApplication::installTranslator(qtr);
    }

    parseArguments(arguments);
    return true;
}

void FormEditorPlugin::extensionsInitialized()
{
    DesignMode::setDesignModeIsRequired();
    // 4) test and make sure everything works (undo, saving, editors, opening/closing multiple files, dirtiness etc)

    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    ActionContainer *mformtools = ActionManager::createMenu(M_FORMEDITOR);
    mformtools->menu()->setTitle(Tr::tr("For&m Editor"));
    mtools->addMenu(mformtools);

    connect(&d->actionSwitchSource, &QAction::triggered, this, &FormEditorPlugin::switchSourceForm);
    Context context(C_FORMEDITOR, Core::Constants::C_EDITORMANAGER);
    Command *cmd = ActionManager::registerAction(&d->actionSwitchSource,
                                                             "FormEditor.FormSwitchSource", context);
    cmd->setDefaultKeySequence(Tr::tr("Shift+F4"));
    mformtools->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
}

////////////////////////////////////////////////////
//
// PRIVATE functions
//
////////////////////////////////////////////////////

// Find out current existing editor file
static FilePath currentFile()
{
    if (const IDocument *document = EditorManager::currentDocument()) {
        const FilePath filePath = document->filePath();
        if (!filePath.isEmpty() && filePath.isFile())
            return filePath;
    }
    return {};
}

// Switch between form ('ui') and source file ('cpp'):
// Find corresponding 'other' file, simply assuming it is in the same directory.
static FilePath otherFile()
{
    // Determine mime type of current file.
    const FilePath current = currentFile();
    if (current.isEmpty())
        return {};
    const Utils::MimeType currentMimeType = Utils::mimeTypeForFile(current);
    // Determine potential suffixes of candidate files
    // 'ui' -> 'cpp', 'cpp/h' -> 'ui'.
    QStringList candidateSuffixes;
    if (currentMimeType.matchesName(FORM_MIMETYPE)) {
        candidateSuffixes += Utils::mimeTypeForName(CppEditor::Constants::CPP_SOURCE_MIMETYPE).suffixes();
    } else if (currentMimeType.matchesName(CppEditor::Constants::CPP_SOURCE_MIMETYPE)
               || currentMimeType.matchesName(CppEditor::Constants::CPP_HEADER_MIMETYPE)) {
        candidateSuffixes += Utils::mimeTypeForName(FORM_MIMETYPE).suffixes();
    } else {
        return {};
    }
    // Try to find existing file with desired suffix
    const FilePath currentBaseName = current.parentDir().pathAppended(current.baseName() + '.');
    for (const QString &candidateSuffix : std::as_const(candidateSuffixes)) {
        const FilePath filePath = currentBaseName.stringAppended(candidateSuffix);
        if (filePath.isFile())
            return filePath.absoluteFilePath();
    }
    return {};
}

void FormEditorPlugin::switchSourceForm()
{
    const FilePath fileToOpen = otherFile();
    if (!fileToOpen.isEmpty())
        EditorManager::openEditor(fileToOpen);
}

} // Internal
} // Designer

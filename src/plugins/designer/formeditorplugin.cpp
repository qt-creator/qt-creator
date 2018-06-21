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

#include "formeditorplugin.h"
#include "formeditorfactory.h"
#include "formeditorw.h"
#include "formtemplatewizardpage.h"
#include "formwindoweditor.h"

#ifdef CPP_ENABLED
#  include "cpp/formclasswizard.h"
#endif

#include "settingspage.h"
#include "qtdesignerformclasscodegenerator.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <cpptools/cpptoolsconstants.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QAction>
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QMenu>
#include <QTranslator>

using namespace Core;
using namespace Designer::Constants;

namespace Designer {
namespace Internal {

class FormEditorPluginPrivate
{
public:
    QAction actionSwitchSource{FormEditorPlugin::tr("Switch Source/Form"), nullptr};

    FormEditorFactory formEditorFactory;
    SettingsPageProvider settingsPageProvider;
    QtDesignerFormClassCodeGenerator formClassCodeGenerator;
};

FormEditorPlugin::~FormEditorPlugin()
{
    FormEditorW::deleteInstance();
    delete d;
}

bool FormEditorPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments)

    d = new FormEditorPluginPrivate;

#ifdef CPP_ENABLED
    IWizardFactory::registerFactoryCreator(
                []() -> QList<IWizardFactory *> {
                    IWizardFactory *wizard = new FormClassWizard;
                    wizard->setCategory(Core::Constants::WIZARD_CATEGORY_QT);
                    wizard->setDisplayCategory(QCoreApplication::translate("Core", Core::Constants::WIZARD_TR_CATEGORY_QT));
                    wizard->setDisplayName(tr("Qt Designer Form Class"));
                    wizard->setIconText("ui/h");
                    wizard->setId("C.FormClass");
                    wizard->setDescription(tr("Creates a Qt Designer form along with a matching class (C++ header and source file) "
                    "for implementation purposes. You can add the form and class to an existing Qt Widget Project."));

                    return QList<IWizardFactory *>() << wizard;
                });
#endif

    ProjectExplorer::JsonWizardFactory::registerPageFactory(new Internal::FormPageFactory);

    // Ensure that loading designer translations is done before FormEditorW is instantiated
    const QString locale = ICore::userInterfaceLanguage();
    if (!locale.isEmpty()) {
        QTranslator *qtr = new QTranslator(this);
        const QString &creatorTrPath = ICore::resourcePath() + "/translations";
        const QString &qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        const QString &trFile = "designer_" + locale;
        if (qtr->load(trFile, qtTrPath) || qtr->load(trFile, creatorTrPath))
            QCoreApplication::installTranslator(qtr);
    }
    error->clear();
    return true;
}

void FormEditorPlugin::extensionsInitialized()
{
    DesignMode::setDesignModeIsRequired();
    // 4) test and make sure everything works (undo, saving, editors, opening/closing multiple files, dirtiness etc)

    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    ActionContainer *mformtools = ActionManager::createMenu(M_FORMEDITOR);
    mformtools->menu()->setTitle(tr("For&m Editor"));
    mtools->addMenu(mformtools);

    connect(&d->actionSwitchSource, &QAction::triggered, this, &FormEditorPlugin::switchSourceForm);
    Context context(C_FORMEDITOR, Core::Constants::C_EDITORMANAGER);
    Command *cmd = ActionManager::registerAction(&d->actionSwitchSource,
                                                             "FormEditor.FormSwitchSource", context);
    cmd->setDefaultKeySequence(tr("Shift+F4"));
    mformtools->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
}

////////////////////////////////////////////////////
//
// PRIVATE functions
//
////////////////////////////////////////////////////

// Find out current existing editor file
static QString currentFile()
{
    if (const IDocument *document = EditorManager::currentDocument()) {
        const QString fileName = document->filePath().toString();
        if (!fileName.isEmpty() && QFileInfo(fileName).isFile())
            return fileName;
    }
    return QString();
}

// Switch between form ('ui') and source file ('cpp'):
// Find corresponding 'other' file, simply assuming it is in the same directory.
static QString otherFile()
{
    // Determine mime type of current file.
    const QString current = currentFile();
    if (current.isEmpty())
        return QString();
    const Utils::MimeType currentMimeType = Utils::mimeTypeForFile(current);
    if (!currentMimeType.isValid())
        return QString();
    // Determine potential suffixes of candidate files
    // 'ui' -> 'cpp', 'cpp/h' -> 'ui'.
    QStringList candidateSuffixes;
    if (currentMimeType.matchesName(FORM_MIMETYPE)) {
        candidateSuffixes += Utils::mimeTypeForName(CppTools::Constants::CPP_SOURCE_MIMETYPE).suffixes();
    } else if (currentMimeType.matchesName(CppTools::Constants::CPP_SOURCE_MIMETYPE)
               || currentMimeType.matchesName(CppTools::Constants::CPP_HEADER_MIMETYPE)) {
        candidateSuffixes += Utils::mimeTypeForName(FORM_MIMETYPE).suffixes();
    } else {
        return QString();
    }
    // Try to find existing file with desired suffix
    const QFileInfo currentFI(current);
    const QString currentBaseName = currentFI.path() + '/' + currentFI.baseName() + '.';
    for (const QString &candidateSuffix : qAsConst(candidateSuffixes)) {
        const QFileInfo fi(currentBaseName + candidateSuffix);
        if (fi.isFile())
            return fi.absoluteFilePath();
    }
    return QString();
}

void FormEditorPlugin::switchSourceForm()
{
    const QString fileToOpen = otherFile();
    if (!fileToOpen.isEmpty())
        EditorManager::openEditor(fileToOpen);
}

} // Internal
} // Designer

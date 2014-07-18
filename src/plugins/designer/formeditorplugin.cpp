/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "formeditorplugin.h"
#include "formeditorfactory.h"
#include "formeditorw.h"
#include "formwizard.h"

#ifdef CPP_ENABLED
#  include "cpp/formclasswizard.h"
#endif

#include "settingspage.h"
#include "qtdesignerformclasscodegenerator.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <cpptools/cpptoolsconstants.h>

#include <QApplication>
#include <QDebug>
#include <QLibraryInfo>
#include <QTranslator>
#include <QtPlugin>

using namespace Core;
using namespace Designer::Internal;
using namespace Designer::Constants;

FormEditorPlugin::FormEditorPlugin()
    : m_actionSwitchSource(new QAction(tr("Switch Source/Form"), this))
{
}

FormEditorPlugin::~FormEditorPlugin()
{
    FormEditorW::deleteInstance();
}

////////////////////////////////////////////////////
//
// INHERITED FROM ExtensionSystem::Plugin
//
////////////////////////////////////////////////////
bool FormEditorPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments)

    if (!MimeDatabase::addMimeTypes(QLatin1String(":/formeditor/Designer.mimetypes.xml"), error))
        return false;

    initializeTemplates();

    addAutoReleasedObject(new FormEditorFactory);
    addAutoReleasedObject(new SettingsPageProvider);
    addAutoReleasedObject(new QtDesignerFormClassCodeGenerator);
    // Ensure that loading designer translations is done before FormEditorW is instantiated
    const QString locale = ICore::userInterfaceLanguage();
    if (!locale.isEmpty()) {
        QTranslator *qtr = new QTranslator(this);
        const QString &creatorTrPath =
                ICore::resourcePath() + QLatin1String("/translations");
        const QString &qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        const QString &trFile = QLatin1String("designer_") + locale;
        if (qtr->load(trFile, qtTrPath) || qtr->load(trFile, creatorTrPath))
            qApp->installTranslator(qtr);
    }
    error->clear();
    return true;
}

void FormEditorPlugin::extensionsInitialized()
{
    DesignMode::instance()->setDesignModeIsRequired();
    // 4) test and make sure everything works (undo, saving, editors, opening/closing multiple files, dirtiness etc)

    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    ActionContainer *mformtools = ActionManager::createMenu(M_FORMEDITOR);
    mformtools->menu()->setTitle(tr("For&m Editor"));
    mtools->addMenu(mformtools);

    connect(m_actionSwitchSource, SIGNAL(triggered()), this, SLOT(switchSourceForm()));
    Core::Context context(Designer::Constants::C_FORMEDITOR, Core::Constants::C_EDITORMANAGER);
    Core::Command *cmd = Core::ActionManager::registerAction(m_actionSwitchSource,
                                                             "FormEditor.FormSwitchSource", context);
    cmd->setDefaultKeySequence(tr("Shift+F4"));
    mformtools->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
}

////////////////////////////////////////////////////
//
// PRIVATE functions
//
////////////////////////////////////////////////////

void FormEditorPlugin::initializeTemplates()
{
    IWizardFactory *wizard = new FormWizard;
    wizard->setWizardKind(IWizardFactory::FileWizard);
    wizard->setCategory(QLatin1String(Core::Constants::WIZARD_CATEGORY_QT));
    wizard->setDisplayCategory(QCoreApplication::translate("Core", Core::Constants::WIZARD_TR_CATEGORY_QT));
    wizard->setDisplayName(tr("Qt Designer Form"));
    wizard->setId(QLatin1String("D.Form"));
    wizard->setDescription(tr("Creates a Qt Designer form that you can add to a Qt Widget Project. "
                                       "This is useful if you already have an existing class for the UI business logic."));
    addAutoReleasedObject(wizard);

#ifdef CPP_ENABLED
    wizard = new FormClassWizard;
    wizard->setWizardKind(IWizardFactory::ClassWizard);
    wizard->setCategory(QLatin1String(Core::Constants::WIZARD_CATEGORY_QT));
    wizard->setDisplayCategory(QCoreApplication::translate("Core", Core::Constants::WIZARD_TR_CATEGORY_QT));
    wizard->setDisplayName(tr("Qt Designer Form Class"));
    wizard->setId(QLatin1String("C.FormClass"));
    wizard->setDescription(tr("Creates a Qt Designer form along with a matching class (C++ header and source file) "
                                       "for implementation purposes. You can add the form and class to an existing Qt Widget Project."));
    addAutoReleasedObject(wizard);
#endif
}

// Find out current existing editor file
static QString currentFile()
{
    if (const IDocument *document = EditorManager::currentDocument()) {
        const QString fileName = document->filePath();
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
    const MimeType currentMimeType = MimeDatabase::findByFile(current);
    if (!currentMimeType)
        return QString();
    // Determine potential suffixes of candidate files
    // 'ui' -> 'cpp', 'cpp/h' -> 'ui'.
    QStringList candidateSuffixes;
    if (currentMimeType.type() == QLatin1String(FORM_MIMETYPE)) {
        candidateSuffixes += MimeDatabase::findByType(QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)).suffixes();
    } else if (currentMimeType.type() == QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)
               || currentMimeType.type() == QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE)) {
        candidateSuffixes += MimeDatabase::findByType(QLatin1String(FORM_MIMETYPE)).suffixes();
    } else {
        return QString();
    }
    // Try to find existing file with desired suffix
    const QFileInfo currentFI(current);
    const QString currentBaseName = currentFI.path() + QLatin1Char('/')
            + currentFI.baseName() + QLatin1Char('.');
    foreach (const QString &candidateSuffix, candidateSuffixes) {
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
        Core::EditorManager::openEditor(fileToOpen);
}

Q_EXPORT_PLUGIN(FormEditorPlugin)

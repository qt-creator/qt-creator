/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "projectfilewizardextension.h"
#include "projectexplorer.h"
#include "session.h"
#include "projectnodes.h"
#include "projectwizardpage.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <coreplugin/icore.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/normalindenter.h>
#include <texteditor/tabsettings.h>
#include <texteditor/storagesettings.h>
#include <projectexplorer/project.h>
#include <projectexplorer/editorconfiguration.h>
#include <utils/mimetypes/mimedatabase.h>
#
#include <QPointer>
#include <QDebug>
#include <QFileInfo>
#include <QTextDocument>
#include <QTextCursor>
#include <QMessageBox>

using namespace TextEditor;
using namespace Core;

/*!
    \class ProjectExplorer::Internal::ProjectFileWizardExtension

    \brief The ProjectFileWizardExtension class implements the post-file
    generating steps of a project wizard.

    This class provides the following functions:
    \list
    \li Add to a project file (*.pri/ *.pro)
    \li Initialize a version control system repository (unless the path is already
        managed) and do 'add' if the VCS supports it.
    \endlist

    \sa ProjectExplorer::Internal::ProjectWizardPage
*/

enum { debugExtension = 0 };

namespace ProjectExplorer {

namespace Internal {

// --------- ProjectWizardContext

class ProjectWizardContext
{
public:
    ProjectWizardContext();
    void clear();

    QPointer<ProjectWizardPage> page; // this is managed by the wizard!
    const IWizardFactory *wizard;
};

ProjectWizardContext::ProjectWizardContext() :
    page(0),
    wizard(0)
{
}

void ProjectWizardContext::clear()
{
    page = 0;
    wizard = 0;
}

// ---- ProjectFileWizardExtension
ProjectFileWizardExtension::ProjectFileWizardExtension()
  : m_context(0)
{
}

ProjectFileWizardExtension::~ProjectFileWizardExtension()
{
    delete m_context;
}

static QString generatedProjectFilePath(const QList<GeneratedFile> &files)
{
    foreach (const GeneratedFile &file, files)
        if (file.attributes() & GeneratedFile::OpenProjectAttribute)
            return file.path();
    return QString();
}

void ProjectFileWizardExtension::firstExtensionPageShown(
        const QList<GeneratedFile> &files,
        const QVariantMap &extraValues)
{
    if (debugExtension)
        qDebug() << Q_FUNC_INFO << files.size();

    QStringList fileNames = Utils::transform(files, &GeneratedFile::path);
    m_context->page->setFiles(fileNames);

    QStringList filePaths;
    ProjectAction projectAction;
    if (m_context->wizard->kind()== IWizardFactory::ProjectWizard) {
        projectAction = AddSubProject;
        filePaths << generatedProjectFilePath(files);
    } else {
        projectAction = AddNewFile;
        filePaths = Utils::transform(files, &GeneratedFile::path);
    }

    Node *contextNode = extraValues.value(QLatin1String(Constants::PREFERRED_PROJECT_NODE)).value<Node *>();

    m_context->page->initializeProjectTree(contextNode, filePaths, m_context->wizard->kind(),
                                           projectAction);
    m_context->page->initializeVersionControls();
}

QList<QWizardPage *> ProjectFileWizardExtension::extensionPages(const IWizardFactory *wizard)
{
    if (!m_context)
        m_context = new ProjectWizardContext;
    else
        m_context->clear();
    // Init context with page and projects
    m_context->page = new ProjectWizardPage;
    m_context->wizard = wizard;
    return QList<QWizardPage *>() << m_context->page;
}

bool ProjectFileWizardExtension::processFiles(
        const QList<GeneratedFile> &files,
        bool *removeOpenProjectAttribute, QString *errorMessage)
{
    if (!processProject(files, removeOpenProjectAttribute, errorMessage))
        return false;
    if (!m_context->page->runVersionControl(files, errorMessage)) {
        QString message;
        if (errorMessage) {
            message = *errorMessage;
            message.append(QLatin1String("\n\n"));
            errorMessage->clear();
        }
        message.append(tr("Open project anyway?"));
        if (QMessageBox::question(ICore::mainWindow(), tr("Version Control Failure"), message,
                                  QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
            return false;
    }
    return true;
}

// Add files to project && version control
bool ProjectFileWizardExtension::processProject(
        const QList<GeneratedFile> &files,
        bool *removeOpenProjectAttribute, QString *errorMessage)
{
    *removeOpenProjectAttribute = false;

    QString generatedProject = generatedProjectFilePath(files);

    FolderNode *folder = m_context->page->currentNode();
    if (!folder)
        return true;
    if (m_context->wizard->kind() == IWizardFactory::ProjectWizard) {
        if (!static_cast<ProjectNode *>(folder)->addSubProjects(QStringList(generatedProject))) {
            *errorMessage = tr("Failed to add subproject \"%1\"\nto project \"%2\".")
                            .arg(generatedProject).arg(folder->filePath().toUserOutput());
            return false;
        }
        *removeOpenProjectAttribute = true;
    } else {
        QStringList filePaths = Utils::transform(files, &GeneratedFile::path);
        if (!folder->addFiles(filePaths)) {
            *errorMessage = tr("Failed to add one or more files to project\n\"%1\" (%2).").
                    arg(folder->filePath().toUserOutput(), filePaths.join(QLatin1Char(',')));
            return false;
        }
    }
    return true;
}

static ICodeStylePreferences *codeStylePreferences(Project *project, Id languageId)
{
    if (!languageId.isValid())
        return 0;

    if (project)
        return project->editorConfiguration()->codeStyle(languageId);

    return TextEditorSettings::codeStyle(languageId);
}

void ProjectFileWizardExtension::applyCodeStyle(GeneratedFile *file) const
{
    if (file->isBinary() || file->contents().isEmpty())
        return; // nothing to do

    Utils::MimeDatabase mdb;
    Id languageId = TextEditorSettings::languageId(mdb.mimeTypeForFile(file->path()).name());

    if (!languageId.isValid())
        return; // don't modify files like *.ui *.pro

    FolderNode *folder = m_context->page->currentNode();
    Project *baseProject = SessionManager::projectForNode(folder);

    ICodeStylePreferencesFactory *factory = TextEditorSettings::codeStyleFactory(languageId);

    Indenter *indenter = 0;
    if (factory)
        indenter = factory->createIndenter();
    if (!indenter)
        indenter = new NormalIndenter();

    ICodeStylePreferences *codeStylePrefs = codeStylePreferences(baseProject, languageId);
    indenter->setCodeStylePreferences(codeStylePrefs);
    QTextDocument doc(file->contents());
    QTextCursor cursor(&doc);
    cursor.select(QTextCursor::Document);
    indenter->indent(&doc, cursor, QChar::Null, codeStylePrefs->currentTabSettings());
    delete indenter;
    if (TextEditorSettings::storageSettings().m_cleanWhitespace) {
        QTextBlock block = doc.firstBlock();
        while (block.isValid()) {
            codeStylePrefs->currentTabSettings().removeTrailingWhitespace(cursor, block);
            block = block.next();
        }
    }
    file->setContents(doc.toPlainText());
}

} // namespace Internal
} // namespace ProjectExplorer

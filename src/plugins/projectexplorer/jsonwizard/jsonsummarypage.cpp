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

#include "jsonsummarypage.h"

#include "jsonwizard.h"
#include "../addnewmodel.h"
#include "../project.h"
#include "../projectexplorerconstants.h"
#include "../projectnodes.h"
#include "../session.h"

#include <coreplugin/coreconstants.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QMessageBox>

using namespace Core;

namespace ProjectExplorer {

// --------------------------------------------------------------------
// Helper:
// --------------------------------------------------------------------

static QString generatedProjectFilePath(const QList<JsonWizard::GeneratorFile> &files)
{
    foreach (const JsonWizard::GeneratorFile &file, files)
        if (file.file.attributes() & GeneratedFile::OpenProjectAttribute)
            return file.file.path();
    return QString();
}

static IWizardFactory::WizardKind wizardKind(JsonWizard *wiz)
{
    IWizardFactory::WizardKind kind = IWizardFactory::ProjectWizard;
    const QString kindStr = wiz->value(QLatin1String("kind")).toString();
    if (kindStr == QLatin1String(Core::Constants::WIZARD_KIND_PROJECT))
        kind = IWizardFactory::ProjectWizard;
    else if (kindStr == QLatin1String(Core::Constants::WIZARD_KIND_CLASS))
        kind = IWizardFactory::ClassWizard;
    else if (kindStr == QLatin1String(Core::Constants::WIZARD_KIND_FILE))
        kind = IWizardFactory::FileWizard;
    else
        QTC_CHECK(false);
    return kind;
}

// --------------------------------------------------------------------
// JsonSummaryPage:
// --------------------------------------------------------------------

JsonSummaryPage::JsonSummaryPage(QWidget *parent) :
    Internal::ProjectWizardPage(parent)
{
    connect(this, &Internal::ProjectWizardPage::projectNodeChanged,
            this, &JsonSummaryPage::projectNodeHasChanged);
}

void JsonSummaryPage::initializePage()
{
    JsonWizard *wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return);

    connect(wiz, &JsonWizard::filesReady, this, &JsonSummaryPage::triggerCommit);
    connect(wiz, &JsonWizard::filesReady, this, &JsonSummaryPage::addToProject);

    JsonWizard::GeneratorFiles files = wiz->fileList();
    QStringList filePaths = Utils::transform(files, [](const JsonWizard::GeneratorFile &f)
                                                    { return f.file.path(); });
    IWizardFactory::WizardKind kind = wizardKind(wiz);
    bool isProject = (kind == IWizardFactory::ProjectWizard);

    setFiles(filePaths);

    if (isProject) {
        filePaths.clear();
        JsonWizard::GeneratorFile f
                = Utils::findOrDefault(files, [](const JsonWizard::GeneratorFile &f) {
            return f.file.attributes() & GeneratedFile::OpenProjectAttribute;
        });
        filePaths << f.file.path();
    }

    Node *contextNode = wiz->value(QLatin1String(Constants::PREFERRED_PROJECT_NODE))
            .value<Node *>();
    initializeProjectTree(contextNode, filePaths, kind,
                          isProject ? AddSubProject : AddNewFile);

    initializeVersionControls();
}

void JsonSummaryPage::cleanupPage()
{
    JsonWizard *wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return);

    wiz->resetFileList();

    disconnect(wiz, &JsonWizard::filesReady, this, 0);
}

void JsonSummaryPage::triggerCommit(const JsonWizard::GeneratorFiles &files)
{
    GeneratedFiles coreFiles
            = Utils::transform(files, [](const JsonWizard::GeneratorFile &f) -> GeneratedFile
                                      { return f.file; });

    QString errorMessage;
    if (!runVersionControl(coreFiles, &errorMessage)) {
        QMessageBox::critical(wizard(), tr("Failed to Commit to Version Control"),
                              tr("Error message from Version Control System: '%1'.")
                              .arg(errorMessage));
    }
}

void JsonSummaryPage::addToProject(const JsonWizard::GeneratorFiles &files)
{
    JsonWizard *wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return);

    QString generatedProject = generatedProjectFilePath(files);
    IWizardFactory::WizardKind kind = wizardKind(wiz);

    FolderNode *folder = currentNode();
    if (!folder)
        return;
    if (kind == IWizardFactory::ProjectWizard) {
        if (!static_cast<ProjectNode *>(folder)->addSubProjects(QStringList(generatedProject))) {
            QMessageBox::critical(wizard(), tr("Failed to Add to Project"),
                                  tr("Failed to add subproject \"%1\"\nto project \"%2\".")
                                  .arg(QDir::toNativeSeparators(generatedProject))
                                  .arg(QDir::toNativeSeparators(folder->path())));
            return;
        }
        wiz->removeAttributeFromAllFiles(GeneratedFile::OpenProjectAttribute);
    } else {
        QStringList filePaths = Utils::transform(files, [](const JsonWizard::GeneratorFile &f) {
            return f.file.path();
        });
        if (!folder->addFiles(filePaths)) {
            QStringList nativeFilePaths = Utils::transform(filePaths, &QDir::toNativeSeparators);
            QMessageBox::critical(wizard(), tr("Failed to Add to Project"),
                                  tr("Failed to add one or more files to project\n\"%1\" (%2).")
                                  .arg(QDir::toNativeSeparators(folder->path()),
                                       nativeFilePaths.join(QLatin1String(", "))));
            return;
        }
    }
    return;
}

void JsonSummaryPage::projectNodeHasChanged()
{
    updateProjectData(currentNode());
}

void JsonSummaryPage::updateProjectData(FolderNode *node)
{
    Project *project = SessionManager::projectForNode(node);

    wizard()->setProperty("SelectedProject", QVariant::fromValue(project));
    wizard()->setProperty("SelectedFolderNode", QVariant::fromValue(node));
}

} // namespace ProjectExplorer

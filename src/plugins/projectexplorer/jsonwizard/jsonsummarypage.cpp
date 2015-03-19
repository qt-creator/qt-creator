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

#include "jsonsummarypage.h"

#include "jsonwizard.h"
#include "../project.h"
#include "../projectexplorerconstants.h"
#include "../projectnodes.h"
#include "../session.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/iversioncontrol.h>

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
    const QString kindStr = wiz->stringValue(QLatin1String("kind"));
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
    Internal::ProjectWizardPage(parent),
    m_wizard(0)
{
    connect(this, &Internal::ProjectWizardPage::projectNodeChanged,
            this, &JsonSummaryPage::projectNodeHasChanged);
    connect(this, &Internal::ProjectWizardPage::versionControlChanged,
            this, &JsonSummaryPage::versionControlHasChanged);
}

void JsonSummaryPage::initializePage()
{
    m_wizard = qobject_cast<JsonWizard *>(wizard());

    m_wizard->setProperty("SelectedProject", QVariant());
    m_wizard->setProperty("SelectedFolderNode", QVariant());
    m_wizard->setProperty("IsSubproject", QString());
    m_wizard->setProperty("VersionControl", QString());

    connect(m_wizard, &JsonWizard::filesReady, this, &JsonSummaryPage::triggerCommit);
    connect(m_wizard, &JsonWizard::filesReady, this, &JsonSummaryPage::addToProject);

    updateFileList();

    IWizardFactory::WizardKind kind = wizardKind(m_wizard);
    bool isProject = (kind == IWizardFactory::ProjectWizard);

    QStringList files;
    if (isProject) {
        JsonWizard::GeneratorFile f
                = Utils::findOrDefault(m_fileList, [](const JsonWizard::GeneratorFile &f) {
            return f.file.attributes() & GeneratedFile::OpenProjectAttribute;
        });
        files << f.file.path();
    } else {
        files = Utils::transform(m_fileList,
                                 [](const JsonWizard::GeneratorFile &f) {
                                    return f.file.path();
                                 });
    }

    Node *contextNode = m_wizard->value(QLatin1String(Constants::PREFERRED_PROJECT_NODE))
            .value<Node *>();
    initializeProjectTree(contextNode, files, kind,
                          isProject ? AddSubProject : AddNewFile);

    initializeVersionControls();
}

bool JsonSummaryPage::validatePage()
{
    m_wizard->commitToFileList(m_fileList);
    m_fileList.clear();
    return true;
}

void JsonSummaryPage::cleanupPage()
{
    disconnect(m_wizard, &JsonWizard::filesReady, this, 0);
}

void JsonSummaryPage::triggerCommit(const JsonWizard::GeneratorFiles &files)
{
    GeneratedFiles coreFiles
            = Utils::transform(files, [](const JsonWizard::GeneratorFile &f) -> GeneratedFile
                                      { return f.file; });

    QString errorMessage;
    if (!runVersionControl(coreFiles, &errorMessage)) {
        QMessageBox::critical(wizard(), tr("Failed to Commit to Version Control"),
                              tr("Error message from Version Control System: \"%1\".")
                              .arg(errorMessage));
    }
}

void JsonSummaryPage::addToProject(const JsonWizard::GeneratorFiles &files)
{
    QTC_CHECK(m_fileList.isEmpty()); // Happens after this page is done
    QString generatedProject = generatedProjectFilePath(files);
    IWizardFactory::WizardKind kind = wizardKind(m_wizard);

    FolderNode *folder = currentNode();
    if (!folder)
        return;
    if (kind == IWizardFactory::ProjectWizard) {
        if (!static_cast<ProjectNode *>(folder)->addSubProjects(QStringList(generatedProject))) {
            QMessageBox::critical(m_wizard, tr("Failed to Add to Project"),
                                  tr("Failed to add subproject \"%1\"\nto project \"%2\".")
                                  .arg(QDir::toNativeSeparators(generatedProject))
                                  .arg(folder->path().toUserOutput()));
            return;
        }
        m_wizard->removeAttributeFromAllFiles(GeneratedFile::OpenProjectAttribute);
    } else {
        QStringList filePaths = Utils::transform(files, [](const JsonWizard::GeneratorFile &f) {
            return f.file.path();
        });
        if (!folder->addFiles(filePaths)) {
            QStringList nativeFilePaths = Utils::transform(filePaths, &QDir::toNativeSeparators);
            QMessageBox::critical(wizard(), tr("Failed to Add to Project"),
                                  tr("Failed to add one or more files to project\n\"%1\" (%2).")
                                  .arg(folder->path().toUserOutput(),
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

void JsonSummaryPage::versionControlHasChanged()
{
    IVersionControl *vc = currentVersionControl();
    m_wizard->setProperty("VersionControl", vc ? vc->id().toString() : QString());

    updateFileList();
}

void JsonSummaryPage::updateFileList()
{
    m_fileList = m_wizard->generateFileList();
    QStringList filePaths
            = Utils::transform(m_fileList, [](const JsonWizard::GeneratorFile &f) { return f.file.path(); });
    setFiles(filePaths);
}

void JsonSummaryPage::updateProjectData(FolderNode *node)
{
    Project *project = SessionManager::projectForNode(node);

    m_wizard->setProperty("SelectedProject", QVariant::fromValue(project));
    m_wizard->setProperty("SelectedFolderNode", QVariant::fromValue(node));
    m_wizard->setProperty("IsSubproject", node ? QLatin1String("yes") : QString());

    updateFileList();
}

} // namespace ProjectExplorer

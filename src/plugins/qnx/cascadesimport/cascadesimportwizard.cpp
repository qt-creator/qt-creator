/****************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry Limited (blackberry-qt@qnx.com)
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

#include "cascadesimportwizard.h"
#include "srcprojectwizardpage.h"
#include "bardescriptorconverter.h"
#include "importlogconverter.h"
#include "projectfileconverter.h"

#include <qnx/qnxconstants.h>

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>

#include <utils/projectintropage.h>

#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QStringBuilder>
#include <QDirIterator>

namespace Qnx {
namespace Internal {

//////////////////////////////////////////////////////////////////////////////
//
// CascadesImportWizardDialog
//
//////////////////////////////////////////////////////////////////////////////

CascadesImportWizardDialog::CascadesImportWizardDialog(QWidget *parent) :
    Core::BaseFileWizard(parent)
{
    setWindowTitle(tr("Import Existing Momentics Cascades Project"));

    m_srcProjectPage = new SrcProjectWizardPage(this);
    m_srcProjectPage->setTitle(tr("Momentics Cascades Project Name and Location"));
    addPage(m_srcProjectPage);

    m_destProjectPage = new Utils::ProjectIntroPage(this);
    m_destProjectPage->setTitle(tr("Project Name and Location"));
    m_destProjectPage->setPath(Core::DocumentManager::projectsDirectory());
    const int destProjectPageId = addPage(m_destProjectPage);
    wizardProgress()->item(destProjectPageId)->setTitle(tr("Qt Creator")); // Override default name

    connect(m_srcProjectPage, SIGNAL(validPathChanged(QString)), this, SLOT(onSrcProjectPathChanged(QString)));
}

QString CascadesImportWizardDialog::srcProjectPath() const
{
    return m_srcProjectPage->path();
}

QString CascadesImportWizardDialog::destProjectPath() const
{
    return m_destProjectPage->path() % QLatin1Char('/') % projectName();
}

QString CascadesImportWizardDialog::projectName() const
{
    return m_destProjectPage->projectName();
}

void CascadesImportWizardDialog::onSrcProjectPathChanged(const QString &path)
{
    Q_UNUSED(path);
    m_destProjectPage->setProjectName(m_srcProjectPage->projectName());
}

//////////////////////////////////////////////////////////////////////////////
//
// Wizard
//
//////////////////////////////////////////////////////////////////////////////
static const char IMPORT_LOG_FILE_NAME[] = "import.log";

CascadesImportWizard::CascadesImportWizard()
{
    setWizardKind(ProjectWizard);
    setIcon(QPixmap(QLatin1String(Qnx::Constants::QNX_BB_CATEGORY_ICON)));
    setDisplayName(tr("Momentics Cascades Project"));
    setId(QLatin1String("Q.QnxBlackBerryCascadesApp"));
    setRequiredFeatures(Core::FeatureSet(Constants::QNX_BB_FEATURE));
    setDescription(tr("Imports existing Cascades projects created within QNX Momentics IDE. "
                      "This allows you to use the project in Qt Creator."));
    setCategory(QLatin1String(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY));
    setDisplayCategory(QLatin1String(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY_DISPLAY));
}

Core::BaseFileWizard *CascadesImportWizard::create(QWidget *parent,
                                      const Core::WizardDialogParameters &parameters) const
{
    CascadesImportWizardDialog *wizard = new CascadesImportWizardDialog(parent);

    foreach (QWizardPage *p, parameters.extensionPages())
        wizard->addPage(p);

    return wizard;
}

bool CascadesImportWizard::convertFile(Core::GeneratedFile &file,
        ConvertedProjectContext &projectContext, QString &errorMessage) const
{
    bool ret = false;
    if (convertFileContent(file, projectContext, errorMessage))
        if (convertFilePath(file, projectContext, errorMessage))
            ret = true;
    return ret;
}

bool CascadesImportWizard::convertFileContent(Core::GeneratedFile &file,
        ConvertedProjectContext &projectContext, QString &errorMessage) const
{
    QString filePath = file.path();
    QString fileName = filePath.section(QLatin1Char('/'), -1);
    bool isRootFile = (fileName == filePath);
    QString fileExtension = fileName.section(QLatin1Char('.'), -1).toLower();
    bool useFileConverter = true;
    if (isRootFile) {
        if (fileName == QLatin1String("bar-descriptor.xml")) {
            BarDescriptorConverter conv(projectContext);
            conv.convertFile(file, errorMessage);
            useFileConverter = false;
        } else if (fileName == QLatin1String(IMPORT_LOG_FILE_NAME)) {
            ImportLogConverter conv(projectContext);
            conv.convertFile(file, errorMessage);
            useFileConverter = false;
        } else if (fileExtension == QLatin1String("pro")) {
            ProjectFileConverter conv(projectContext);
            conv.convertFile(file, errorMessage);
            useFileConverter = false;
        }
    }
    if (useFileConverter) {
        FileConverter conv(projectContext);
        conv.convertFile(file, errorMessage);
    }
    return errorMessage.isEmpty();
}

bool CascadesImportWizard::convertFilePath(Core::GeneratedFile &file,
        ConvertedProjectContext &projectContext, QString &errorMessage) const
{
    Q_UNUSED(errorMessage);
    const QString destProjectPath = projectContext.destProjectPath();
    file.setPath(destProjectPath % QLatin1Char('/') % file.path());
    return true;
}

void CascadesImportWizard::collectFiles_helper(QStringList &filePaths,
        ConvertedProjectContext &projectContext, const QString &rootPath,
        const QList< QRegExp > &blackList) const
{
    const QString srcProjectPath = projectContext.srcProjectPath();
    bool isProjectRoot = (rootPath.length() == srcProjectPath.length());
    QDirIterator it(rootPath, QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
    while (it.hasNext()) {
        it.next();
        bool isBlacklisted = false;
        QString fileName = it.fileName();
        foreach (const QRegExp &rx, blackList) {
            QString pattern = rx.pattern();
            if (pattern.at(0) == QLatin1Char('/')) {
                if (isProjectRoot) {
                    QString fn = QLatin1Char('/') % fileName;
                    if (rx.exactMatch(fn)) {
                        isBlacklisted = true;
                        break;
                    }
                }
            } else {
                if (rx.exactMatch(fileName)) {
                    isBlacklisted = true;
                    break;
                }
            }
        }
        if (!isBlacklisted) {
            QFileInfo fi = it.fileInfo();
            if (fi.isFile())
                filePaths << it.filePath().mid(srcProjectPath.length() + 1);
            else if (fi.isDir())
                collectFiles_helper(filePaths, projectContext, it.filePath(), blackList);
        }
    }
}

bool CascadesImportWizard::collectFiles(ConvertedProjectContext &projectContext, QString &errorMessage) const
{
    static QList<QRegExp> blackList;
    if (blackList.isEmpty()) {
        blackList << QRegExp(QLatin1String("/arm"), Qt::CaseInsensitive, QRegExp::Wildcard)
            << QRegExp(QLatin1String("/x86"), Qt::CaseInsensitive, QRegExp::Wildcard)
            << QRegExp(QLatin1String("/translations"), Qt::CaseInsensitive, QRegExp::Wildcard)
            << QRegExp(QLatin1String("Makefile"), Qt::CaseInsensitive, QRegExp::Wildcard)
            << QRegExp(QLatin1String("Makefile.*"), Qt::CaseInsensitive, QRegExp::Wildcard)
            << QRegExp(QLatin1String("/config.pri"), Qt::CaseInsensitive, QRegExp::Wildcard)
            << QRegExp(QLatin1String("/precompiled.h"), Qt::CaseInsensitive, QRegExp::Wildcard);
    }
    QStringList filePaths;
    collectFiles_helper(filePaths, projectContext, projectContext.srcProjectPath(), blackList);
    filePaths << QLatin1String(IMPORT_LOG_FILE_NAME);
    projectContext.setCollectedFiles(filePaths);
    return errorMessage.isEmpty();
}

Core::GeneratedFiles CascadesImportWizard::generateFiles(const QWizard *w, QString *pErrorMessage) const
{
    Core::GeneratedFiles files;
    QString errorMessage;

    const CascadesImportWizardDialog *wizardDialog = qobject_cast<const CascadesImportWizardDialog *>(w);
    if (wizardDialog) {
        ConvertedProjectContext projectContext;
        projectContext.setSrcProjectPath(wizardDialog->srcProjectPath());
        projectContext.setDestProjectPath(wizardDialog->destProjectPath());

        if (collectFiles(projectContext, errorMessage)) {
            foreach (const QString &filePath, projectContext.collectedFiles()) {
                Core::GeneratedFile file(filePath);
                if (convertFile(file, projectContext, errorMessage)) {
                    if (!file.path().isEmpty())
                        files << file;
                }
                if (!errorMessage.isEmpty()) {
                    errorMessage = tr("Error generating file \"%1\": %2").arg(filePath).arg(errorMessage);
                    break;
                }
            }
        }
    }


    if (!errorMessage.isEmpty())
        if (pErrorMessage)
            *pErrorMessage = errorMessage;
    return files;
}

bool CascadesImportWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage)
{
    Q_UNUSED(w);
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
}

} // namespace Internal
} // namespace Qnx

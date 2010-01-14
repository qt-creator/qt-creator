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

#include "basecheckoutwizard.h"
#include "vcsbaseconstants.h"
#include "checkoutwizarddialog.h"
#include "checkoutjobs.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtGui/QMessageBox>

namespace VCSBase {

struct BaseCheckoutWizardPrivate {
    BaseCheckoutWizardPrivate() : dialog(0) {}
    void clear();

    Internal::CheckoutWizardDialog *dialog;
    QList<QWizardPage *> parameterPages;
    QString checkoutPath;
    QString id;
};

void BaseCheckoutWizardPrivate::clear()
{
    parameterPages.clear();
    dialog = 0;
    checkoutPath.clear();
}

BaseCheckoutWizard::BaseCheckoutWizard(QObject *parent) :
    Core::IWizard(parent),
    d(new BaseCheckoutWizardPrivate)
{
}

BaseCheckoutWizard::~BaseCheckoutWizard()
{
    delete d;
}

Core::IWizard::WizardKind BaseCheckoutWizard::kind() const
{
    return Core::IWizard::ProjectWizard;
}

QString BaseCheckoutWizard::category() const
{
    return QLatin1String(VCSBase::Constants::VCS_WIZARD_CATEGORY);
}

QString BaseCheckoutWizard::displayCategory() const
{
    return QCoreApplication::translate("VCSBase", VCSBase::Constants::VCS_WIZARD_TR_CATEGORY);
}

QString BaseCheckoutWizard::id() const
{
    return d->id;
}

void BaseCheckoutWizard::setId(const QString &id)
{
    d->id = id;
}

QStringList BaseCheckoutWizard::runWizard(const QString &path, QWidget *parent)
{
    // Create dialog and launch
    d->parameterPages = createParameterPages(path);
    Internal::CheckoutWizardDialog dialog(d->parameterPages, parent);
    d->dialog = &dialog;
    connect(&dialog, SIGNAL(progressPageShown()), this, SLOT(slotProgressPageShown()));
    dialog.setWindowTitle(displayName());
    if (dialog.exec() != QDialog::Accepted)
        return QStringList();
    // Now try to find the project file and open
    const QString checkoutPath = d->checkoutPath;
    d->clear();
    QString errorMessage;
    const QString projectFile = openProject(checkoutPath, &errorMessage);
    if (projectFile.isEmpty()) {
        QMessageBox msgBox(QMessageBox::Warning, tr("Cannot Open Project"),
                           tr("Failed to open project in '%1'.").arg(QDir::toNativeSeparators(checkoutPath)));
        msgBox.setDetailedText(errorMessage);
        msgBox.exec();
        return QStringList();
    }
    return QStringList(projectFile);
}

static inline QString msgNoProjectFiles(const QDir &dir, const QStringList &patterns)
{
    return BaseCheckoutWizard::tr("Could not find any project files matching (%1) in the directory '%2'.").arg(patterns.join(QLatin1String(", ")), QDir::toNativeSeparators(dir.absolutePath()));
}

// Try to find the project files in a project directory with some smartness
static QFileInfoList findProjectFiles(const QDir &projectDir, QString *errorMessage)
{
    // Hardcoded: Find *.pro/Cmakefiles
    QStringList projectFilePatterns;
    projectFilePatterns << QLatin1String("*.pro") << QLatin1String("CMakeLists.txt");
    // Project directory
    QFileInfoList projectFiles = projectDir.entryInfoList(projectFilePatterns, QDir::Files|QDir::NoDotAndDotDot|QDir::Readable);
    if (!projectFiles.empty())
        return projectFiles;
    // Try a 'src' directory
    QFileInfoList srcDirs = projectDir.entryInfoList(QStringList(QLatin1String("src")), QDir::Dirs|QDir::NoDotAndDotDot|QDir::Readable);
    if (srcDirs.empty()) {
        *errorMessage = msgNoProjectFiles(projectDir, projectFilePatterns);
        return QFileInfoList();
    }
    const QDir srcDir = QDir(srcDirs.front().absoluteFilePath());
    projectFiles = srcDir.entryInfoList(projectFilePatterns, QDir::Files|QDir::NoDotAndDotDot|QDir::Readable);
    if (projectFiles.empty()) {
        *errorMessage = msgNoProjectFiles(srcDir, projectFilePatterns);
        return QFileInfoList();
    }
    return projectFiles;;
}

QString BaseCheckoutWizard::openProject(const QString &path, QString *errorMessage)
{
    ProjectExplorer::ProjectExplorerPlugin *pe  = ProjectExplorer::ProjectExplorerPlugin::instance();
    if (!pe) {
        *errorMessage = tr("The Project Explorer is not available.");
        return QString();
    }

    // Search the directory for project files
    const QDir dir(path);
    if (!dir.exists()) {
        *errorMessage = tr("'%1' does not exist.").arg(path); // Should not happen
        return QString();
    }
    QFileInfoList projectFiles = findProjectFiles(dir, errorMessage);
    if (projectFiles.empty())
        return QString();
    // Open. Do not use a busy cursor here as additional wizards might pop up
    const QString projectFile = projectFiles.front().absoluteFilePath();
    if (!pe->openProject(projectFile)) {
        *errorMessage = tr("Unable to open the project '%1'.").arg(projectFile);
        return QString();
    }
    return projectFile;
}

void BaseCheckoutWizard::slotProgressPageShown()
{
    const QSharedPointer<AbstractCheckoutJob> job = createJob(d->parameterPages, &(d->checkoutPath));
    if (!job.isNull())
        d->dialog->start(job);
}

} // namespace VCSBase

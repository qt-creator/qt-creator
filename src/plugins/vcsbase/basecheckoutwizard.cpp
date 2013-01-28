/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "basecheckoutwizard.h"
#include "vcsbaseconstants.h"
#include "checkoutwizarddialog.h"
#include "checkoutjobs.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

/*!
    \class VcsBase::BaseCheckoutWizard

    \brief A Core::IWizard implementing a wizard for initially checking out a project using
    a version control system.

   Implements all of Core::IWizard with the exception of
   name()/description() and icon().

   Pops up a QWizard consisting of a Parameter Page which is created
   by a virtual factory function and a progress
   page containing a log text. The factory function createJob()
   creates a job with the output connected to the log window,
   returning the path to the checkout.

   On success, the wizard tries to locate a project file
   and open it.

   \sa VcsBase::BaseCheckoutWizardPage
*/

namespace VcsBase {
namespace Internal {

class BaseCheckoutWizardPrivate
{
public:
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

} // namespace Internal

BaseCheckoutWizard::BaseCheckoutWizard(QObject *parent) :
    Core::IWizard(parent),
    d(new Internal::BaseCheckoutWizardPrivate)
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
    return QLatin1String(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY);
}

QString BaseCheckoutWizard::displayCategory() const
{
    return QCoreApplication::translate("ProjectExplorer", ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY_DISPLAY);
}

QString BaseCheckoutWizard::id() const
{
    return d->id;
}

QString BaseCheckoutWizard::descriptionImage() const
{
    return QString();
}

void BaseCheckoutWizard::setId(const QString &id)
{
    d->id = id;
}

void BaseCheckoutWizard::runWizard(const QString &path, QWidget *parent, const QString & /*platform*/, const QVariantMap &extraValues)
{
    Q_UNUSED(extraValues)
    // Create dialog and launch
    d->parameterPages = createParameterPages(path);
    Internal::CheckoutWizardDialog dialog(d->parameterPages, parent);
    d->dialog = &dialog;
    connect(&dialog, SIGNAL(progressPageShown()), this, SLOT(slotProgressPageShown()));
    dialog.setWindowTitle(displayName());
    if (dialog.exec() != QDialog::Accepted)
        return;
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
    }
}

Core::FeatureSet BaseCheckoutWizard::requiredFeatures() const
{
    return Core::FeatureSet();
}

Core::IWizard::WizardFlags BaseCheckoutWizard::flags() const
{
    return Core::IWizard::PlatformIndependent;
}

static inline QString msgNoProjectFiles(const QDir &dir, const QStringList &patterns)
{
    return BaseCheckoutWizard::tr("Could not find any project files matching (%1) in the directory '%2'.").arg(patterns.join(QLatin1String(", ")), QDir::toNativeSeparators(dir.absolutePath()));
}

// Try to find the project files in a project directory with some smartness
static QFileInfoList findProjectFiles(const QDir &projectDir, QString *errorMessage)
{
    const QStringList projectFilePatterns = ProjectExplorer::ProjectExplorerPlugin::projectFilePatterns();
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
        *errorMessage = tr("'%1' does not exist.").
                        arg(QDir::toNativeSeparators(path)); // Should not happen
        return QString();
    }
    QFileInfoList projectFiles = findProjectFiles(dir, errorMessage);
    if (projectFiles.empty())
        return QString();
    // Open. Do not use a busy cursor here as additional wizards might pop up
    const QString projectFile = projectFiles.front().absoluteFilePath();
    if (!pe->openProject(projectFile, errorMessage))
        return QString();

    return projectFile;
}

void BaseCheckoutWizard::slotProgressPageShown()
{
    const QSharedPointer<AbstractCheckoutJob> job = createJob(d->parameterPages, &(d->checkoutPath));
    d->dialog->start(job);
}

} // namespace VcsBase

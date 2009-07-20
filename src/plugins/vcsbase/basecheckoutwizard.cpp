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
** contact the sales department at http://www.qtsoftware.com/contact.
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
    BaseCheckoutWizardPrivate() : dialog(0), parameterPage(0) {}
    void clear();

    Internal::CheckoutWizardDialog *dialog;
    QWizardPage *parameterPage;
    QString checkoutPath;
};

void BaseCheckoutWizardPrivate::clear()
{
    parameterPage = 0;
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

Core::IWizard::Kind BaseCheckoutWizard::kind() const
{
    return  Core::IWizard::ProjectWizard;
}

QString BaseCheckoutWizard::category() const
{
    return QLatin1String(VCSBase::Constants::VCS_WIZARD_CATEGORY);
}

QString BaseCheckoutWizard::trCategory() const
{
    return QCoreApplication::translate("VCSBase", VCSBase::Constants::VCS_WIZARD_CATEGORY);
}

QStringList BaseCheckoutWizard::runWizard(const QString &path, QWidget *parent)
{
    // Create dialog and launch
    d->parameterPage = createParameterPage(path);
    Internal::CheckoutWizardDialog dialog(d->parameterPage, parent);
    d->dialog = &dialog;
    connect(&dialog, SIGNAL(progressPageShown()), this, SLOT(slotProgressPageShown()));
    dialog.setWindowTitle(name());
    if (dialog.exec() != QDialog::Accepted)
        return QStringList();
    // Now try to find the project file and open
    const QString checkoutPath = d->checkoutPath;
    d->clear();
    QString errorMessage;
    const QString projectFile = openProject(checkoutPath, &errorMessage);
    if (projectFile.isEmpty()) {
        QMessageBox msgBox(QMessageBox::Warning, tr("Cannot Open Project"),
                           tr("Failed to open project in '%1'").arg(checkoutPath));
        msgBox.setDetailedText(errorMessage);
        msgBox.exec();
        return QStringList();
    }
    return QStringList(projectFile);
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
    // Hardcoded: Find *.pro/Cmakefiles
    QStringList patterns;
    patterns << QLatin1String("*.pro") << QLatin1String("CMakeList.txt");
    QFileInfoList projectFiles = dir.entryInfoList(patterns, QDir::Files|QDir::NoDotAndDotDot|QDir::Readable);
    if (projectFiles.empty()) {
        *errorMessage = tr("No project files could be found (%1).").arg(patterns.join(QLatin1String(", ")));
        return QString();
    }
    // Open!
    const QString projectFile = projectFiles.front().absoluteFilePath();
    if (!pe->openProject(projectFile)) {
        *errorMessage = tr("Unable to open the project '%1'.").arg(projectFile);
        return QString();
    }
    return projectFile;
}

void BaseCheckoutWizard::slotProgressPageShown()
{
    const QSharedPointer<AbstractCheckoutJob> job = createJob(d->parameterPage, &(d->checkoutPath));
    if (!job.isNull())
        d->dialog->start(job);
}

} // namespace VCSBase

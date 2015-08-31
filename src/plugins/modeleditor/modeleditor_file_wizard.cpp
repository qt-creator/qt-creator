/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#include "modeleditor_file_wizard.h"

#include "modeleditor_constants.h"

#include "qmt/controller/namecontroller.h"
#include "qmt/model/mcanvasdiagram.h"
#include "qmt/model/mpackage.h"
#include "qmt/serializer/projectserializer.h"
#include "qmt/project/project.h"

#include <coreplugin/basefilewizard.h>
#include <utils/filewizardpage.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>

namespace ModelEditor {
namespace Internal {

FileWizardFactory::FileWizardFactory()
    : Core::BaseFileWizardFactory()
{
    setWizardKind(Core::IWizardFactory::FileWizard);
    setCategory(QLatin1String(Constants::WIZARD_CATEGORY));
    QString trCategory = QCoreApplication::translate(Constants::WIZARD_CATEGORY,
                                                     Constants::WIZARD_TR_CATEGORY);
    setDisplayCategory(trCategory);
    setDisplayName(tr("Model"));
    setId(Constants::WIZARD_MODEL_ID);
    setDescription(tr("Creates an empty model"));
    setFlags(Core::IWizardFactory::PlatformIndependent);
}

Core::BaseFileWizard *FileWizardFactory::create(
        QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    auto wizard = new Core::BaseFileWizard(this, QVariantMap(), parent);
    wizard->setWindowTitle(tr("New %1").arg(displayName()));

    auto page = new Utils::FileWizardPage;
    if (parameters.flags().testFlag(Core::WizardDialogParameters::ForceCapitalLetterForFileName))
        page->setForceFirstCapitalLetterForFileName(true);
    page->setTitle(tr("Model Name and Location"));
    page->setFileNameLabel(tr("Model name:"));
    page->setPathLabel(tr("Location:"));
    page->setPath(parameters.defaultPath());
    wizard->addPage(page);

    foreach (QWizardPage *p, wizard->extensionPages())
        wizard->addPage(p);
    return wizard;
}

Core::GeneratedFiles FileWizardFactory::generateFiles(const QWizard *w,
                                                      QString *errorMessage) const
{
    const Utils::Wizard *wizard = qobject_cast<const Utils::Wizard *>(w);
    Utils::FileWizardPage *page = wizard->find<Utils::FileWizardPage>();
    QTC_ASSERT(page, return Core::GeneratedFiles());

    return generateFilesFromPath(page->path(), page->fileName(), errorMessage);
}

Core::GeneratedFiles FileWizardFactory::generateFilesFromPath(const QString &path,
                                                              const QString &name,
                                                              QString *errorMessage) const
{
    Q_UNUSED(errorMessage);

    Core::GeneratedFiles files;

    const QString suffix = preferredSuffix(QLatin1String(Constants::MIME_TYPE_MODEL));
    const QString fileName = Core::BaseFileWizardFactory::buildFileName(path, name, suffix);
    Core::GeneratedFile file(fileName);

    auto rootPackage = new qmt::MPackage();
    rootPackage->setName(qmt::NameController::convertFileNameToElementName(fileName));

    auto rootDiagram = new qmt::MCanvasDiagram();
    rootDiagram->setName(qmt::NameController::convertFileNameToElementName(fileName));
    rootPackage->addChild(rootDiagram);

    qmt::Project project;
    project.setRootPackage(rootPackage);
    project.setFileName(fileName);

    qmt::ProjectSerializer serializer;
    QByteArray contents = serializer.save(&project);

    file.setContents(QString::fromUtf8(contents));
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    files << file;
    return files;
}

} // namespace Internal
} // namespace ModelEditor

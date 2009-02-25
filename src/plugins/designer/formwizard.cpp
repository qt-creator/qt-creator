/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "formwizard.h"
#include "formwizarddialog.h"
#include "formwindoweditor.h"
#include "designerconstants.h"

#include <QtCore/QFile>
#include <QtCore/QDebug>

using namespace Designer;
using namespace Designer::Internal;

FormWizard::FormWizard(const BaseFileWizardParameters &parameters, QObject *parent)
  : Core::BaseFileWizard(parameters, parent)
{
}

QWizard *FormWizard::createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const
{
    FormFileWizardDialog *wizardDialog = new FormFileWizardDialog(extensionPages, parent);
    wizardDialog->setPath(defaultPath);
    return wizardDialog;
}

Core::GeneratedFiles FormWizard::generateFiles(const QWizard *w,
                                               QString *errorMessage) const
{
    const FormFileWizardDialog *wizard = qobject_cast<const FormFileWizardDialog *>(w);
    const QString fileName = Core::BaseFileWizard::buildFileName(wizard->path(), wizard->name(),
                                                                 preferredSuffix(QLatin1String(Constants::FORM_MIMETYPE)));

    const QString formTemplate = wizard->templateContents();
    if (formTemplate.isEmpty()) {
        *errorMessage = QLatin1String("Internal error: FormWizard::generateFiles: empty template contents");
        return Core::GeneratedFiles();
    }

    Core::GeneratedFile file(fileName);
    file.setContents(formTemplate);
    file.setEditorKind(QLatin1String(Constants::C_FORMEDITOR));
    return Core::GeneratedFiles() << file;
}

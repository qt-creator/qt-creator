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

#include "formwizard.h"
#include "formwizarddialog.h"
#include "designerconstants.h"
#include <qtsupport/qtsupportconstants.h>

#include <QDebug>

using namespace Designer;
using namespace Designer::Internal;

FormWizard::FormWizard()
{
    addRequiredFeature(Core::Id(QtSupport::Constants::FEATURE_QWIDGETS));
}

Core::BaseFileWizard *FormWizard::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    FormFileWizardDialog *wizardDialog = new FormFileWizardDialog(parameters.extensionPages(),
                                                                  parent);
    wizardDialog->setPath(parameters.defaultPath());
    return wizardDialog;
}

Core::GeneratedFiles FormWizard::generateFiles(const QWizard *w,
                                               QString *errorMessage) const
{
    const FormFileWizardDialog *wizard = qobject_cast<const FormFileWizardDialog *>(w);
    const QString fileName = Core::BaseFileWizardFactory::buildFileName(wizard->path(), wizard->fileName(),
                                                                 preferredSuffix(QLatin1String(Constants::FORM_MIMETYPE)));

    const QString formTemplate = wizard->templateContents();
    if (formTemplate.isEmpty()) {
        *errorMessage = QLatin1String("Internal error: FormWizard::generateFiles: empty template contents");
        return Core::GeneratedFiles();
    }

    Core::GeneratedFile file(fileName);
    file.setContents(formTemplate);
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    return Core::GeneratedFiles() << file;
}

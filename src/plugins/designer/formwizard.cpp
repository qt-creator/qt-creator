/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "formwizard.h"
#include "formwizarddialog.h"
#include "formwindoweditor.h"
#include "designerconstants.h"

#include <coreplugin/icore.h>

#include <QtCore/QFile>
#include <QtCore/QDebug>

enum { debugFormWizard = 0 };

using namespace Designer;
using namespace Designer::Internal;

FormWizard::FormWizard(const BaseFileWizardParameters &parameters, Core::ICore *core, QObject *parent) :
    Core::BaseFileWizard(parameters, core, parent)
{
}

QWizard *FormWizard::createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const
{
    FormFileWizardDialog *wizardDialog = new FormFileWizardDialog(core(), extensionPages, parent);
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

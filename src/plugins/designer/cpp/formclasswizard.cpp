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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include "formclasswizard.h"
#include "formclasswizarddialog.h"
#include "designerconstants.h"
#include "formwindoweditor.h"

#include <coreplugin/icore.h>
#include <cppeditor/cppeditorconstants.h>

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDebug>

enum { debugFormClassWizard = 0 };

using namespace Designer;
using namespace Designer::Internal;

FormClassWizard::FormClassWizard(const BaseFileWizardParameters &parameters, Core::ICore *core, QObject *parent) :
    Core::BaseFileWizard(parameters, core, parent)
{
}

QString FormClassWizard::headerSuffix() const
{
    return preferredSuffix(QLatin1String(CppEditor::Constants::CPP_HEADER_MIMETYPE));
}

QString FormClassWizard::sourceSuffix() const
{
    return preferredSuffix(QLatin1String(CppEditor::Constants::CPP_SOURCE_MIMETYPE));
}

QString FormClassWizard::formSuffix() const
{
    return preferredSuffix(QLatin1String(Constants::FORM_MIMETYPE));
}

QWizard *FormClassWizard::createWizardDialog(QWidget *parent,
                                             const QString &defaultPath,
                                             const WizardPageList &extensionPages) const
{
    FormClassWizardDialog *wizardDialog = new FormClassWizardDialog(extensionPages,
                                                                    parent);
    wizardDialog->setSuffixes(headerSuffix(), sourceSuffix(), formSuffix());
    wizardDialog->setPath(defaultPath);
    return wizardDialog;
}

Core::GeneratedFiles FormClassWizard::generateFiles(const QWizard *w, QString *errorMessage) const
{
    const FormClassWizardDialog *wizardDialog = qobject_cast<const FormClassWizardDialog *>(w);
    const FormClassWizardParameters params = wizardDialog->parameters();

    if (params.uiTemplate.isEmpty()) {
        *errorMessage = tr("Internal error: FormClassWizard::generateFiles: empty template contents");
        return Core::GeneratedFiles();
    }

    // header
    const QString formFileName = buildFileName(params.path, params.uiFile, formSuffix());
    const QString headerFileName = buildFileName(params.path, params.headerFile, headerSuffix());
    const QString sourceFileName = buildFileName(params.path, params.sourceFile, sourceSuffix());

    Core::GeneratedFile headerFile(headerFileName);
    headerFile.setEditorKind(QLatin1String(CppEditor::Constants::CPPEDITOR_KIND));

    // Source
    Core::GeneratedFile sourceFile(sourceFileName);
    sourceFile.setEditorKind(QLatin1String(CppEditor::Constants::CPPEDITOR_KIND));

    // UI
    Core::GeneratedFile uiFile(formFileName);
    uiFile.setContents(params.uiTemplate);
    uiFile.setEditorKind(QLatin1String(Constants::C_FORMEDITOR));

    QString source, header;
    params.generateCpp(&header, &source);
    sourceFile.setContents(source);
    headerFile.setContents(header);

    if (debugFormClassWizard)
        qDebug() << Q_FUNC_INFO << '\n' << header << '\n' << source;

    return  Core::GeneratedFiles() << headerFile << sourceFile << uiFile;
}

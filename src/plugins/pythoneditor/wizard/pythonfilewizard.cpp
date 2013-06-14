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

/**
 * @brief The FileWizard class - adds wizard for creating new Python source file
 */

#include "pythonfilewizard.h"
#include "../pythoneditorconstants.h"

#include <utils/filewizarddialog.h>
#include <texteditor/textfilewizard.h>

#include <QWizard>

namespace PythonEditor {

/**
 * @brief GetDefaultParams
 * @return Default parameters for menu item "Files&Classes->Python->Python file"
 */
static const Core::BaseFileWizardParameters GetDefaultParams()
{
    Core::BaseFileWizardParameters p(Core::IWizard::FileWizard);

    p.setId(QLatin1String(Constants::C_PY_SOURCE_WIZARD_ID));
    p.setCategory(QLatin1String(Constants::C_PY_WIZARD_CATEGORY));
    p.setDisplayCategory(QLatin1String(Constants::C_PY_DISPLAY_CATEGORY));
    p.setDisplayName(
                FileWizard::tr(Constants::EN_PY_SOURCE_DISPLAY_NAME));
    p.setDescription(
                FileWizard::tr(Constants::EN_PY_SOURCE_DESCRIPTION));

    return p;
}

/**
 * @brief Initialize wizard and add new option to "New..." dialog.
 * @param parent
 */
FileWizard::FileWizard(QObject *parent)
    :Core::BaseFileWizard(GetDefaultParams(), parent)
{
}

FileWizard::~FileWizard()
{
}

/**
 * @brief FileWizard::createWizardDialog
 * @param parent
 * @param params
 * @return
 */
QWizard *FileWizard::createWizardDialog(QWidget *parent,
                                        const Core::WizardDialogParameters &params) const
{
    Utils::FileWizardDialog *pDialog = new Utils::FileWizardDialog(parent);
    pDialog->setWindowTitle(tr("New %1").arg(displayName()));
    setupWizard(pDialog);
    pDialog->setPath(params.defaultPath());
    foreach (QWizardPage *p, params.extensionPages())
        applyExtensionPageShortTitle(pDialog, pDialog->addPage(p));

    return pDialog;
}

Core::GeneratedFiles FileWizard::generateFiles(const QWizard *dialog,
                                                QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const Utils::FileWizardDialog *pWizard =
            qobject_cast<const Utils::FileWizardDialog *>(dialog);

    QString folder = pWizard->path();
    QString name = pWizard->fileName();

    name = Core::BaseFileWizard::buildFileName(
                folder, name, QLatin1String(Constants::C_PY_EXTENSION));
    Core::GeneratedFile file(name);
    file.setContents(QLatin1String(Constants::C_PY_SOURCE_CONTENT));
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    return (Core::GeneratedFiles() << file);
}

} // namespace PythonEditor

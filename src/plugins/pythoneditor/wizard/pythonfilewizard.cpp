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

/**
 * @brief The FileWizard class - adds wizard for creating new Python source file
 */

#include "pythonfilewizard.h"
#include "../pythoneditorconstants.h"

#include <coreplugin/basefilewizard.h>
#include <texteditor/textfilewizard.h>

#include <utils/filewizardpage.h>
#include <utils/qtcassert.h>

#include <QWizard>

namespace PythonEditor {

/**
 * @brief Initialize wizard and add new option to "New..." dialog.
 * @param parent
 */
FileWizard::FileWizard()
{
    setWizardKind(Core::IWizardFactory::FileWizard);
    setId(QLatin1String(Constants::C_PY_SOURCE_WIZARD_ID));
    setCategory(QLatin1String(Constants::C_PY_WIZARD_CATEGORY));
    setDisplayCategory(QLatin1String(Constants::C_PY_DISPLAY_CATEGORY));
    setDisplayName(FileWizard::tr(Constants::EN_PY_SOURCE_DISPLAY_NAME));
    setDescription(FileWizard::tr(Constants::EN_PY_SOURCE_DESCRIPTION));
}

/**
 * @brief FileWizard::createWizardDialog
 * @param parent
 * @param params
 * @return
 */
Core::BaseFileWizard *FileWizard::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    Core::BaseFileWizard *wizard = new Core::BaseFileWizard(parent);
    wizard->setWindowTitle(tr("New %1").arg(displayName()));

    Utils::FileWizardPage *page = new Utils::FileWizardPage;
    page->setPath(parameters.defaultPath());
    wizard->addPage(page);

    foreach (QWizardPage *p, parameters.extensionPages())
        wizard->addPage(p);

    return wizard;
}

Core::GeneratedFiles FileWizard::generateFiles(const QWizard *dialog,
                                                QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const Core::BaseFileWizard *wizard =
            qobject_cast<const Core::BaseFileWizard *>(dialog);

    Utils::FileWizardPage *page = wizard->find<Utils::FileWizardPage>();
    QTC_ASSERT(page, return Core::GeneratedFiles());

    QString folder = page->path();
    QString name = page->fileName();

    name = Core::BaseFileWizardFactory::buildFileName(
                folder, name, QLatin1String(Constants::C_PY_EXTENSION));
    Core::GeneratedFile file(name);
    file.setContents(QLatin1String(Constants::C_PY_SOURCE_CONTENT));
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    return (Core::GeneratedFiles() << file);
}

} // namespace PythonEditor

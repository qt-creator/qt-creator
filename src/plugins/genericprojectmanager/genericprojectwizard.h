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

#ifndef GENERICPROJECTWIZARD_H
#define GENERICPROJECTWIZARD_H

#include <coreplugin/basefilewizard.h>
#include <utils/wizard.h>

namespace Utils { class FileWizardPage; }

namespace GenericProjectManager {
namespace Internal {

class FilesSelectionWizardPage;

class GenericProjectWizardDialog : public Utils::Wizard
{
    Q_OBJECT

public:
    GenericProjectWizardDialog(QWidget *parent = 0);

    QString path() const;
    void setPath(const QString &path);
    QStringList selectedFiles() const;
    QStringList selectedPaths() const;

    QString projectName() const;

    Utils::FileWizardPage *m_firstPage;
    FilesSelectionWizardPage *m_secondPage;
};

class GenericProjectWizard : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    GenericProjectWizard();
    Core::FeatureSet requiredFeatures() const;

    static Core::BaseFileWizardParameters parameters();

protected:
    QWizard *createWizardDialog(QWidget *parent,
                                const Core::WizardDialogParameters &wizardDialogParameters) const;
    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const;
    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);
};

} // namespace Internal
} // namespace GenericProjectManager

#endif // GENERICPROJECTWIZARD_H

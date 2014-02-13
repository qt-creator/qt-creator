/****************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry Limited (blackberry-qt@qnx.com)
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

#ifndef QNX_BLACKBERRY_CASCADESIMPORT_WIZARD_H
#define QNX_BLACKBERRY_CASCADESIMPORT_WIZARD_H

#include "fileconverter.h"

#include <coreplugin/basefilewizard.h>
#include <utils/wizard.h>

namespace Utils { class ProjectIntroPage; }

namespace Qnx {
namespace Internal {

class SrcProjectWizardPage;

class CascadesImportWizardDialog : public Utils::Wizard
{
    Q_OBJECT
public:
    CascadesImportWizardDialog(QWidget *parent = 0);

    QString srcProjectPath() const;
    QString destProjectPath() const;

    QString projectName() const;
private slots:
    void onSrcProjectPathChanged(const QString &path);
private:
    SrcProjectWizardPage *m_srcProjectPage;
    Utils::ProjectIntroPage *m_destProjectPage;
};

class CascadesImportWizard : public Core::BaseFileWizard
{
    Q_OBJECT
public:
    CascadesImportWizard();

protected:
    ExtensionList selectExtensions();
    QWizard* createWizardDialog(QWidget *parent,
                                const Core::WizardDialogParameters &wizardDialogParameters) const;
    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const;
    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);
private:
    bool collectFiles(ConvertedProjectContext &projectContext, QString &errorMessage) const;
    void collectFiles_helper(QStringList &filePaths, ConvertedProjectContext &projectContext,
                             const QString &rootPath, const QList< QRegExp > &blackList) const;
    bool convertFile(Core::GeneratedFile &file, ConvertedProjectContext &projectContext,
                     QString &errorMessage) const;
    bool convertFilePath(Core::GeneratedFile &file, ConvertedProjectContext &projectContext,
                         QString &errorMessage) const;
    bool convertFileContent(Core::GeneratedFile &file, ConvertedProjectContext &projectContext,
                            QString &errorMessage) const;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_BLACKBERRY_CASCADESIMPORT_WIZARD_H

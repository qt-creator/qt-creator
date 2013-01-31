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

#ifndef PROJECTFILEWIZARDEXTENSION2_H
#define PROJECTFILEWIZARDEXTENSION2_H

#include "projectexplorer_export.h"

#include <coreplugin/ifilewizardextension.h>

namespace ProjectExplorer {

namespace Internal {

struct ProjectWizardContext;

class PROJECTEXPLORER_EXPORT ProjectFileWizardExtension : public Core::IFileWizardExtension
{
    Q_OBJECT
public:
    explicit ProjectFileWizardExtension();
    ~ProjectFileWizardExtension();

    QList<QWizardPage *> extensionPages(const Core::IWizard *wizard);
    bool processFiles(const QList<Core::GeneratedFile> &files,
                 bool *removeOpenProjectAttribute, QString *errorMessage);
    void applyCodeStyle(Core::GeneratedFile *file) const;

    QStringList getProjectChoices() const;
    QStringList getProjectToolTips() const;

    void hideProjectComboBox();

    void setProjectIndex(int i);

public slots:
    void firstExtensionPageShown(const QList<Core::GeneratedFile> &files, const QVariantMap &extraValues);
    void initializeVersionControlChoices();

private:
    void initProjectChoices(const QString &generatedProjectFilePath);
    bool processProject(const QList<Core::GeneratedFile> &files,
                        bool *removeOpenProjectAttribute, QString *errorMessage);
    bool processVersionControl(const QList<Core::GeneratedFile> &files, QString *errorMessage);

    ProjectWizardContext *m_context;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTFILEWIZARDEXTENSION2_H

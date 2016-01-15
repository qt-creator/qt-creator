/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef PROJECTFILEWIZARDEXTENSION2_H
#define PROJECTFILEWIZARDEXTENSION2_H

#include "projectexplorer_export.h"

#include <coreplugin/ifilewizardextension.h>

namespace ProjectExplorer {
class FolderNode;

namespace Internal {

class ProjectWizardContext;

class PROJECTEXPLORER_EXPORT ProjectFileWizardExtension : public Core::IFileWizardExtension
{
    Q_OBJECT
public:
    explicit ProjectFileWizardExtension();
    ~ProjectFileWizardExtension();

    QList<QWizardPage *> extensionPages(const Core::IWizardFactory *wizard);
    bool processFiles(const QList<Core::GeneratedFile> &files,
                 bool *removeOpenProjectAttribute, QString *errorMessage);
    void applyCodeStyle(Core::GeneratedFile *file) const;

public slots:
    void firstExtensionPageShown(const QList<Core::GeneratedFile> &files, const QVariantMap &extraValues);

private:
    bool processProject(const QList<Core::GeneratedFile> &files,
                        bool *removeOpenProjectAttribute, QString *errorMessage);

    ProjectWizardContext *m_context;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTFILEWIZARDEXTENSION2_H

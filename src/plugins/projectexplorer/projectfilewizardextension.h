// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <coreplugin/ifilewizardextension.h>

namespace ProjectExplorer {
class FolderNode;
class Node;
class Project;

namespace Internal {

class ProjectWizardContext;

class PROJECTEXPLORER_EXPORT ProjectFileWizardExtension : public Core::IFileWizardExtension
{
    Q_OBJECT

public:
    ~ProjectFileWizardExtension() override;

    QList<QWizardPage *> extensionPages(const Core::IWizardFactory *wizard) override;
    bool processFiles(const QList<Core::GeneratedFile> &files,
                      bool *removeOpenProjectAttribute, QString *errorMessage) override;
    void applyCodeStyle(Core::GeneratedFile *file) const override;

public slots:
    void firstExtensionPageShown(const QList<Core::GeneratedFile> &files, const QVariantMap &extraValues) override;

private:
    Node *findWizardContextNode(Node *contextNode, Project *project, const Utils::FilePath &path);
    bool processProject(const QList<Core::GeneratedFile> &files,
                        bool *removeOpenProjectAttribute, QString *errorMessage);

    ProjectWizardContext *m_context = nullptr;
};

} // namespace Internal
} // namespace ProjectExplorer

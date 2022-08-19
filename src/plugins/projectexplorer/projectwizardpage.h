// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "projectnodes.h"

#include <coreplugin/generatedfile.h>
#include <coreplugin/iwizardfactory.h>

#include <utils/wizardpage.h>
#include <utils/treemodel.h>

QT_BEGIN_NAMESPACE
class QTreeView;
class QModelIndex;
QT_END_NAMESPACE

namespace Core { class IVersionControl; }

namespace ProjectExplorer {
namespace Internal {

class AddNewTree;

namespace Ui { class WizardPage; }

// Documentation inside.
class ProjectWizardPage : public Utils::WizardPage
{
    Q_OBJECT

public:
    explicit ProjectWizardPage(QWidget *parent = nullptr);
    ~ProjectWizardPage() override;

    FolderNode *currentNode() const;

    void setNoneLabel(const QString &label);

    int versionControlIndex() const;
    void setVersionControlIndex(int);
    Core::IVersionControl *currentVersionControl();

    // Returns the common path
    void setFiles(const QStringList &files);

    bool runVersionControl(const QList<Core::GeneratedFile> &files, QString *errorMessage);

    void initializeProjectTree(Node *context, const Utils::FilePaths &paths,
                               Core::IWizardFactory::WizardKind kind,
                               ProjectAction action);

    void initializeVersionControls();
    void setProjectUiVisible(bool visible);

signals:
    void projectNodeChanged();
    void versionControlChanged(int);

private:
    void projectChanged(int);
    void manageVcs();
    void hideVersionControlUiElements();

    void setAdditionalInfo(const QString &text);
    void setAddingSubProject(bool addingSubProject);
    void setBestNode(ProjectExplorer::Internal::AddNewTree *tree);
    void setVersionControls(const QStringList &);
    void setProjectToolTip(const QString &);
    bool expandTree(const QModelIndex &root);

    Ui::WizardPage *m_ui;
    QStringList m_projectToolTips;
    Utils::TreeModel<> m_model;

    QList<Core::IVersionControl*> m_activeVersionControls;
    QString m_commonDirectory;
    bool m_repositoryExists = false;
};

} // namespace Internal
} // namespace ProjectExplorer

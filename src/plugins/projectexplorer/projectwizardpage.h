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

    void initializeProjectTree(Node *context, const QStringList &paths,
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
    void setModel(Utils::TreeModel<> *model);
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

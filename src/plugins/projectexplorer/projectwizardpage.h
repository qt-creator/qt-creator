/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PROJECTWIZARDPAGE_H
#define PROJECTWIZARDPAGE_H

#include "projectnodes.h"

#include <coreplugin/generatedfile.h>
#include <coreplugin/iwizardfactory.h>

#include <utils/wizardpage.h>

QT_BEGIN_NAMESPACE
class QTreeView;
class QModelIndex;
QT_END_NAMESPACE

namespace Core { class IVersionControl; }
namespace Utils { class TreeModel; }

namespace ProjectExplorer {
namespace Internal {

class AddNewTree;

namespace Ui { class WizardPage; }

// Documentation inside.
class ProjectWizardPage : public Utils::WizardPage
{
    Q_OBJECT

public:
    explicit ProjectWizardPage(QWidget *parent = 0);
    virtual ~ProjectWizardPage();

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

signals:
    void projectNodeChanged();
    void versionControlChanged(int);

public slots:
    void initializeVersionControls();

private slots:
    void projectChanged(int);
    void manageVcs();

private:
    void setAdditionalInfo(const QString &text);
    void setAddingSubProject(bool addingSubProject);
    void setModel(Utils::TreeModel *model);
    void setBestNode(ProjectExplorer::Internal::AddNewTree *tree);
    void setVersionControls(const QStringList &);
    void setProjectToolTip(const QString &);
    bool expandTree(const QModelIndex &root);

    Ui::WizardPage *m_ui;
    QStringList m_projectToolTips;
    Utils::TreeModel *m_model;

    QList<Core::IVersionControl*> m_activeVersionControls;
    QString m_commonDirectory;
    bool m_repositoryExists;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTWIZARDPAGE_H

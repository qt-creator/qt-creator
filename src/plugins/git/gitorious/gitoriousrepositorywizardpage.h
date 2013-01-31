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

#ifndef GITORIOUSREPOSITORYWIZARDPAGE_H
#define GITORIOUSREPOSITORYWIZARDPAGE_H

#include <QWizardPage>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
class QStandardItemModel;
class QStandardItem;
class QModelIndex;
class QUrl;
QT_END_NAMESPACE

namespace Gitorious {
namespace Internal {

class GitoriousProjectWizardPage;

namespace Ui {
    class GitoriousRepositoryWizardPage;
}

// A wizard page listing Gitorious repositories in a tree, by repository type.

class GitoriousRepositoryWizardPage : public QWizardPage {
    Q_OBJECT
public:
    explicit GitoriousRepositoryWizardPage(const GitoriousProjectWizardPage *projectPage,
                                           QWidget *parent = 0);
    ~GitoriousRepositoryWizardPage();

    void initializePage();
    bool isComplete() const;

    QString repositoryName() const;
    QUrl repositoryURL() const;

public slots:
    void slotCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

protected:
    void changeEvent(QEvent *e);

private:
    // return the repository (column 0) item.
    QStandardItem *currentItem0() const;
    // return the repository (column 0) item of index.
    QStandardItem *item0FromIndex(const QModelIndex &filterIndex) const;

    Ui::GitoriousRepositoryWizardPage *ui;
    const GitoriousProjectWizardPage *m_projectPage;
    QStandardItemModel *m_model;
    QSortFilterProxyModel *m_filterModel;
    bool m_valid;
};

} // namespace Internal
} // namespace Gitorious
#endif // GITORIOUSREPOSITORYWIZARDPAGE_H

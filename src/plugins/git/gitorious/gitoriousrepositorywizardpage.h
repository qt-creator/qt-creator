/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef GITORIOUSREPOSITORYWIZARDPAGE_H
#define GITORIOUSREPOSITORYWIZARDPAGE_H

#include <QtGui/QWizardPage>

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

    virtual void initializePage();
    virtual bool isComplete() const;

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

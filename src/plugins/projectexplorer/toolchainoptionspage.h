/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TOOLCHAINOPTIONSPAGE_H
#define TOOLCHAINOPTIONSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>

#include <QAbstractItemModel>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QItemSelectionModel;
class QPushButton;
class QTreeView;
QT_END_NAMESPACE

namespace Utils { class DetailsWidget; }

namespace ProjectExplorer {

class ToolChain;
class ToolChainConfigWidget;
class ToolChainFactory;

namespace Internal {

class ToolChainNode;
// --------------------------------------------------------------------------
// ToolChainModel
// --------------------------------------------------------------------------

class ToolChainModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ToolChainModel(QObject *parent = 0);
    ~ToolChainModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(const QModelIndex &topIdx, ToolChain *) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    ToolChain *toolChain(const QModelIndex &);
    int manualToolChains() const;

    ToolChainConfigWidget *widget(const QModelIndex &);

    bool isDirty() const;
    bool isDirty(ToolChain *) const;

    void apply();

    void markForRemoval(ToolChain *);
    void markForAddition(ToolChain *);

signals:
    void toolChainStateChanged();

private slots:
    void addToolChain(ProjectExplorer::ToolChain *);
    void removeToolChain(ProjectExplorer::ToolChain *);
    void setDirty();

private:
    QModelIndex index(ToolChainNode *, int column = 0) const;
    ToolChainNode *createNode(ToolChainNode *parent, ToolChain *tc, bool changed);
    static ToolChainNode *findToolChain(const QList<ToolChainNode *> &container, ToolChain *tc);


    ToolChainNode *m_root;
    ToolChainNode *m_autoRoot;
    ToolChainNode *m_manualRoot;

    QList<ToolChainNode *> m_toAddList;
    QList<ToolChainNode *> m_toRemoveList;
};

// --------------------------------------------------------------------------
// ToolChainOptionsPage
// --------------------------------------------------------------------------

class ToolChainOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    ToolChainOptionsPage();

    QWidget *widget();
    void apply();
    void finish();

private slots:
    void toolChainSelectionChanged();
    void createToolChain(QObject *);
    void removeToolChain();
    void updateState();

private:
    QModelIndex currentIndex() const;

    QPointer<QWidget> m_configWidget;

    ToolChainModel *m_model;
    QList<ToolChainFactory *> m_factories;
    QItemSelectionModel * m_selectionModel;
    QTreeView *m_toolChainView;
    Utils::DetailsWidget *m_container;
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TOOLCHAINOPTIONSPAGE_H

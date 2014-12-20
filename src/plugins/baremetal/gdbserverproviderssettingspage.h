/****************************************************************************
**
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
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

#ifndef GDBSERVERPROVIDERSSETTINGSPAGE_H
#define GDBSERVERPROVIDERSSETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>

#include <QAbstractItemModel>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QItemSelectionModel;
class QPushButton;
class QTreeView;
QT_END_NAMESPACE

namespace Utils { class DetailsWidget; }

namespace BareMetal {
namespace Internal {

class GdbServerProvider;
class GdbServerProviderConfigWidget;
class GdbServerProviderNode;

class GdbServerProviderModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit GdbServerProviderModel(QObject *parent = 0);
    ~GdbServerProviderModel();

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;

    QModelIndex index(const QModelIndex &topIdx, const GdbServerProvider *) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;

    GdbServerProvider *provider(const QModelIndex &) const;

    GdbServerProviderConfigWidget *widget(const QModelIndex &) const;

    bool isDirty() const;
    bool isDirty(GdbServerProvider *) const;

    void apply();

    void markForRemoval(GdbServerProvider *);
    void markForAddition(GdbServerProvider *);

signals:
    void providerStateChanged();

private slots:
    void addProvider(GdbServerProvider *);
    void removeProvider(GdbServerProvider *);
    void setDirty();

private:
    enum ColumnIndex { NameIndex = 0, TypeIndex, ColumnsCount };

    QModelIndex index(GdbServerProviderNode *, int column = 0) const;

    GdbServerProviderNode *createNode(GdbServerProviderNode *parent,
                                      GdbServerProvider *, bool changed);

    GdbServerProviderNode *nodeFromIndex(const QModelIndex &) const;

    static GdbServerProviderNode *findNode(
            const QList<GdbServerProviderNode *> &container,
            const GdbServerProvider *);

    GdbServerProviderNode *m_root;
    QList<GdbServerProviderNode *> m_toAddNodes;
    QList<GdbServerProviderNode *> m_toRemoveNodes;
};

class GdbServerProvidersSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit GdbServerProvidersSettingsPage(QObject *parent = 0);

    QWidget *widget();
    void apply();
    void finish();

private slots:
    void providerSelectionChanged();
    void createProvider(QObject *);
    void removeProvider();
    void updateState();

private:
    QModelIndex currentIndex() const;

    QPointer<QWidget> m_configWidget;

    QPointer<GdbServerProviderModel> m_model;
    QPointer<QItemSelectionModel> m_selectionModel;
    QPointer<QTreeView> m_providerView;
    QPointer<Utils::DetailsWidget> m_container;
    QPointer<QPushButton> m_addButton;
    QPointer<QPushButton> m_cloneButton;
    QPointer<QPushButton> m_delButton;
};

} // namespace Internal
} // namespace BareMetal

#endif // GDBSERVERPROVIDERSSETTINGSPAGE_H

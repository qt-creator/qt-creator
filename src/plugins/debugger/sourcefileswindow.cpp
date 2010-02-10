/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "sourcefileswindow.h"
#include "debuggeractions.h"
#include "debuggermanager.h"

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>

#include <QtGui/QAction>
#include <QtGui/QComboBox>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>
#include <QtGui/QResizeEvent>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>

using Debugger::Internal::SourceFilesWindow;
using Debugger::Internal::SourceFilesModel;

//////////////////////////////////////////////////////////////////
//
// SourceFilesModel
//
//////////////////////////////////////////////////////////////////

class Debugger::Internal::SourceFilesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    SourceFilesModel(QObject *parent = 0) : QAbstractItemModel(parent) {}

    // QAbstractItemModel
    int columnCount(const QModelIndex &parent) const
        { return parent.isValid() ? 0 : 2; }
    int rowCount(const QModelIndex &parent) const
        { return parent.isValid() ? 0 : m_shortNames.size(); }
    QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }
    QModelIndex index(int row, int column, const QModelIndex &) const
        { return createIndex(row, column); }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void clearModel();
    void update() { reset(); }
    void setSourceFiles(const QMap<QString, QString> &sourceFiles);

public:
    QStringList m_shortNames;
    QStringList m_fullNames;
};

void SourceFilesModel::clearModel()
{
    if (m_shortNames.isEmpty())
        return;
    m_shortNames.clear();
    m_fullNames.clear();
    reset();
}

QVariant SourceFilesModel::headerData(int section,
    Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static QString headers[] = {
            tr("Internal name") + "        ",
            tr("Full name") + "        ",
        };
        return headers[section];
    }
    return QVariant();
}

Qt::ItemFlags SourceFilesModel::flags(const QModelIndex &index) const
{
    if (index.row() >= m_fullNames.size())
        return 0;
    QFileInfo fi(m_fullNames.at(index.row()));
    return fi.isReadable() ? QAbstractItemModel::flags(index) : Qt::ItemFlags(0);
}

QVariant SourceFilesModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= m_shortNames.size())
        return QVariant();

    switch (index.column()) {
        case 0:
            if (role == Qt::DisplayRole)
                return m_shortNames.at(row);
            // FIXME: add icons
            //if (role == Qt::DecorationRole)
            //    return module.symbolsRead ? icon2 : icon;
            break;
        case 1:
            if (role == Qt::DisplayRole)
                return m_fullNames.at(row);
            //if (role == Qt::DecorationRole)
            //    return module.symbolsRead ? icon2 : icon;
            break;
    }
    return QVariant();
}

bool SourceFilesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return QAbstractItemModel::setData(index, value, role);
}

void SourceFilesModel::setSourceFiles(const QMap<QString, QString> &sourceFiles)
{
    m_shortNames.clear();
    m_fullNames.clear();
    QMap<QString, QString>::ConstIterator it = sourceFiles.begin();
    QMap<QString, QString>::ConstIterator et = sourceFiles.end();
    for (; it != et; ++it) {
        m_shortNames.append(it.key());
        m_fullNames.append(it.value());
    }
    reset();
}

//////////////////////////////////////////////////////////////////
//
// SourceFilesWindow
//
//////////////////////////////////////////////////////////////////

SourceFilesWindow::SourceFilesWindow(QWidget *parent)
    : QTreeView(parent)
{
    m_model = new SourceFilesModel(this);
    QAction *act = theDebuggerAction(UseAlternatingRowColors);

    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(m_model);
    setModel(proxyModel);

    setWindowTitle(tr("Source Files"));
    setSortingEnabled(true);
    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));
    //header()->setDefaultAlignment(Qt::AlignLeft);

    connect(this, SIGNAL(activated(QModelIndex)),
        this, SLOT(sourceFileActivated(QModelIndex)));
    connect(act, SIGNAL(toggled(bool)),
        this, SLOT(setAlternatingRowColorsHelper(bool)));
}

void SourceFilesWindow::sourceFileActivated(const QModelIndex &index)
{
    qDebug() << "ACTIVATED: " << index.row() << index.column()
        << model()->data(index);
    emit fileOpenRequested(model()->data(index).toString());
}

void SourceFilesWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QModelIndex index = indexAt(ev->pos());
    index = index.sibling(index.row(), 0);
    QString name = model()->data(index).toString();

    QMenu menu;
    QAction *act1 = new QAction(tr("Reload data"), &menu);
    act1->setEnabled(Debugger::DebuggerManager::instance()->debuggerActionsEnabled());
    //act1->setCheckable(true);
    QAction *act2 = 0;
    if (name.isEmpty()) {
        act2 = new QAction(tr("Open file"), &menu);
        act2->setEnabled(false);
    } else {
        act2 = new QAction(tr("Open file \"%1\"'").arg(name), &menu);
        act2->setEnabled(true);
    }

    menu.addAction(act1);
    menu.addAction(act2);
    menu.addSeparator();
    menu.addAction(theDebuggerAction(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == act1)
        emit reloadSourceFilesRequested();
    else if (act == act2)
        emit fileOpenRequested(name);
}

void SourceFilesWindow::setSourceFiles(const QMap<QString, QString> &sourceFiles)
{
    m_model->setSourceFiles(sourceFiles);
    header()->setResizeMode(0, QHeaderView::ResizeToContents);
}

void SourceFilesWindow::removeAll()
{
    m_model->setSourceFiles(QMap<QString, QString>());
    header()->setResizeMode(0, QHeaderView::ResizeToContents);
}

#include "sourcefileswindow.moc"

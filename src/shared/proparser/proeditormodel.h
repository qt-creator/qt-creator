/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef PROEDITORMODEL_H
#define PROEDITORMODEL_H

#include "namespace_global.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtGui/QSortFilterProxyModel>

QT_BEGIN_NAMESPACE
class ProBlock;
class ProFile;
class ProItem;
class ProVariable;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class ProCommandManager;
class ProItemInfoManager;

class ProEditorModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ProEditorModel(QObject *parent = 0);
    ~ProEditorModel();

    void setInfoManager(ProItemInfoManager *infomanager);
    ProItemInfoManager *infoManager() const;
    ProCommandManager *cmdManager() const;

    void setProFiles(QList<ProFile*> proFiles);
    QList<ProFile*> proFiles() const;

    QList<QModelIndex> findVariables(const QStringList &varname, const QModelIndex &parent = QModelIndex()) const;
    QList<QModelIndex> findBlocks(const QModelIndex &parent = QModelIndex()) const;

    bool moveItem(const QModelIndex &index, int row);
    bool insertItem(ProItem *item, int row, const QModelIndex &parent);
    bool removeItem(const QModelIndex &index);

    ProItem *proItem(const QModelIndex &index) const;
    ProBlock *proBlock(const QModelIndex &index) const;
    ProVariable *proVariable(const QModelIndex &index) const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const; 
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    Qt::ItemFlags flags(const QModelIndex &index) const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;

    inline QList<ProFile*> changed() const { return m_changed.toList(); }
protected:
    ProItem *createExpressionItem(QString &str) const;

    QString blockName(ProBlock *block) const;
    ProBlock *scopeContents(ProBlock *block) const;

    QString expressionToString(ProBlock *block, bool display = false) const;
    QList<ProItem *> stringToExpression(const QString &exp) const;

    bool insertModelItem(ProItem *item, int row, const QModelIndex &parent);
    bool removeModelItem(const QModelIndex &index);

private:
    void markProFileModified(QModelIndex index);
    ProCommandManager *m_cmdmanager;
    QList<ProFile*> m_proFiles;
    QSet<ProFile*> m_changed;
    ProItemInfoManager *m_infomanager;

    friend class ProAddCommand;
    friend class ProRemoveCommand;
    friend class ChangeProScopeCommand;
    friend class ChangeProAdvancedCommand;
};

class ProScopeFilter : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum CheckableType { 
        None,
        Variable, 
        Blocks
    };
    
    void setVariableFilter(const QStringList &vars);
    void setCheckable( CheckableType ct );

    // returns the checked (source) indexes
    QList<QModelIndex> checkedIndexes() const;

    Qt::ItemFlags flags(const QModelIndex &index) const;
    ProScopeFilter(QObject *parent);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

protected:
    ProVariable *sourceVariable(const QModelIndex &index) const;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    CheckableType m_checkable;
    QStringList m_vars;
    QMap<QModelIndex, bool> m_checkStates;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROEDITORMODEL_H


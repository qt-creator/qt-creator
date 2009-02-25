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

#include "moduleshandler.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QList>
#include <QtCore/QTextStream>

#include <QtGui/QAction>
#include <QtGui/QMainWindow>
#include <QtGui/QStandardItemModel>
#include <QtGui/QSortFilterProxyModel>

using namespace Debugger;
using namespace Debugger::Internal;


//////////////////////////////////////////////////////////////////
//
// ModulesModel
//
//////////////////////////////////////////////////////////////////

class Debugger::Internal::ModulesModel : public QAbstractItemModel
{
public:
    ModulesModel(ModulesHandler *parent)
      : QAbstractItemModel(parent)
    {}

    // QAbstractItemModel
    int columnCount(const QModelIndex &parent) const
        { return parent.isValid() ? 0 : 4; }
    int rowCount(const QModelIndex &parent) const
        { return parent.isValid() ? 0 : m_modules.size(); }
    QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }
    QModelIndex index(int row, int column, const QModelIndex &) const
        { return createIndex(row, column); }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    void clearModel() { if (!m_modules.isEmpty()) { m_modules.clear(); update(); } }
    void update() { reset(); }

public:
    QList<Module> m_modules;
};

QVariant ModulesModel::headerData(int section,
    Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static QString headers[] = {
            tr("Module name") + "        ", 
            tr("Symbols read") + "        ",
            tr("Start address") + "        ",
            tr("End addAress") + "        "
        };
        return headers[section];
    }
    return QVariant();
}

QVariant ModulesModel::data(const QModelIndex &index, int role) const
{
    //static const QIcon icon(":/gdbdebugger/images/breakpoint.svg");
    //static const QIcon icon2(":/gdbdebugger/images/breakpoint_pending.svg");

    int row = index.row();
    if (row < 0 || row >= m_modules.size())
        return QVariant();

    const Module &module = m_modules.at(row);

    switch (index.column()) {
        case 0:
            if (role == Qt::DisplayRole)
                return module.moduleName;
            // FIXME: add icons
            //if (role == Qt::DecorationRole)
            //    return module.symbolsRead ? icon2 : icon;
            break;
        case 1:
            if (role == Qt::DisplayRole)
                return module.symbolsRead ? "yes" : "no"; 
            break;
        case 2:
            if (role == Qt::DisplayRole)
                return module.startAddress;
            break;
        case 3:
            if (role == Qt::DisplayRole)
                return module.endAddress;
            break;
    }
    return QVariant();
}

bool ModulesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return QAbstractItemModel::setData(index, value, role);
}


//////////////////////////////////////////////////////////////////
//
// ModulesHandler
//
//////////////////////////////////////////////////////////////////

ModulesHandler::ModulesHandler()
{
    m_model = new ModulesModel(this);
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
}

QAbstractItemModel *ModulesHandler::model() const
{
    return m_proxyModel;
}

void ModulesHandler::removeAll()
{
    m_model->clearModel();
}


void ModulesHandler::setModules(const QList<Module> &modules)
{
    m_model->m_modules = modules;
    m_model->update();
}

QList<Module> ModulesHandler::modules() const
{
    return m_model->m_modules;
}

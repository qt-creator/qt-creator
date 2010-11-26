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

#include "moduleshandler.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtGui/QSortFilterProxyModel>


//////////////////////////////////////////////////////////////////
//
// ModulesModel
//
//////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

ModulesModel::ModulesModel(ModulesHandler *parent)
  : QAbstractItemModel(parent)
{}

QVariant ModulesModel::headerData(int section,
    Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static QString headers[] = {
            tr("Module name") + "        ",
            tr("Module path") + "        ",
            tr("Symbols read") + "        ",
            tr("Symbols type") + "        ",
            tr("Start address") + "        ",
            tr("End address") + "        "
        };
        return headers[section];
    }
    return QVariant();
}

QVariant ModulesModel::data(const QModelIndex &index, int role) const
{
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
                return module.modulePath;
            break;
        case 2:
            if (role == Qt::DisplayRole)
                switch (module.symbolsRead) {
                    case Module::UnknownReadState: return tr("unknown");
                    case Module::ReadFailed: return tr("no");
                    case Module::ReadOk: return tr("yes");
                }
            break;
        case 3:
            if (role == Qt::DisplayRole)
                switch (module.symbolsType) {
                    case Module::UnknownType: return tr("unknown");
                    case Module::PlainSymbols: return tr("plain");
                    case Module::FastSymbols: return tr("fast");
                }
            break;
        case 4:
            if (role == Qt::DisplayRole)
                return module.startAddress;
            break;
        case 5:
            if (role == Qt::DisplayRole)
                return module.endAddress;
            break;
    }
    return QVariant();
}

void ModulesModel::addModule(const Module &m)
{
    beginInsertRows(QModelIndex(), m_modules.size(), m_modules.size());
    m_modules.push_back(m);
    endInsertRows();
}

void ModulesModel::setModules(const Modules &m)
{
    m_modules = m;
    reset();
}

void ModulesModel::clearModel()
{
    if (!m_modules.isEmpty()) {
        m_modules.clear();
        reset();
    }
}

int ModulesModel::indexOfModule(const QString &name) const
{
    // Recent modules are more likely to be unloaded first.
    for (int i = m_modules.size() - 1; i >= 0; i--)
        if (m_modules.at(i).moduleName == name)
            return i;
    return -1;
}

void ModulesModel::removeModule(const QString &moduleName)
{
    const int index = indexOfModule(moduleName);
    QTC_ASSERT(index != -1, return);
    beginRemoveRows(QModelIndex(), index, index);
    m_modules.remove(index);
    endRemoveRows();
}

void ModulesModel::updateModule(const QString &moduleName, const Module &module)
{
    const int index = indexOfModule(moduleName);
    QTC_ASSERT(index != -1, return);
    m_modules[index] = module;
    reset();
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

void ModulesHandler::addModule(const Module &module)
{
    m_model->addModule(module);
}

void ModulesHandler::removeModule(const QString &moduleName)
{
    m_model->removeModule(moduleName);
}

void ModulesHandler::updateModule(const QString &moduleName, const Module &module)
{
    m_model->updateModule(moduleName, module);
}

void ModulesHandler::setModules(const Modules &modules)
{
    m_model->setModules(modules);
}

Modules ModulesHandler::modules() const
{
    return m_model->modules();
}

} // namespace Internal
} // namespace Debugger


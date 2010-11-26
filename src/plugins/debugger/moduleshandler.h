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

#ifndef DEBUGGER_MODULESHANDLER_H
#define DEBUGGER_MODULESHANDLER_H

#include <QtCore/QVector>
#include <QtCore/QObject>
#include <QtGui/QSortFilterProxyModel>


namespace Debugger {
namespace Internal {

class ModulesModel;
class ModulesHandler;


//////////////////////////////////////////////////////////////////
//
// Symbol
//
//////////////////////////////////////////////////////////////////

class Symbol
{
public:
    QString address;
    QString state;
    QString name;
    QString section;
    QString demangled;
};

typedef QVector<Symbol> Symbols;

//////////////////////////////////////////////////////////////////
//
// Module
//
//////////////////////////////////////////////////////////////////

class Module
{
public:
    Module() : symbolsRead(UnknownReadState), symbolsType(UnknownType) {}

public:
    enum SymbolReadState {
        UnknownReadState,  // Not tried.
        ReadFailed,        // Tried to read, but failed.
        ReadOk,            // Dwarf index available.
    };
    enum SymbolType {
        UnknownType,       // Unknown.
        PlainSymbols,      // Ordinary symbols available.
        FastSymbols,       // Dwarf index available.
    };
    QString moduleName;
    QString modulePath;
    SymbolReadState symbolsRead;
    SymbolType symbolsType;
    QString startAddress;
    QString endAddress;
};

typedef QVector<Module> Modules;


//////////////////////////////////////////////////////////////////
//
// ModulesModel
//
//////////////////////////////////////////////////////////////////

class ModulesModel : public QAbstractItemModel
{
    // Needs tr - context.
    Q_OBJECT
public:
    ModulesModel(ModulesHandler *parent);

    // QAbstractItemModel
    int columnCount(const QModelIndex &parent) const
        { return parent.isValid() ? 0 : 5; }
    int rowCount(const QModelIndex &parent) const
        { return parent.isValid() ? 0 : m_modules.size(); }
    QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }
    QModelIndex index(int row, int column, const QModelIndex &) const
        { return createIndex(row, column); }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    void clearModel();
    void addModule(const Module &module);
    void removeModule(const QString &moduleName);
    void setModules(const Modules &modules);
    void updateModule(const QString &moduleName, const Module &module);

    const Modules &modules() const { return m_modules; }

private:
    int indexOfModule(const QString &name) const;

    Modules m_modules;
};


//////////////////////////////////////////////////////////////////
//
// ModulesHandler
//
//////////////////////////////////////////////////////////////////

class ModulesHandler : public QObject
{
    Q_OBJECT

public:
    ModulesHandler();

    QAbstractItemModel *model() const;

    void setModules(const Modules &modules);
    void addModule(const Module &module);
    void removeModule(const QString &moduleName);
    void updateModule(const QString &moduleName, const Module &module);

    Modules modules() const;
    void removeAll();

private:
    ModulesModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_MODULESHANDLER_H

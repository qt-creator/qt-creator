/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <utils/elfreader.h>
#include <utils/treemodel.h>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class DebuggerEngine;
class ModuleItem;

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
// Section
//
//////////////////////////////////////////////////////////////////

class Section
{
public:
    QString from;
    QString to;
    QString address;
    QString name;
    QString flags;
};

typedef QVector<Section> Sections;

//////////////////////////////////////////////////////////////////
//
// Module
//
//////////////////////////////////////////////////////////////////

class Module
{
public:
    Module() : symbolsRead(UnknownReadState), startAddress(0), endAddress(0) {}

public:
    enum SymbolReadState {
        UnknownReadState,  // Not tried.
        ReadFailed,        // Tried to read, but failed.
        ReadOk             // Dwarf index available.
    };
    QString moduleName;
    QString modulePath;
    QString hostPath;
    SymbolReadState symbolsRead;
    quint64 startAddress;
    quint64 endAddress;

    Utils::ElfData elfData;
};

typedef QVector<Module> Modules;

//////////////////////////////////////////////////////////////////
//
// ModulesHandler
//
//////////////////////////////////////////////////////////////////

class ModulesModel;

class ModulesHandler : public QObject
{
    Q_OBJECT

public:
    explicit ModulesHandler(DebuggerEngine *engine);

    QAbstractItemModel *model() const;

    void removeModule(const QString &modulePath);
    void updateModule(const Module &module);

    void beginUpdateAll();
    void endUpdateAll();

    void removeAll();
    Modules modules() const;

private:
    ModuleItem *moduleFromPath(const QString &modulePath) const;

    ModulesModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
};

} // namespace Internal
} // namespace Debugger

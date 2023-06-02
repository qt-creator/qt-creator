// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/elfreader.h>
#include <utils/treemodel.h>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Debugger::Internal {

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

using Symbols = QVector<Symbol>;

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

using Sections = QVector<Section>;

//////////////////////////////////////////////////////////////////
//
// Module
//
//////////////////////////////////////////////////////////////////

class Module
{
public:
    Module() = default;

public:
    enum SymbolReadState {
        UnknownReadState,  // Not tried.
        ReadFailed,        // Tried to read, but failed.
        ReadOk             // Dwarf index available.
    };
    QString moduleName;
    Utils::FilePath modulePath;
    QString hostPath;
    SymbolReadState symbolsRead = UnknownReadState;
    quint64 startAddress = 0;
    quint64 endAddress = 0;

    Utils::ElfData elfData;
};

using Modules = QVector<Module>;

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
    ~ModulesHandler() override;

    QAbstractItemModel *model() const;

    void removeModule(const Utils::FilePath &modulePath);
    void updateModule(const Module &module);

    void beginUpdateAll();
    void endUpdateAll();

    void removeAll();
    const Modules modules() const;

private:
    ModuleItem *moduleFromPath(const Utils::FilePath &modulePath) const;

    ModulesModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
};

} // Debugger::Internal

/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_MODULESHANDLER_H
#define DEBUGGER_MODULESHANDLER_H

#include <utils/elfreader.h>
#include <utils/treemodel.h>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class DebuggerEngine;

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
    DebuggerEngine *m_engine;
    Utils::TreeModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_MODULESHANDLER_H

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

#ifndef DEBUGGER_MODULESHANDLER_H
#define DEBUGGER_MODULESHANDLER_H

#include <QtCore/QList>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QSortFilterProxyModel;
QT_END_NAMESPACE


namespace Debugger {
namespace Internal {

class ModulesModel;

enum ModulesModelRoles
{
    DisplaySourceRole = Qt::UserRole,
    LoadSymbolsRole,
    LoadAllSymbolsRole
};


//////////////////////////////////////////////////////////////////
//
// Module
//
//////////////////////////////////////////////////////////////////

class Module
{
public:
    Module() : symbolsRead(false) {}

public:
    QString moduleName;
    bool symbolsRead;
    QString startAddress;
    QString endAddress;
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

    void setModules(const QList<Module> &modules);
    QList<Module> modules() const;
    void removeAll();

private:
    ModulesModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_MODULESHANDLER_H

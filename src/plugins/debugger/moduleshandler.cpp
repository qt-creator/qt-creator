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

#include "moduleshandler.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <utils/basetreeview.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/treemodel.h>

#include <QCoreApplication>
#include <QDebug>
#include <QMenu>
#include <QSortFilterProxyModel>

#include <functional>

using namespace Utils;

namespace Debugger {
namespace Internal {

class ModuleItem : public TreeItem
{
    Q_DECLARE_TR_FUNCTIONS(Debuggger::Internal::ModulesHandler)

public:
    QVariant data(int column, int role) const override;

public:
    Module module;
    bool updated;
};

QVariant ModuleItem::data(int column, int role) const
{
    switch (column) {
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
        if (role == Qt::ToolTipRole) {
            QString msg;
            if (!module.elfData.buildId.isEmpty())
                msg += QString::fromLatin1("Build Id: " + module.elfData.buildId);
            if (!module.elfData.debugLink.isEmpty())
                msg += QString::fromLatin1("Debug Link: " + module.elfData.debugLink);
            return msg;
        }
        break;
    case 2:
        if (role == Qt::DisplayRole)
            switch (module.symbolsRead) {
            case Module::UnknownReadState: return tr("Unknown");
            case Module::ReadFailed:       return tr("No");
            case Module::ReadOk:           return tr("Yes");
            }
        break;
    case 3:
        if (role == Qt::DisplayRole)
            switch (module.elfData.symbolsType) {
            case UnknownSymbols: return tr("Unknown");
            case NoSymbols:      return tr("None");
            case PlainSymbols:   return tr("Plain");
            case FastSymbols:    return tr("Fast");
            case LinkedSymbols:  return tr("debuglnk");
            case BuildIdSymbols: return tr("buildid");
            }
        else if (role == Qt::ToolTipRole)
            switch (module.elfData.symbolsType) {
            case UnknownSymbols:
                return tr("It is unknown whether this module contains debug "
                          "information.\nUse \"Examine Symbols\" from the "
                          "context menu to initiate a check.");
            case NoSymbols:
                return tr("This module neither contains nor references debug "
                          "information.\nStepping into the module or setting "
                          "breakpoints by file and line will not work.");
            case PlainSymbols:
                return tr("This module contains debug information.\nStepping "
                          "into the module or setting breakpoints by file and "
                          "line is expected to work.");
            case FastSymbols:
                return tr("This module contains debug information.\nStepping "
                          "into the module or setting breakpoints by file and "
                          "line is expected to work.");
            case LinkedSymbols:
            case BuildIdSymbols:
                return tr("This module does not contain debug information "
                          "itself, but contains a reference to external "
                          "debug information.");
            }
        break;
    case 4:
        if (role == Qt::DisplayRole)
            if (module.startAddress)
                return QString("0x" + QString::number(module.startAddress, 16));
        break;
    case 5:
        if (role == Qt::DisplayRole) {
            if (module.endAddress)
                return QString("0x" + QString::number(module.endAddress, 16));
            //: End address of loaded module
            return tr("<unknown>", "address");
        }
        break;
    }
    return QVariant();
}

//////////////////////////////////////////////////////////////////
//
// ModulesModel
//
//////////////////////////////////////////////////////////////////

class ModulesModel : public TreeModel<TypedTreeItem<ModuleItem>, ModuleItem>
{
    Q_DECLARE_TR_FUNCTIONS(Debuggger::Internal::ModulesHandler)

public:
    bool setData(const QModelIndex &idx, const QVariant &data, int role) override
    {
        if (role == BaseTreeView::ItemViewEventRole) {
            ItemViewEvent ev = data.value<ItemViewEvent>();
            if (ev.type() == QEvent::ContextMenu)
                return contextMenuEvent(ev);
        }

        return TreeModel::setData(idx, data, role);
    }

    bool contextMenuEvent(const ItemViewEvent &ev);

    DebuggerEngine *engine;
};

bool ModulesModel::contextMenuEvent(const ItemViewEvent &ev)
{
    ModuleItem *item = itemForIndexAtLevel<1>(ev.index());

    const bool enabled = engine->debuggerActionsEnabled();
    const bool canReload = engine->hasCapability(ReloadModuleCapability);
    const bool canLoadSymbols = engine->hasCapability(ReloadModuleSymbolsCapability);
    const bool canShowSymbols = engine->hasCapability(ShowModuleSymbolsCapability);
    const bool moduleNameValid = item && !item->module.moduleName.isEmpty();
    const QString moduleName = item ? item->module.moduleName : QString();
    const QString modulePath = item ? item->module.modulePath : QString();

    auto menu = new QMenu;

    addAction(menu, tr("Update Module List"),
              enabled && canReload,
              [this] { engine->reloadModules(); });

    addAction(menu, tr("Show Source Files for Module \"%1\"").arg(moduleName),
              tr("Show Source Files for Module"),
              moduleNameValid && enabled && canReload,
              [this, modulePath] { engine->loadSymbols(modulePath); });

    // FIXME: Dependencies only available on Windows, when "depends" is installed.
    addAction(menu, tr("Show Dependencies of \"%1\"").arg(moduleName),
              tr("Show Dependencies"),
              moduleNameValid && !moduleName.isEmpty() && HostOsInfo::isWindowsHost(),
              [this, modulePath] { QProcess::startDetached("depends", {modulePath}); });

    addAction(menu, tr("Load Symbols for All Modules"),
              enabled && canLoadSymbols,
              [this] { engine->loadAllSymbols(); });

    addAction(menu, tr("Examine All Modules"),
              enabled && canLoadSymbols,
              [this] { engine->examineModules(); });

    addAction(menu, tr("Load Symbols for Module \"%1\"").arg(moduleName),
              tr("Load Symbols for Module"),
              moduleNameValid && canLoadSymbols,
              [this, modulePath] { engine->loadSymbols(modulePath); });

    addAction(menu, tr("Edit File \"%1\"").arg(moduleName),
              tr("Edit File"),
              moduleNameValid,
              [this, modulePath] { engine->gotoLocation(modulePath); });

    addAction(menu, tr("Show Symbols in File \"%1\"").arg(moduleName),
              tr("Show Symbols"),
              canShowSymbols && moduleNameValid,
              [this, modulePath] { engine->requestModuleSymbols(modulePath); });

    addAction(menu, tr("Show Sections in File \"%1\"").arg(moduleName),
              tr("Show Sections"),
              canShowSymbols && moduleNameValid,
              [this, modulePath] { engine->requestModuleSections(modulePath); });

    menu->addSeparator();
    menu->addAction(action(SettingsDialog));

    menu->popup(ev.globalPos());
    return true;
}

//////////////////////////////////////////////////////////////////
//
// ModulesHandler
//
//////////////////////////////////////////////////////////////////

ModulesHandler::ModulesHandler(DebuggerEngine *engine)
{
    QString pad = "        ";
    m_model = new ModulesModel;
    m_model->engine = engine;
    m_model->setObjectName("ModulesModel");
    m_model->setHeader(QStringList({
        tr("Module Name") + pad,
        tr("Module Path") + pad,
        tr("Symbols Read") + pad,
        tr("Symbols Type") + pad,
        tr("Start Address") + pad,
        tr("End Address") + pad}));

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setObjectName("ModulesProxyModel");
    m_proxyModel->setSourceModel(m_model);
}

QAbstractItemModel *ModulesHandler::model() const
{
    return m_proxyModel;
}

ModuleItem *ModulesHandler::moduleFromPath(const QString &modulePath) const
{
    // Recent modules are more likely to be unloaded first.
    return m_model->findItemAtLevel<1>([modulePath](ModuleItem *item) {
        return item->module.modulePath == modulePath;
    });
}

void ModulesHandler::removeAll()
{
    m_model->clear();
}

Modules ModulesHandler::modules() const
{
    Modules mods;
    m_model->forItemsAtLevel<1>([&mods](ModuleItem *item) { mods.append(item->module); });
    return mods;
}

void ModulesHandler::removeModule(const QString &modulePath)
{
    if (ModuleItem *item = moduleFromPath(modulePath))
        m_model->destroyItem(item);
}

void ModulesHandler::updateModule(const Module &module)
{
    const QString path = module.modulePath;
    if (path.isEmpty())
        return;

    ModuleItem *item = moduleFromPath(path);
    if (item) {
        item->module = module;
    } else {
        item = new ModuleItem;
        item->module = module;
        m_model->rootItem()->appendChild(item);
    }

    try { // MinGW occasionallly throws std::bad_alloc.
        ElfReader reader(path);
        item->module.elfData = reader.readHeaders();
        item->update();
    } catch(...) {
        qWarning("%s: An exception occurred while reading module '%s'",
                 Q_FUNC_INFO, qPrintable(module.modulePath));
    }
    item->updated = true;
}

void ModulesHandler::beginUpdateAll()
{
    m_model->forItemsAtLevel<1>([](ModuleItem *item) { item->updated = false; });
}

void ModulesHandler::endUpdateAll()
{
    QList<TreeItem *> toDestroy;
    m_model->forItemsAtLevel<1>([&toDestroy](ModuleItem *item) {
        if (!item->updated)
            toDestroy.append(item);
    });
    qDeleteAll(toDestroy);
}

} // namespace Internal
} // namespace Debugger

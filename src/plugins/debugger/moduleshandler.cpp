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

#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <utils/basetreeview.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
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
    bool setData(int column, const QVariant &data, int role) override;

    bool contextMenuEvent(const ItemViewEvent &event);

public:
    DebuggerEngine *engine;
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

bool ModuleItem::setData(int, const QVariant &data, int role)
{
    if (role == BaseTreeView::ItemActivatedRole) {
        engine->gotoLocation(module.modulePath);
        return true;
    }

    if (role == BaseTreeView::ItemViewEventRole) {
        ItemViewEvent ev = data.value<ItemViewEvent>();
        if (ev.type() == QEvent::ContextMenu)
            return contextMenuEvent(ev);
    }

    return false;
}

bool ModuleItem::contextMenuEvent(const ItemViewEvent &event)
{
    auto menu = new QMenu;

    const bool enabled = engine->debuggerActionsEnabled();
    const bool canReload = engine->hasCapability(ReloadModuleCapability);
    const bool canLoadSymbols = engine->hasCapability(ReloadModuleSymbolsCapability);
    const bool canShowSymbols = engine->hasCapability(ShowModuleSymbolsCapability);
    const bool moduleNameValid = !module.moduleName.isEmpty();

    addAction(menu, tr("Update Module List"),
              enabled && canReload,
              [this] { engine->reloadModules(); });

    addAction(menu, tr("Show Source Files for Module \"%1\"").arg(module.moduleName),
              enabled && canReload,
              [this] { engine->loadSymbols(module.modulePath); });

    // FIXME: Dependencies only available on Windows, when "depends" is installed.
    addAction(menu, tr("Show Dependencies of \"%1\"").arg(module.moduleName),
              tr("Show Dependencies"),
              moduleNameValid && !module.modulePath.isEmpty() && HostOsInfo::isWindowsHost(),
              [this] { QProcess::startDetached("depends", QStringList(module.modulePath)); });

    addAction(menu, tr("Load Symbols for All Modules"),
              enabled && canLoadSymbols,
              [this] { engine->loadAllSymbols(); });

    addAction(menu, tr("Examine All Modules"),
              enabled && canLoadSymbols,
              [this] { engine->examineModules(); });

    addAction(menu, tr("Load Symbols for Module \"%1\"").arg(module.moduleName),
              tr("Load Symbols for Module"),
              canLoadSymbols,
              [this] { engine->loadSymbols(module.modulePath); });

    addAction(menu, tr("Edit File \"%1\"").arg(module.moduleName),
              tr("Edit File"),
              moduleNameValid,
              [this] { engine->gotoLocation(module.modulePath); });

    addAction(menu, tr("Show Symbols in File \"%1\"").arg(module.moduleName),
              tr("Show Symbols"),
              canShowSymbols && moduleNameValid,
              [this] { engine->requestModuleSymbols(module.modulePath); });

    addAction(menu, tr("Show Sections in File \"%1\"").arg(module.moduleName),
              tr("Show Sections"),
              canShowSymbols && moduleNameValid,
              [this] { engine->requestModuleSections(module.modulePath); });

    menu->popup(event.globalPos());
    return true;
}

//////////////////////////////////////////////////////////////////
//
// ModulesHandler
//
//////////////////////////////////////////////////////////////////

ModulesHandler::ModulesHandler(DebuggerEngine *engine)
{
    m_engine = engine;

    QString pad = "        ";
    m_model = new ModulesModel;
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
        item->engine = m_engine;
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

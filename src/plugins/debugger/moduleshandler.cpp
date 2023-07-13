// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "moduleshandler.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggertr.h"

#include <utils/basetreeview.h>
#include <utils/hostosinfo.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#include <QCoreApplication>
#include <QDebug>
#include <QMenu>
#include <QSortFilterProxyModel>

#include <functional>

using namespace Utils;

namespace Debugger::Internal {

class ModuleItem : public TreeItem
{
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
            return module.modulePath.toUserOutput();
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
            case Module::UnknownReadState: return Tr::tr("Unknown");
            case Module::ReadFailed:       return Tr::tr("No");
            case Module::ReadOk:           return Tr::tr("Yes");
            }
        break;
    case 3:
        if (role == Qt::DisplayRole)
            switch (module.elfData.symbolsType) {
            case UnknownSymbols: return Tr::tr("Unknown");
            case NoSymbols:      return Tr::tr("None");
            case PlainSymbols:   return Tr::tr("Plain");
            case FastSymbols:    return Tr::tr("Fast");
            case LinkedSymbols:  return Tr::tr("debuglnk");
            case BuildIdSymbols: return Tr::tr("buildid");
            }
        else if (role == Qt::ToolTipRole)
            switch (module.elfData.symbolsType) {
            case UnknownSymbols:
                return Tr::tr("It is unknown whether this module contains debug "
                          "information.\nUse \"Examine Symbols\" from the "
                          "context menu to initiate a check.");
            case NoSymbols:
                return Tr::tr("This module neither contains nor references debug "
                          "information.\nStepping into the module or setting "
                          "breakpoints by file and line will not work.");
            case PlainSymbols:
            case FastSymbols:
                return Tr::tr("This module contains debug information.\nStepping "
                          "into the module or setting breakpoints by file and "
                          "line is expected to work.");
            case LinkedSymbols:
            case BuildIdSymbols:
                return Tr::tr("This module does not contain debug information "
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
            return Tr::tr("<unknown>", "address");
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

static bool dependsCanBeFound()
{
    static bool dependsInPath = Environment::systemEnvironment().searchInPath("depends").isEmpty();
    return dependsInPath;
}

bool ModulesModel::contextMenuEvent(const ItemViewEvent &ev)
{
    ModuleItem *item = itemForIndexAtLevel<1>(ev.sourceModelIndex());

    const bool enabled = engine->debuggerActionsEnabled();
    const bool canReload = engine->hasCapability(ReloadModuleCapability);
    const bool canLoadSymbols = engine->hasCapability(ReloadModuleSymbolsCapability);
    const bool canShowSymbols = engine->hasCapability(ShowModuleSymbolsCapability);
    const bool moduleNameValid = item && !item->module.moduleName.isEmpty();
    const QString moduleName = item ? item->module.moduleName : QString();
    const FilePath modulePath = item ? item->module.modulePath : FilePath();

    auto menu = new QMenu;

    addAction(this, menu, Tr::tr("Update Module List"),
              enabled && canReload,
              [this] { engine->reloadModules(); });

    addAction(this, menu, Tr::tr("Show Source Files for Module \"%1\"").arg(moduleName),
              Tr::tr("Show Source Files for Module"),
              moduleNameValid && enabled && canReload,
              [this, modulePath] { engine->loadSymbols(modulePath); });

    addAction(this, menu, Tr::tr("Show Dependencies of \"%1\"").arg(moduleName),
              Tr::tr("Show Dependencies"),
              moduleNameValid && !modulePath.needsDevice() && modulePath.exists()
                  && dependsCanBeFound(),
              [modulePath] {
                  Process::startDetached({{"depends"}, {modulePath.toString()}});
              });

    addAction(this, menu, Tr::tr("Load Symbols for All Modules"),
              enabled && canLoadSymbols,
              [this] { engine->loadAllSymbols(); });

    addAction(this, menu, Tr::tr("Examine All Modules"),
              enabled && canLoadSymbols,
              [this] { engine->examineModules(); });

    addAction(this, menu, Tr::tr("Load Symbols for Module \"%1\"").arg(moduleName),
              Tr::tr("Load Symbols for Module"),
              moduleNameValid && canLoadSymbols,
              [this, modulePath] { engine->loadSymbols(modulePath); });

    addAction(this, menu, Tr::tr("Edit File \"%1\"").arg(moduleName),
              Tr::tr("Edit File"),
              moduleNameValid,
              [this, modulePath] { engine->gotoLocation(modulePath); });

    addAction(this, menu, Tr::tr("Show Symbols in File \"%1\"").arg(moduleName),
              Tr::tr("Show Symbols"),
              canShowSymbols && moduleNameValid,
              [this, modulePath] { engine->requestModuleSymbols(modulePath); });

    addAction(this, menu, Tr::tr("Show Sections in File \"%1\"").arg(moduleName),
              Tr::tr("Show Sections"),
              canShowSymbols && moduleNameValid,
              [this, modulePath] { engine->requestModuleSections(modulePath); });

    menu->addAction(settings().settingsDialog.action());

    connect(menu, &QMenu::aboutToHide, menu, &QObject::deleteLater);
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
        Tr::tr("Module Name") + pad,
        Tr::tr("Module Path") + pad,
        Tr::tr("Symbols Read") + pad,
        Tr::tr("Symbols Type") + pad,
        Tr::tr("Start Address") + pad,
        Tr::tr("End Address") + pad}));

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setObjectName("ModulesProxyModel");
    m_proxyModel->setSourceModel(m_model);
}

ModulesHandler::~ModulesHandler()
{
    delete m_model;
}

QAbstractItemModel *ModulesHandler::model() const
{
    return m_proxyModel;
}

ModuleItem *ModulesHandler::moduleFromPath(const FilePath &modulePath) const
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

const Modules ModulesHandler::modules() const
{
    Modules mods;
    m_model->forItemsAtLevel<1>([&mods](ModuleItem *item) { mods.append(item->module); });
    return mods;
}

void ModulesHandler::removeModule(const FilePath &modulePath)
{
    if (ModuleItem *item = moduleFromPath(modulePath))
        m_model->destroyItem(item);
}

void ModulesHandler::updateModule(const Module &module)
{
    const FilePath path = module.modulePath;
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
                 Q_FUNC_INFO, qPrintable(module.modulePath.toUserOutput()));
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
    for (TreeItem *item : toDestroy)
        m_model->destroyItem(item);
}

} // namespace Debugger::Internal

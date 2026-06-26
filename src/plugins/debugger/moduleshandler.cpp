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
#include <utils/listmodel.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>
#include <QMenu>
#include <QSortFilterProxyModel>

#include <functional>

using namespace Utils;

namespace Debugger::Internal {

//////////////////////////////////////////////////////////////////
//
// ModulesModel
//
//////////////////////////////////////////////////////////////////

class ModulesModel : public ListModel<Module>
{
public:
    QVariant itemData(const Module &module, int column, int role) const final;

    bool setData(const QModelIndex &idx, const QVariant &data, int role) override
    {
        if (role == BaseTreeView::ItemViewEventRole) {
            ItemViewEvent ev = data.value<ItemViewEvent>();
            if (ev.type() == QEvent::ContextMenu)
                return contextMenuEvent(ev);
        }

        return ListModel::setData(idx, data, role);
    }

    bool contextMenuEvent(const ItemViewEvent &ev);

    DebuggerEngine *engine;
};

QVariant ModulesModel::itemData(const Module &module, int column, int role) const
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
            case NoSymbols:
                return Tr::tr("None", "Symbols Type (No debug information found)");
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

static bool dependsCanBeFound()
{
    static bool dependsInPath = Environment::systemEnvironment().searchInPath("depends").isEmpty();
    return dependsInPath;
}

bool ModulesModel::contextMenuEvent(const ItemViewEvent &ev)
{
    const Module module = dataAt(ev.sourceModelIndex().row());

    const bool enabled = engine->debuggerActionsEnabled();
    const bool canReload = engine->hasCapability(ReloadModuleCapability);
    const bool canLoadSymbols = engine->hasCapability(ReloadModuleSymbolsCapability);
    const bool canShowSymbols = engine->hasCapability(ShowModuleSymbolsCapability);
    const bool moduleNameValid = !module.moduleName.isEmpty();
    const QString moduleName = module.moduleName;
    const FilePath modulePath = module.modulePath;

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
              moduleNameValid && modulePath.isLocal() && modulePath.exists()
                  && dependsCanBeFound(),
              [modulePath] {
                  Process::startDetached({{"depends"}, {modulePath.nativePath()}});
              });

    addAction(this, menu, Tr::tr("Load Symbols for All Modules"),
              enabled && canLoadSymbols,
              [this] { engine->loadAllSymbols(); });

    addAction(this, menu, Tr::tr("Examine All Modules"),
              enabled && canLoadSymbols && engine->isExamineModulesEnabled(), [this] {
        ModulesHandler *handler = engine->modulesHandler();
        for (const Module &module : handler->modules()) {
            if (module.elfData.symbolsType == UnknownSymbols)
                handler->updateModule(module);
        }
    });

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

    addStandardActions(ev.view(), menu);

    menu->setAttribute(Qt::WA_DeleteOnClose);
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

void ModulesHandler::removeAll()
{
    m_model->clear();
}

const Modules ModulesHandler::modules() const
{
    return m_model->allData();
}

void ModulesHandler::removeModule(const FilePath &modulePath)
{
    m_model->destroyItems([&modulePath](const Module &module) {
        return module.modulePath == modulePath;
    });
}

static FilePath pickPath(const FilePath &hostPath, const FilePath &modulePath)
{
    if (!hostPath.isEmpty() && hostPath.exists())
        return hostPath;
    return modulePath; // Checking if this exists can be slow, delay it for as long as possible
}

void ModulesHandler::updateModule(const Module &module)
{
    const FilePath path = pickPath(module.hostPath, module.modulePath);
    if (path.isEmpty())
        return;

    // Recent modules are more likely to be unloaded first.
    auto item = m_model->findItemByData([&path](const Module &module) {
        return module.modulePath == path;
    });
    if (item)
        item->itemData = module;
    else
        item = m_model->appendItem(module);

    if (path.isLocal()) {
        try { // MinGW occasionallly throws std::bad_alloc.
            ElfReader reader(path);
            item->itemData.elfData = reader.readHeaders();
            item->update();
        } catch(...) {
            qWarning("%s: An exception occurred while reading module '%s'",
                     Q_FUNC_INFO, qPrintable(module.modulePath.toUserOutput()));
        }
    } else {
        m_model->engine->showMessage(
                    QString("Skipping elf-reading of remote path %1").arg(path.toUserOutput()));
    }
    item->itemData.updated = true;
}

void ModulesHandler::beginUpdateAll()
{
    m_model->forAllData([](Module &module) { module.updated = false; });
}

void ModulesHandler::endUpdateAll()
{
    m_model->destroyItems([](const Module &module) { return !module.updated; });
}

} // namespace Debugger::Internal

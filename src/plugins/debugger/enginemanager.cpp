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

#include "enginemanager.h"

#include "analyzer/analyzermanager.h"
#include "debuggeractions.h"
#include "debuggerinternalconstants.h"
#include "debuggericons.h"
#include "debuggercore.h"
#include "debuggerruncontrol.h"
#include "stackhandler.h"

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <utils/basetreeview.h>
#include <utils/treemodel.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QMenu>
#include <QTimer>

using namespace Core;
using namespace Utils;

namespace Debugger {
namespace Internal {

const bool hideSwitcherUnlessNeeded = false;

#if 0
SnapshotData::SnapshotData()
{}

void SnapshotData::clear()
{
    m_frames.clear();
    m_location.clear();
    m_date = QDateTime();
}

QString SnapshotData::function() const
{
    if (m_frames.isEmpty())
        return QString();
    const StackFrame &frame = m_frames.at(0);
    return frame.function + QLatin1Char(':') + QString::number(frame.line);
}

QString SnapshotData::toString() const
{
    QString res;
    QTextStream str(&res);
/*    str << SnapshotHandler::tr("Function:") << ' ' << function() << ' '
        << SnapshotHandler::tr("File:") << ' ' << m_location << ' '
        << SnapshotHandler::tr("Date:") << ' ' << m_date.toString(); */
    return res;
}

QString SnapshotData::toToolTip() const
{
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>"
/*
        << "<tr><td>" << SnapshotHandler::tr("Function:")
            << "</td><td>" << function() << "</td></tr>"
        << "<tr><td>" << SnapshotHandler::tr("File:")
            << "</td><td>" << QDir::toNativeSeparators(m_location) << "</td></tr>"
        << "</table></body></html>"; */
    return res;
}

QDebug operator<<(QDebug d, const  SnapshotData &f)
{
    QString res;
    QTextStream str(&res);
    str << f.location();
/*
    str << "level=" << f.level << " address=" << f.address;
    if (!f.function.isEmpty())
        str << ' ' << f.function;
    if (!f.location.isEmpty())
        str << ' ' << f.location << ':' << f.line;
    if (!f.from.isEmpty())
        str << " from=" << f.from;
    if (!f.to.isEmpty())
        str << " to=" << f.to;
*/
    d.nospace() << res;
    return d;
}
#endif

class EngineItem : public QObject, public TreeItem
{
public:
    QVariant data(int column, int role) const final;
    bool setData(int row, const QVariant &data, int role) final;

    const bool m_isPreset = false;
    QPointer<DebuggerEngine> m_engine;
};

class EngineManagerPrivate : public QObject
{
public:
    EngineManagerPrivate()
    {
        m_engineModel.setHeader({EngineManager::tr("Name"), EngineManager::tr("File")});
        // The preset case:
        auto preset = new EngineItem;
        m_engineModel.rootItem()->appendChild(preset);
        m_currentItem = preset;

        m_engineChooser = new QComboBox;
        m_engineChooser->setModel(&m_engineModel);
        m_engineChooser->setIconSize(QSize(0, 0));
        if (hideSwitcherUnlessNeeded)
            m_engineChooser->hide();

        connect(m_engineChooser, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
                this, &EngineManagerPrivate::activateEngineByIndex);
    }

    ~EngineManagerPrivate()
    {
        delete m_engineChooser;
    }

    EngineItem *findEngineItem(DebuggerEngine *engine);
    void activateEngine(DebuggerEngine *engine);
    void activateEngineItem(EngineItem *engineItem);
    void activateEngineByIndex(int index);
    void selectUiForCurrentEngine();
    void updateEngineChooserVisibility();

    TreeModel<TypedTreeItem<EngineItem>, EngineItem> m_engineModel;
    QPointer<EngineItem> m_currentItem;
    Core::Id m_previousMode;
    QPointer<QComboBox> m_engineChooser;
};

////////////////////////////////////////////////////////////////////////
//
// EngineManager
//
////////////////////////////////////////////////////////////////////////

/*!
    \class Debugger::Internal::EngineManager
    \brief The EngineManager manages running debugger engines.
*/

static EngineManager *theEngineManager = nullptr;
static EngineManagerPrivate *d = nullptr;

EngineManager::EngineManager()
{
    theEngineManager = this;
    d = new EngineManagerPrivate;
}

QWidget *EngineManager::engineChooser()
{
    return d->m_engineChooser;
}

EngineManager::~EngineManager()
{
    theEngineManager = nullptr;
    delete d;
}

EngineManager *EngineManager::instance()
{
    return theEngineManager;
}

QAbstractItemModel *EngineManager::model()
{
    return &d->m_engineModel;
}

void EngineManager::activateEngine(DebuggerEngine *engine)
{
    d->activateEngine(engine);
}

QVariant EngineItem::data(int column, int role) const
{
    if (m_engine) {
        if (role == SnapshotCapabilityRole)
            return m_engine->hasCapability(SnapshotCapability);

        const DebuggerRunParameters &rp = m_engine->runParameters();

        switch (role) {
        case Qt::DisplayRole:
            switch (column) {
            case 0: {
                QString myName = m_engine->displayName();
                int count = 0;
                for (TreeItem *item : *TreeItem::parent()) {
                    DebuggerEngine *engine = static_cast<EngineItem *>(item)->m_engine;
                    count += engine && engine->displayName() == myName;
                }
                if (count > 1)
                    myName += QString(" (%1)").arg(m_engine->runId());

                return myName;
            }
            case 1:
                return rp.coreFile.isEmpty() ? rp.inferior.executable : rp.coreFile;
            }
            return QVariant();

        case Qt::ToolTipRole:
            return QVariant();

        case Qt::DecorationRole:
            // Return icon that indicates whether this is the active engine
            if (column == 0)
                return d->m_currentItem == this ? Icons::LOCATION.icon() : Icons::EMPTY.icon();

        default:
            break;
        }
    } else {
        switch (role) {
        case Qt::DisplayRole:
            if (column == 0)
                return EngineManager::tr("Debugger Preset");
            return QString("-");
        default:
            break;
        }
    }
    return QVariant();
}

bool EngineItem::setData(int row, const QVariant &value, int role)
{
    Q_UNUSED(row);
    if (!m_engine)
        return false;

    if (role == BaseTreeView::ItemActivatedRole) {
        EngineItem *engineItem = d->findEngineItem(m_engine);
        d->activateEngineItem(engineItem);
        return true;
    }

    if (role == BaseTreeView::ItemViewEventRole) {
        ItemViewEvent ev = value.value<ItemViewEvent>();

        if (auto cmev = ev.as<QContextMenuEvent>()) {

            auto menu = new QMenu(ev.view());

            QAction *actCreate = menu->addAction(EngineManager::tr("Create Snapshot"));
            actCreate->setEnabled(m_engine->hasCapability(SnapshotCapabilityRole));
            menu->addSeparator();

            QAction *actRemove = menu->addAction(EngineManager::tr("Abort Debugger"));
            actRemove->setEnabled(true);

            QAction *act = menu->exec(cmev->globalPos());

            if (act == actCreate && m_engine)
                m_engine->createSnapshot();
            else if (act == actRemove && m_engine)
                m_engine->quitDebugger();

            return true;
        }

        if (auto kev = ev.as<QKeyEvent>(QEvent::KeyPress)) {
            if (kev->key() == Qt::Key_Delete && m_engine) {
                m_engine->quitDebugger();
            } else if (kev->key() == Qt::Key_Return || kev->key() == Qt::Key_Enter) {
                d->activateEngineByIndex(row);
            }
            return true;
        }
    }

    return false;
}

void EngineManagerPrivate::activateEngineByIndex(int index)
{
    activateEngineItem(m_engineModel.rootItem()->childAt(index));
}

void EngineManagerPrivate::activateEngineItem(EngineItem *engineItem)
{
    Context previousContext;
    if (m_currentItem) {
        if (DebuggerEngine *engine = m_currentItem->m_engine) {
            previousContext.add(engine->languageContext());
            previousContext.add(engine->debuggerContext());
        } else {
            previousContext.add(Context(Constants::C_DEBUGGER_NOTRUNNING));
        }
    }

    m_currentItem = engineItem;

    Context newContext;
    if (m_currentItem) {
        if (DebuggerEngine *engine = m_currentItem->m_engine) {
            newContext.add(engine->languageContext());
            newContext.add(engine->debuggerContext());
        } else {
            newContext.add(Context(Constants::C_DEBUGGER_NOTRUNNING));
        }
    }

    ICore::updateAdditionalContexts(previousContext, newContext);
    selectUiForCurrentEngine();
}

void EngineManagerPrivate::selectUiForCurrentEngine()
{
    if (ModeManager::currentModeId() != Constants::MODE_DEBUG)
        return;

    Perspective *perspective = nullptr;
    int row = 0;

    if (m_currentItem && m_currentItem->m_engine)
        perspective = m_currentItem->m_engine->perspective();

    if (m_currentItem)
        row = m_engineModel.rootItem()->indexOf(m_currentItem);

    m_engineChooser->setCurrentIndex(row);
    const int contentWidth = m_engineChooser->fontMetrics().width(m_engineChooser->currentText() + "xx");
    QStyleOptionComboBox option;
    option.initFrom(m_engineChooser);
    const QSize sz(contentWidth, 1);
    const int width = m_engineChooser->style()->sizeFromContents(
                QStyle::CT_ComboBox, &option, sz).width();
    m_engineChooser->setFixedWidth(width);

    if (!perspective)
        perspective = Perspective::findPerspective(Debugger::Constants::PRESET_PERSPECTIVE_ID);

    QTC_ASSERT(perspective, return);
    perspective->select();

    m_engineModel.rootItem()->forFirstLevelChildren([this](EngineItem *engineItem) {
        if (engineItem && engineItem->m_engine)
            engineItem->m_engine->updateUi(engineItem == m_currentItem);
    });

    emit theEngineManager->currentEngineChanged();
}

EngineItem *EngineManagerPrivate::findEngineItem(DebuggerEngine *engine)
{
    return m_engineModel.rootItem()->findFirstLevelChild([engine](EngineItem *engineItem) {
        return engineItem->m_engine == engine;
    });
}

void EngineManagerPrivate::activateEngine(DebuggerEngine *engine)
{
    EngineItem *engineItem = findEngineItem(engine);
    activateEngineItem(engineItem);
}

void EngineManagerPrivate::updateEngineChooserVisibility()
{
    // Show it if there's more than one option (i.e. not the preset engine only)
    if (hideSwitcherUnlessNeeded) {
        const int count = m_engineModel.rootItem()->childCount();
        m_engineChooser->setVisible(count >= 2);
    }
}

void EngineManager::registerEngine(DebuggerEngine *engine)
{
    auto engineItem = new EngineItem;
    engineItem->m_engine = engine;
    d->m_engineModel.rootItem()->appendChild(engineItem);
    d->updateEngineChooserVisibility();
}

void EngineManager::activateDebugMode()
{
    if (ModeManager::currentModeId() != Constants::MODE_DEBUG) {
        d->m_previousMode = ModeManager::currentModeId();
        ModeManager::activateMode(Constants::MODE_DEBUG);
    }
}

void EngineManager::deactivateDebugMode()
{
    if (ModeManager::currentModeId() == Constants::MODE_DEBUG && d->m_previousMode.isValid()) {
        // If stopping the application also makes Qt Creator active (as the
        // "previously active application"), doing the switch synchronously
        // leads to funny effects with floating dock widgets
        const Core::Id mode = d->m_previousMode;
        QTimer::singleShot(0, d, [mode]() { ModeManager::activateMode(mode); });
        d->m_previousMode = Id();
    }
}

bool EngineManager::isLastOf(const QString &type)
{
    int count = 0;
    d->m_engineModel.rootItem()->forFirstLevelChildren([&](EngineItem *engineItem) {
        if (engineItem && engineItem->m_engine)
            count += (engineItem->m_engine->debuggerName() == type);
    });
    return count == 1;
}

void EngineManager::unregisterEngine(DebuggerEngine *engine)
{
    if (ModeManager::currentModeId() == Constants::MODE_DEBUG) {
        if (Perspective *parent = Perspective::findPerspective(Constants::PRESET_PERSPECTIVE_ID))
            parent->select();
    }

    d->activateEngineItem(d->m_engineModel.rootItem()->childAt(0)); // Preset.

    // Could be that the run controls died before it was appended.
    if (auto engineItem = d->findEngineItem(engine))
        d->m_engineModel.destroyItem(engineItem);

    d->updateEngineChooserVisibility();
    emit theEngineManager->currentEngineChanged();
}

QList<QPointer<DebuggerEngine>> EngineManager::engines()
{
    QList<QPointer<DebuggerEngine>> result;
    d->m_engineModel.forItemsAtLevel<1>([&result](EngineItem *engineItem) {
        if (DebuggerEngine *engine = engineItem->m_engine)
            result.append(engine);
    });
    return result;
}

QPointer<DebuggerEngine> EngineManager::currentEngine()
{
    return d->m_currentItem ? d->m_currentItem->m_engine : nullptr;
}

} // namespace Internal
} // namespace Debugger

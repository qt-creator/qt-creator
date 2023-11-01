// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "enginemanager.h"

#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggericons.h"
#include "debuggermainwindow.h"
#include "debuggertr.h"

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <utils/basetreeview.h>
#include <utils/treemodel.h>
#include <utils/qtcassert.h>

#include <QAbstractProxyModel>
#include <QComboBox>
#include <QDebug>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTimer>

using namespace Core;
using namespace Utils;

namespace Debugger::Internal {

const bool hideSwitcherUnlessNeeded = false;
const char INDEX_ID[] = "Debugger/Debugger.SelectedEngineIndex";

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
    return frame.function + ':' + QString::number(frame.line);
}

QString SnapshotData::toString() const
{
    QString res;
    QTextStream str(&res);
/*    str << Tr::tr("Function:") << ' ' << function() << ' '
        << Tr::tr("File:") << ' ' << m_location << ' '
        << Tr::tr("Date:") << ' ' << m_date.toString(); */
    return res;
}

QString SnapshotData::toToolTip() const
{
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>"
/*
        << "<tr><td>" << Tr::tr("Function:")
            << "</td><td>" << function() << "</td></tr>"
        << "<tr><td>" << Tr::tr("File:")
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

class EngineTypeFilterProxyModel : public QSortFilterProxyModel
{
public:
    explicit EngineTypeFilterProxyModel(const QString &type, QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
        , m_type(type)
    {
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        if (index.isValid()) {
            QVariant data = sourceModel()->data(index, Qt::UserRole);
            if (data.isValid() && data.toString() == m_type) {
                return true; // Display only DapEngines
            }
        }
        return false;
    }
private:
    QString m_type;
};

class ModelChooser : public QObject
{
    Q_OBJECT
public:
    ModelChooser(QAbstractItemModel *sourceModel,
                 const QString &engineType,
                 QObject *parent = nullptr)
        : QObject(parent)
        , m_engineChooser(new QComboBox())
        , m_proxyModel(new EngineTypeFilterProxyModel(engineType))
        , m_sourceModel(sourceModel)
        , m_enginType(engineType)
        , m_key(engineType.isEmpty() ? Utils::Key(INDEX_ID) + "." + engineType.toUtf8()
                                     : Utils::Key(INDEX_ID))
    {
        m_proxyModel->setSourceModel(sourceModel);

        m_engineChooser->setModel(m_proxyModel);
        m_engineChooser->setIconSize(QSize(0, 0));
        if (hideSwitcherUnlessNeeded)
            m_engineChooser->hide();

        connect(m_engineChooser, &QComboBox::activated, this, [this](int index) {
            QModelIndex sourceIndex = m_proxyModel->mapToSource(m_proxyModel->index(index, 0));
            emit activated(sourceIndex.row());
            m_lastActivatedIndex = sourceIndex.row();
            ICore::settings()->setValue(m_key, m_lastActivatedIndex);
        });

        connect(m_proxyModel, &QAbstractItemModel::rowsRemoved, this, [this] {
            setCurrentIndex(m_lastActivatedIndex);
        });
    }

    ~ModelChooser()
    {
        delete m_engineChooser;
        delete m_proxyModel;
    }

    QComboBox *comboBox() const { return m_engineChooser; }
    QAbstractItemModel *model() const { return m_proxyModel; }
    const QString &engineType() const { return m_enginType; }

    void restoreIndex()
    {
        m_lastActivatedIndex = ICore::settings()->value(m_key, 0).toInt();
        if (m_lastActivatedIndex <= m_engineChooser->count())
            setCurrentIndex(m_lastActivatedIndex);
    }

    void setCurrentIndex(int index)
    {
        const QModelIndex sourceIndex = m_proxyModel->mapFromSource(m_sourceModel->index(index, 0));
        if (sourceIndex.isValid())
            m_engineChooser->setCurrentIndex(sourceIndex.row());
        else
            m_engineChooser->setCurrentIndex(0);
    }

    void adjustUiForEngine(int row)
    {
        setCurrentIndex(row);

        const int contentWidth = m_engineChooser->fontMetrics().horizontalAdvance(
            m_engineChooser->currentText() + "xx");
        QStyleOptionComboBox option;
        option.initFrom(m_engineChooser);
        const QSize sz(contentWidth, 1);
        const int width = m_engineChooser->style()
                              ->sizeFromContents(QStyle::CT_ComboBox, &option, sz)
                              .width();
        m_engineChooser->setFixedWidth(width);
    }

signals:
    void activated(int index);

private:
    QPointer<QComboBox> m_engineChooser;
    QPointer<EngineTypeFilterProxyModel> m_proxyModel;
    QAbstractItemModel *m_sourceModel;
    QString m_enginType;
    const Utils::Key m_key;
    int m_lastActivatedIndex = -1;
};

struct PerspectiveItem
{
    QString name;
    QString type;
    QString id;
};

class EngineItem : public QObject, public TreeItem
{
public:
    QVariant data(int column, int role) const final;
    bool setData(int row, const QVariant &data, int role) final;

    const bool m_isPreset = false;
    QPointer<DebuggerEngine> m_engine;

    PerspectiveItem m_perspective;
};

class EngineManagerPrivate : public QObject
{
public:
    EngineManagerPrivate()
    {
        m_engineModel.setHeader({Tr::tr("Perspective"), Tr::tr("Debugged Application")});

        m_engineChooser = new ModelChooser(&m_engineModel, "", this);
        m_engineDAPChooser = new ModelChooser(&m_engineModel, "DAP", this);

        connect(m_engineChooser, &ModelChooser::activated, this, [this](int index) {
            activateEngineByIndex(index);
        });

        connect(m_engineDAPChooser, &ModelChooser::activated, this, [this](int index) {
            activateEngineByIndex(index);
        });
    }

    ~EngineManagerPrivate() = default;

    EngineItem *findEngineItem(DebuggerEngine *engine);
    void activateEngineItem(EngineItem *engineItem);
    void activateEngineByIndex(int index);
    void selectUiForCurrentEngine();
    void updateEngineChooserVisibility();
    void updatePerspectives();

    TreeModel<TypedTreeItem<EngineItem>, EngineItem> m_engineModel;
    QPointer<EngineItem> m_currentItem; // The primary information is DebuggerMainWindow::d->m_currentPerspective
    Utils::Id m_previousMode;

    QPointer<ModelChooser> m_engineChooser;
    QPointer<ModelChooser> m_engineDAPChooser;

    QList<PerspectiveItem> m_perspectives;
    bool m_shuttingDown = false;

    // This contains the contexts that need to be removed when switching
    // away from the current engine item. Since the plugin itself adds
    // C_DEBUGGER_NOTRUNNING on initialization this is set here as well,
    // so it can be removed when switching away from the initial (null)
    // engine. See QTCREATORBUG-22330.
    Context m_currentAdditionalContext{Constants::C_DEBUGGER_NOTRUNNING};
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
    return d->m_engineChooser->comboBox();
}

QWidget *EngineManager::dapEngineChooser()
{
    return d->m_engineDAPChooser->comboBox();
}

void EngineManager::updatePerspectives()
{
    d->updatePerspectives();
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
    return d->m_engineChooser->model();
}

QAbstractItemModel *EngineManager::dapModel()
{
    return d->m_engineDAPChooser->model();
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
                return (rp.coreFile.isEmpty() ? rp.inferior.command.executable() : rp.coreFile).toUserOutput();
            }
            return QVariant();

        case Qt::ToolTipRole:
            return QVariant();

        case Qt::DecorationRole:
            // Return icon that indicates whether this is the active engine
            if (column == 0)
                return d->m_currentItem == this ? Icons::LOCATION.icon() : Icons::EMPTY.icon();
            break;
        case Qt::UserRole:
            return QVariant::fromValue(m_engine->debuggerType());
        default:
            break;
        }
    } else {
        switch (role) {
        case Qt::DisplayRole:
            if (column == 0)
                return m_perspective.name;
            return QString("-");
        case Qt::UserRole:
            return m_perspective.type;
        default:
            break;
        }
    }
    return QVariant();
}

bool EngineItem::setData(int row, const QVariant &value, int role)
{
    Q_UNUSED(row)
    if (!m_engine)
        return false;

    if (role == BaseTreeView::ItemActivatedRole) {
        EngineItem *engineItem = d->findEngineItem(m_engine);
        d->activateEngineByIndex(engineItem->indexInParent());
        return true;
    }

    if (role == BaseTreeView::ItemViewEventRole) {
        ItemViewEvent ev = value.value<ItemViewEvent>();

        if (auto cmev = ev.as<QContextMenuEvent>()) {

            auto menu = new QMenu(ev.view());

            QAction *actCreate = menu->addAction(Tr::tr("Create Snapshot"));
            actCreate->setEnabled(m_engine->hasCapability(SnapshotCapabilityRole));
            menu->addSeparator();

            QAction *actRemove = menu->addAction(Tr::tr("Abort Debugger"));
            actRemove->setEnabled(true);

            QAction *act = menu->exec(cmev->globalPos());

            if (act == actCreate && m_engine)
                m_engine->createSnapshot();
            else if (act == actRemove && m_engine)
                m_engine->quitDebugger();

            return true;
        }

        if (auto kev = ev.as<QKeyEvent>(QEvent::KeyPress)) {
            if ((kev->key() == Qt::Key_Delete || kev->key() == Qt::Key_Backspace) && m_engine) {
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
    // The actual activation is triggered indirectly via the perspective change.
    Perspective *perspective = nullptr;
    EngineItem *engineItem = m_engineModel.rootItem()->childAt(index);

    if (engineItem && !engineItem->m_engine) {
        perspective = Perspective::findPerspective(engineItem->m_perspective.id);
    } else {
        QTC_ASSERT(engineItem, return);
        QTC_ASSERT(engineItem->m_engine, return);
        perspective = engineItem->m_engine->perspective();
    }

    QTC_ASSERT(perspective, return);
    perspective->select();
}

void EngineManagerPrivate::activateEngineItem(EngineItem *engineItem)
{
    if (m_currentItem == engineItem)
        return;

    QTC_ASSERT(engineItem, return);
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

    ICore::updateAdditionalContexts(m_currentAdditionalContext, newContext);
    m_currentAdditionalContext = newContext;

    // In case this was triggered externally by some Perspective::select() call.
    const int idx = engineItem->indexInParent();

    if ((engineItem->m_engine
         && engineItem->m_engine->debuggerType() == m_engineDAPChooser->engineType())
        || (engineItem->m_engine
            && engineItem->m_perspective.type == m_engineDAPChooser->engineType()))
        m_engineDAPChooser->setCurrentIndex(idx);
    else
        m_engineChooser->setCurrentIndex(idx);

    selectUiForCurrentEngine();
}

void EngineManagerPrivate::selectUiForCurrentEngine()
{
    if (ModeManager::currentModeId() != Constants::MODE_DEBUG)
        return;

    int row = 0;
    if (m_currentItem)
        row = m_engineModel.rootItem()->indexOf(m_currentItem);

    if ((m_currentItem->m_engine
         && m_currentItem->m_engine->debuggerType() == m_engineDAPChooser->engineType())
        || (m_currentItem->m_engine
            && m_currentItem->m_perspective.type == m_engineDAPChooser->engineType()))
        m_engineDAPChooser->adjustUiForEngine(row);
    else
        m_engineChooser->adjustUiForEngine(row);

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

void EngineManagerPrivate::updateEngineChooserVisibility()
{
    // Show it if there's more than one option (i.e. not the preset engine only)
    if (hideSwitcherUnlessNeeded) {
        const int count = m_engineModel.rootItem()->childCount();
        m_engineChooser->comboBox()->setVisible(count >= 2);
        m_engineDAPChooser->comboBox()->setVisible(count >= 2);
    }
}

void EngineManagerPrivate::updatePerspectives()
{
    d->updateEngineChooserVisibility();

    Perspective *current = DebuggerMainWindow::currentPerspective();
    if (!current) {
        return;
    }

    m_engineModel.rootItem()->forFirstLevelChildren([this, current](EngineItem *engineItem) {
        if (engineItem == m_currentItem)
            return;

        bool shouldBeActive = false;
        if (engineItem->m_engine) {
            // Normal engine.
            shouldBeActive = engineItem->m_engine->perspective()->isCurrent();
        } else {
            // Preset.
            shouldBeActive = current->id() == Debugger::Constants::PRESET_PERSPECTIVE_ID;
        }

        if (shouldBeActive && engineItem != m_currentItem)
            activateEngineItem(engineItem);
    });
}

QString EngineManager::registerEngine(DebuggerEngine *engine)
{
    auto engineItem = new EngineItem;
    engineItem->m_engine = engine;
    d->m_engineModel.rootItem()->appendChild(engineItem);
    d->updateEngineChooserVisibility();
    return QString::number(d->m_engineModel.rootItem()->childCount());
}

void EngineManager::unregisterEngine(DebuggerEngine *engine)
{
    EngineItem *engineItem = d->findEngineItem(engine);
    QTC_ASSERT(engineItem, return);
    d->m_engineModel.destroyItem(engineItem);
    d->updateEngineChooserVisibility();
}

QString EngineManager::registerDefaultPerspective(const QString &name,
                                                  const QString &type,
                                                  const QString &id)
{
    auto engineItem = new EngineItem;
    engineItem->m_perspective.name = name;
    engineItem->m_perspective.type = type;
    engineItem->m_perspective.id = id;
    d->m_engineModel.rootItem()->appendChild(engineItem);
    d->m_engineDAPChooser->restoreIndex();
    d->m_engineChooser->restoreIndex();
    return QString::number(d->m_engineModel.rootItem()->childCount());
}

void EngineManager::activateDebugMode()
{
    if (ModeManager::currentModeId() != Constants::MODE_DEBUG) {
        d->m_previousMode = ModeManager::currentModeId();
        ModeManager::activateMode(Constants::MODE_DEBUG);
    }
}

void EngineManager::activateByIndex(int index)
{
    d->activateEngineByIndex(index);
}

void EngineManager::deactivateDebugMode()
{
    if (ModeManager::currentModeId() == Constants::MODE_DEBUG && d->m_previousMode.isValid()) {
        // If stopping the application also makes Qt Creator active (as the
        // "previously active application"), doing the switch synchronously
        // leads to funny effects with floating dock widgets
        const Utils::Id mode = d->m_previousMode;
        QTimer::singleShot(0, d, [mode] { ModeManager::activateMode(mode); });
        d->m_previousMode = Id();
    }
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

bool EngineManager::shutDown()
{
    d->m_shuttingDown = true;
    bool anyEngineAborting = false;
    for (DebuggerEngine *engine : EngineManager::engines()) {
        if (engine && engine->state() != Debugger::DebuggerNotReady) {
            engine->abortDebugger();
            anyEngineAborting = true;
        }
    }
    return anyEngineAborting;
}

} // Debugger::Internal

#include "enginemanager.moc"

// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "dockmanager.h"

#include "ads_globals.h"
#include "ads_globals_p.h"
#include "advanceddockingsystemtr.h"
#include "autohidedockcontainer.h"
#include "dockareawidget.h"
#include "dockfocuscontroller.h"
#include "dockingstatereader.h"
#include "dockoverlay.h"
#include "docksplitter.h"
#include "dockwidget.h"
#include "floatingdockcontainer.h"
#include "iconprovider.h"

#include "workspacedialog.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>

#include <algorithm>
#include <iostream>

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QLoggingCategory>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QSettings>
#include <QVariant>
#include <QWindow>
#include <QXmlStreamWriter>

#if !(defined(Q_OS_UNIX) && !defined(Q_OS_MACOS))
#include <QWindowStateChangeEvent>
#endif

Q_LOGGING_CATEGORY(adsLog, "qtc.qmldesigner.advanceddockingsystem", QtWarningMsg);

using namespace Utils;

namespace ADS {
/**
 * Internal file version in case the structure changes internally
 */
enum eStateFileVersion {
    InitialVersion = 0,       //!< InitialVersion
    Version1 = 1,             //!< Version1
    CurrentVersion = Version1 //!< CurrentVersion
};

static DockManager::ConfigFlags g_staticConfigFlags = DockManager::DefaultNonOpaqueConfig;
static DockManager::AutoHideFlags g_staticAutoHideConfigFlags; // auto hide is disabled by default

static QString g_floatingContainersTitle;

/**
 * Private data class of DockManager class (pimpl)
 */
class DockManagerPrivate
{
public:
    DockManager *q;
    QList<QPointer<FloatingDockContainer>> m_floatingWidgets;
    QList<QPointer<FloatingDockContainer>> m_hiddenFloatingWidgets;
    QList<DockContainerWidget *> m_containers;
    DockOverlay *m_containerOverlay = nullptr;
    DockOverlay *m_dockAreaOverlay = nullptr;
    QMap<QString, DockWidget *> m_dockWidgetsMap;
    bool m_restoringState = false;
    QVector<FloatingDockContainer *> m_uninitializedFloatingWidgets;
    DockFocusController *m_focusController = nullptr;
    DockWidget *m_centralWidget = nullptr;
    bool m_isLeavingMinimized = false;

    QString m_workspacePresetsPath;
    QList<Workspace> m_workspaces;
    Workspace m_workspace;

    QtcSettings *m_settings = nullptr;
    bool m_modeChangeState = false;
    bool m_workspaceOrderDirty = false;

    /**
     * Private data constructor
     */
    DockManagerPrivate(DockManager *parent);

    /**
     * Restores the state. If testing is set to true it will check if
     * the given data stream is a valid docking system state file.
     */
    bool restoreStateFromXml(const QByteArray &state, int version, bool testing = false);

    /**
     * Restore state
     */
    bool restoreState(const QByteArray &state, int version);

    void restoreDockWidgetsOpenState();
    void restoreDockAreasIndices();
    void emitTopLevelEvents();

    void hideFloatingWidgets()
    {
        // Hide updates of floating widgets from user
        for (const auto &floatingWidget : std::as_const(m_floatingWidgets)) {
            if (floatingWidget)
                floatingWidget->hide();
        }
    }

    void markDockWidgetsDirty()
    {
        for (const auto &dockWidget : std::as_const(m_dockWidgetsMap))
            dockWidget->setProperty(internal::g_dirtyProperty, true);
    }

    /**
     * Restores the container with the given index
     */
    bool restoreContainer(int index, DockingStateReader &stream, bool testing);

    void workspaceLoadingProgress();
}; // class DockManagerPrivate

DockManagerPrivate::DockManagerPrivate(DockManager *parent)
    : q(parent)
{}

bool DockManagerPrivate::restoreContainer(int index, DockingStateReader &stream, bool testing)
{
    if (testing)
        index = 0;

    bool result = false;
    if (index >= m_containers.count()) {
        FloatingDockContainer *floatingWidget = new FloatingDockContainer(q);
        result = floatingWidget->restoreState(stream, testing);
    } else {
        auto container = m_containers[index];
        if (container->isFloating())
            result = container->floatingWidget()->restoreState(stream, testing);
        else
            result = container->restoreState(stream, testing);
    }

    return result;
}

bool DockManagerPrivate::restoreStateFromXml(const QByteArray &state, int version, bool testing)
{
    Q_UNUSED(version) // TODO version is not needed, why is it in here in the first place?

    if (state.isEmpty())
        return false;

    DockingStateReader stateReader(state);
    if (!stateReader.readNextStartElement())
        return false;

    if (stateReader.name() != QLatin1String("QtAdvancedDockingSystem"))
        return false;

    qCInfo(adsLog) << "Version" << stateReader.attributes().value("version");
    bool ok;
    int v = stateReader.attributes().value("version").toInt(&ok);
    if (!ok || v > CurrentVersion)
        return false;

    stateReader.setFileVersion(v);

    qCInfo(adsLog) << "User version" << stateReader.attributes().value("userVersion");
    // Older files do not support userVersion, but we still want to load them so we first test
    // if the attribute exists.
    if (!stateReader.attributes().value("userVersion").isEmpty()) {
        v = stateReader.attributes().value("userVersion").toInt(&ok);
        if (!ok || v != version)
            return false;
    }

    qCInfo(adsLog) << stateReader.attributes().value("containers").toInt();

    if (m_centralWidget) {
        const auto centralWidgetAttribute = stateReader.attributes().value("centralWidget");
        // If we have a central widget, but a state without central widget, then something is wrong.
        if (centralWidgetAttribute.isEmpty()) {
            qWarning()
                << "DockManager has central widget, but saved state does not have central widget.";
            return false;
        }

        // If the object name of the central widget does not match the name of the saved central
        // widget, something is wrong.
        if (m_centralWidget->objectName() != centralWidgetAttribute.toString()) {
            qWarning() << "Object name of central widget does not match name of central widget in "
                          "saved state.";
            return false;
        }
    }

    bool result = true;
    int dockContainerCount = 0;
    while (stateReader.readNextStartElement()) {
        if (stateReader.name() == QLatin1String("container")) {
            result = restoreContainer(dockContainerCount, stateReader, testing);
            if (!result)
                break;

            dockContainerCount++;
        }
    }

    if (!testing) {
        // Delete remaining empty floating widgets
        int floatingWidgetIndex = dockContainerCount - 1;
        for (int i = floatingWidgetIndex; i < m_floatingWidgets.count(); ++i) {
            QPointer<FloatingDockContainer> floatingWidget = m_floatingWidgets[i];
            q->removeDockContainer(floatingWidget->dockContainer());
            floatingWidget->deleteLater();
        }
    }

    return result;
}

void DockManagerPrivate::restoreDockWidgetsOpenState()
{
    // All dock widgets, that have not been processed in the restore state function are
    // invisible to the user now and have no assigned dock area. They do not belong to any dock
    // container, until the user toggles the toggle view action the next time.
    for (auto dockWidget : std::as_const(m_dockWidgetsMap)) {
        if (dockWidget->property(internal::g_dirtyProperty).toBool()) {
            // If the DockWidget is an auto hide widget that is not assigned yet,
            // then we need to delete the auto hide container now
            if (dockWidget->isAutoHide())
                dockWidget->autoHideDockContainer()->cleanupAndDelete();

            dockWidget->flagAsUnassigned();
            emit dockWidget->viewToggled(false);
        } else {
            dockWidget->toggleViewInternal(
                !dockWidget->property(internal::g_closedProperty).toBool());
        }
    }
}

void DockManagerPrivate::restoreDockAreasIndices()
{
    // Now all dock areas are properly restored and we setup the index of the dock areas,
    // because the previous toggleView() action has changed the dock area index.
    for (auto dockContainer : std::as_const(m_containers)) {
        for (int i = 0; i < dockContainer->dockAreaCount(); ++i) {
            DockAreaWidget *dockArea = dockContainer->dockArea(i);
            const QString dockWidgetName = dockArea->property("currentDockWidget").toString();
            DockWidget *dockWidget = nullptr;
            if (!dockWidgetName.isEmpty())
                dockWidget = q->findDockWidget(dockWidgetName);

            if (!dockWidget || dockWidget->isClosed()) {
                int index = dockArea->indexOfFirstOpenDockWidget();
                if (index < 0)
                    continue;

                dockArea->setCurrentIndex(index);
            } else {
                dockArea->internalSetCurrentDockWidget(dockWidget);
            }
        }
    }
}

void DockManagerPrivate::emitTopLevelEvents()
{
    // Finally we need to send the topLevelChanged() signal for all dock widgets if top
    // level changed.
    for (auto dockContainer : std::as_const(m_containers)) {
        DockWidget *topLevelDockWidget = dockContainer->topLevelDockWidget();
        if (topLevelDockWidget) {
            topLevelDockWidget->emitTopLevelChanged(true);
        } else {
            for (int i = 0; i < dockContainer->dockAreaCount(); ++i) {
                auto dockArea = dockContainer->dockArea(i);
                for (auto dockWidget : dockArea->dockWidgets())
                    dockWidget->emitTopLevelChanged(false);
            }
        }
    }
}

bool DockManagerPrivate::restoreState(const QByteArray &state, int version)
{
    QByteArray currentState = state.startsWith("<?xml") ? state : qUncompress(state);
    // Check the format of the given data stream
    if (!restoreStateFromXml(currentState, version, true)) {
        qWarning() << "CheckFormat: Error checking format!";
        return false;
    }

    // Hide updates of floating widgets from user
    hideFloatingWidgets();
    markDockWidgetsDirty();

    if (!restoreStateFromXml(currentState, version)) {
        qWarning() << "RestoreState: Error restoring state!";
        return false;
    }

    restoreDockWidgetsOpenState();
    restoreDockAreasIndices();
    emitTopLevelEvents();
    q->dumpLayout();

    return true;
}

DockManager::DockManager(QWidget *parent)
    : DockContainerWidget(this, parent)
    , d(new DockManagerPrivate(this))
{
    connect(this, &DockManager::workspaceListChanged, this, [=] {
        d->m_workspaceOrderDirty = true;
    });

    createRootSplitter();
    createSideTabBarWidgets();
    QMainWindow *mainWindow = qobject_cast<QMainWindow *>(parent);
    if (mainWindow)
        mainWindow->setCentralWidget(this);

    d->m_dockAreaOverlay = new DockOverlay(this, DockOverlay::ModeDockAreaOverlay);
    d->m_containerOverlay = new DockOverlay(this, DockOverlay::ModeContainerOverlay);
    d->m_containers.append(this);

    if (DockManager::configFlags().testFlag(DockManager::FocusHighlighting))
        d->m_focusController = new DockFocusController(this);

    window()->installEventFilter(this);

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    connect(qApp, &QApplication::focusWindowChanged, this, [](QWindow *focusWindow) {
        // Bring modal dialogs to foreground to ensure that they are in front of any
        // floating dock widget.
        if (focusWindow && focusWindow->isModal())
            focusWindow->raise();
    });
#endif
}

DockManager::~DockManager()
{
    emit aboutToUnloadWorkspace(d->m_workspace.fileName());
    save();
    saveStartupWorkspace();

    // Fix memory leaks, see https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/issues/307
    std::vector<ADS::DockAreaWidget *> areas;
    for (int i = 0; i != dockAreaCount(); ++i)
        areas.push_back(dockArea(i));

    for (auto area : areas) {
        for (auto widget : area->dockWidgets())
            delete widget;

        delete area;
    }

    // Using a temporary vector since the destructor of FloatingDockWidgetContainer
    // alters d->m_floatingWidgets.
    const auto copy = d->m_floatingWidgets;
    for (const auto &floatingWidget : copy) {
        if (floatingWidget)
            delete floatingWidget.get();
    }
    delete d;
}

DockManager::ConfigFlags DockManager::configFlags()
{
    return g_staticConfigFlags;
}

DockManager::AutoHideFlags DockManager::autoHideConfigFlags()
{
    return g_staticAutoHideConfigFlags;
}

void DockManager::setConfigFlags(const ConfigFlags flags)
{
    g_staticConfigFlags = flags;
}

void DockManager::setAutoHideConfigFlags(const AutoHideFlags flags)
{
    g_staticAutoHideConfigFlags = flags;
}

void DockManager::setConfigFlag(eConfigFlag flag, bool on)
{
    internal::setFlag(g_staticConfigFlags, flag, on);
}

void DockManager::setAutoHideConfigFlag(eAutoHideFlag flag, bool on)
{
    internal::setFlag(g_staticAutoHideConfigFlags, flag, on);
}

bool DockManager::testConfigFlag(eConfigFlag flag)
{
    return configFlags().testFlag(flag);
}

bool DockManager::testAutoHideConfigFlag(eAutoHideFlag flag)
{
    return autoHideConfigFlags().testFlag(flag);
}

IconProvider &DockManager::iconProvider()
{
    static IconProvider instance;
    return instance;
}

int DockManager::startDragDistance()
{
    return static_cast<int>(QApplication::startDragDistance() * 1.5);
}

void DockManager::setSettings(QtcSettings *settings)
{
    d->m_settings = settings;
}

void DockManager::setWorkspacePresetsPath(const QString &path)
{
    d->m_workspacePresetsPath = path;
}

void DockManager::initialize()
{
    qCInfo(adsLog) << Q_FUNC_INFO;

    QTC_ASSERT(d->m_settings, return); // Can't continue if settings are not set yet

    syncWorkspacePresets();
    prepareWorkspaces();

    QString workspace = ADS::Constants::DEFAULT_WORKSPACE;

    // Determine workspace to restore at startup
    if (autoRestoreWorkspace()) {
        const QString lastWorkspace = startupWorkspace();
        if (!lastWorkspace.isEmpty()) {
            if (!workspaceExists(lastWorkspace)) {
                // This is a fallback mechanism for pre 4.1 settings which stored the workspace name
                // instead of the file name.
                QString minusVariant = lastWorkspace;
                minusVariant.replace(" ", "-");
                minusVariant.append("." + workspaceFileExtension);

                if (workspaceExists(minusVariant))
                    workspace = minusVariant;

                QString underscoreVariant = lastWorkspace;
                underscoreVariant.replace(" ", "_");
                underscoreVariant.append("." + workspaceFileExtension);

                if (workspaceExists(underscoreVariant))
                    workspace = underscoreVariant;
            } else {
                workspace = lastWorkspace;
            }
        } else
            qWarning() << "Could not restore workspace:" << lastWorkspace;
    }

    openWorkspace(workspace);
}

DockAreaWidget *DockManager::addDockWidget(DockWidgetArea area,
                                           DockWidget *dockWidget,
                                           DockAreaWidget *dockAreaWidget,
                                           int index)
{
    d->m_dockWidgetsMap.insert(dockWidget->objectName(), dockWidget);
    auto container = dockAreaWidget ? dockAreaWidget->dockContainer() : this;
    auto dockArea = container->addDockWidget(area, dockWidget, dockAreaWidget, index);
    emit dockWidgetAdded(dockWidget);
    return dockArea;
}

DockAreaWidget *DockManager::addDockWidgetToContainer(DockWidgetArea area,
                                                      DockWidget *dockWidget,
                                                      DockContainerWidget *dockContainerWidget)
{
    d->m_dockWidgetsMap.insert(dockWidget->objectName(), dockWidget);
    auto dockArea = dockContainerWidget->addDockWidget(area, dockWidget);
    emit dockWidgetAdded(dockWidget);
    return dockArea;
}

AutoHideDockContainer *DockManager::addAutoHideDockWidget(SideBarLocation location,
                                                          DockWidget *dockWidget)
{
    return addAutoHideDockWidgetToContainer(location, dockWidget, this);
}

AutoHideDockContainer *DockManager::addAutoHideDockWidgetToContainer(
    SideBarLocation location, DockWidget *dockWidget, DockContainerWidget *dockContainerWidget)
{
    d->m_dockWidgetsMap.insert(dockWidget->objectName(), dockWidget);
    auto container = dockContainerWidget->createAndSetupAutoHideContainer(location, dockWidget);
    container->collapseView(true);
    emit dockWidgetAdded(dockWidget);
    return container;
}

DockAreaWidget *DockManager::addDockWidgetTab(DockWidgetArea area, DockWidget *dockWidget)
{
    DockAreaWidget *areaWidget = lastAddedDockAreaWidget(area);
    if (areaWidget)
        return addDockWidget(ADS::CenterDockWidgetArea, dockWidget, areaWidget);
    else if (!openedDockAreas().isEmpty())
        return addDockWidget(area, dockWidget, openedDockAreas().constLast()); // TODO
    else
        return addDockWidget(area, dockWidget, nullptr);
}

DockAreaWidget *DockManager::addDockWidgetTabToArea(DockWidget *dockWidget,
                                                    DockAreaWidget *dockAreaWidget,
                                                    int index)
{
    return addDockWidget(ADS::CenterDockWidgetArea, dockWidget, dockAreaWidget, index);
}

FloatingDockContainer *DockManager::addDockWidgetFloating(DockWidget *dockWidget)
{
    d->m_dockWidgetsMap.insert(dockWidget->objectName(), dockWidget);
    DockAreaWidget *oldDockArea = dockWidget->dockAreaWidget();
    if (oldDockArea)
        oldDockArea->removeDockWidget(dockWidget);

    dockWidget->setDockManager(this);
    FloatingDockContainer *floatingWidget = new FloatingDockContainer(dockWidget);
    floatingWidget->resize(dockWidget->size());
    if (isVisible())
        floatingWidget->show();
    else
        d->m_uninitializedFloatingWidgets.append(floatingWidget);

    emit dockWidgetAdded(dockWidget);
    return floatingWidget;
}

DockWidget *DockManager::findDockWidget(const QString &objectName) const
{
    return d->m_dockWidgetsMap.value(objectName, nullptr);
}

void DockManager::removeDockWidget(DockWidget *dockWidget)
{
    emit dockWidgetAboutToBeRemoved(dockWidget);
    d->m_dockWidgetsMap.remove(dockWidget->objectName());
    DockContainerWidget::removeDockWidget(dockWidget);
    dockWidget->setDockManager(nullptr);
    emit dockWidgetRemoved(dockWidget);
}

QMap<QString, DockWidget *> DockManager::dockWidgetsMap() const
{
    return d->m_dockWidgetsMap;
}

const QList<DockContainerWidget *> DockManager::dockContainers() const
{
    return d->m_containers;
}

const QList<QPointer<FloatingDockContainer>> DockManager::floatingWidgets() const
{
    return d->m_floatingWidgets;
}

unsigned int DockManager::zOrderIndex() const
{
    return 0;
}

QByteArray DockManager::saveState(const QString &displayName, int version) const
{
    qCInfo(adsLog) << "Save state" << displayName;

    QByteArray xmlData;
    QXmlStreamWriter stream(&xmlData);
    auto configFlags = DockManager::configFlags();
    stream.setAutoFormatting(configFlags.testFlag(XmlAutoFormattingEnabled));
    stream.setAutoFormattingIndent(workspaceXmlFormattingIndent);
    stream.writeStartDocument();
    stream.writeStartElement("QtAdvancedDockingSystem");
    stream.writeAttribute("version", QString::number(CurrentVersion));
    stream.writeAttribute("userVersion", QString::number(version));
    stream.writeAttribute("containers", QString::number(d->m_containers.count()));
    stream.writeAttribute(workspaceDisplayNameAttribute.toString(), displayName);

    if (d->m_centralWidget)
        stream.writeAttribute("centralWidget", d->m_centralWidget->objectName());

    for (auto container : std::as_const(d->m_containers))
        container->saveState(stream);

    stream.writeEndElement();
    stream.writeEndDocument();
    return xmlData;
}

bool DockManager::restoreState(const QByteArray &state, int version)
{
    // Prevent multiple calls as long as state is not restore. This may happen, if
    // QApplication::processEvents() is called somewhere.
    if (d->m_restoringState)
        return false;

    // We hide the complete dock manager here. Restoring the state means that DockWidgets are
    // removed from the DockArea internal stack layout which in turn  means, that each time a
    // widget is removed the stack will show and raise the next available widget which in turn
    // triggers show events for the dock widgets. To avoid this we hide the dock manager.
    // Because there will be no processing of application events until this function is
    // finished, the user will not see this hiding.
    bool isHidden = this->isHidden();
    if (!isHidden)
        hide();

    d->m_restoringState = true;
    emit restoringState();
    bool result = d->restoreState(state, version);
    d->m_restoringState = false;
    if (!isHidden)
        show();

    emit stateRestored();
    return result;
}

bool DockManager::isRestoringState() const
{
    return d->m_restoringState;
}

bool DockManager::isLeavingMinimizedState() const
{
    return d->m_isLeavingMinimized;
}

bool DockManager::eventFilter(QObject *obj, QEvent *event)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    // Emulate Qt:Tool behavior.
    /*
    // Window always on top of the MainWindow.
    if (event->type() == QEvent::WindowActivate) {
        for (auto &floatingWidget : floatingWidgets()) {
            if (!floatingWidget->isVisible() || window()->isMinimized())
                continue;

            // setWindowFlags(Qt::WindowStaysOnTopHint) will hide the window and thus requires
            // a show call. This then leads to flickering and a nasty endless loop (also buggy
            // behavior on Ubuntu). So we just do it ourself.
            floatingWidget->setWindowFlag(Qt::WindowStaysOnTopHint, true);
        }
    } else if (event->type() == QEvent::WindowDeactivate) {
        for (auto &floatingWidget : floatingWidgets()) {
            if (!floatingWidget->isVisible() || window()->isMinimized())
                continue;

            floatingWidget->setWindowFlag(Qt::WindowStaysOnTopHint, false);

            floatingWidget->raise();
        }
    }
*/
    // Sync minimize with MainWindow
    if (event->type() == QEvent::WindowStateChange) {
        for (auto &floatingWidget : floatingWidgets()) {
            if (!floatingWidget->isVisible())
                continue;

            if (window()->isMinimized())
                floatingWidget->showMinimized();
            else
                floatingWidget->setWindowState(floatingWidget->windowState()
                                               & (~Qt::WindowMinimized));
        }
        if (!window()->isMinimized())
            QApplication::setActiveWindow(window());
    }
    return Super::eventFilter(obj, event);

#else
    if (event->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent *ev = static_cast<QWindowStateChangeEvent *>(event);
        if (ev->oldState().testFlag(Qt::WindowMinimized)) {
            d->m_isLeavingMinimized = true;
            QMetaObject::invokeMethod(
                this, [this] { endLeavingMinimizedState(); }, Qt::QueuedConnection);
        }
    }
    return Super::eventFilter(obj, event);
#endif
}

DockWidget *DockManager::focusedDockWidget() const
{
    if (!d->m_focusController)
        return nullptr;
    else
        return d->m_focusController->focusedDockWidget();
}

QList<int> DockManager::splitterSizes(DockAreaWidget *containedArea) const
{
    if (containedArea) {
        auto splitter = internal::findParent<DockSplitter *>(containedArea);
        if (splitter)
            return splitter->sizes();
    }

    return {};
}

void DockManager::setSplitterSizes(DockAreaWidget *containedArea, const QList<int> &sizes)
{
    if (!containedArea)
        return;

    auto splitter = internal::findParent<DockSplitter *>(containedArea);
    if (splitter && splitter->count() == sizes.count())
        splitter->setSizes(sizes);
}

void DockManager::setFloatingContainersTitle(const QString &title)
{
    g_floatingContainersTitle = title;
}

QString DockManager::floatingContainersTitle()
{
    if (g_floatingContainersTitle.isEmpty())
        return qApp->applicationDisplayName();

    return g_floatingContainersTitle;
}

DockWidget *DockManager::centralWidget() const
{
    return d->m_centralWidget;
}

DockAreaWidget *DockManager::setCentralWidget(DockWidget *widget)
{
    if (!widget) {
        d->m_centralWidget = nullptr;
        return nullptr;
    }

    // Setting a new central widget is now allowed if there is already a central widget or
    // if there are already other dock widgets.
    if (d->m_centralWidget) {
        qWarning(
            "Setting a central widget not possible because there is already a central widget.");
        return nullptr;
    }

    // Setting a central widget is now allowed if there are already other dock widgets.
    if (!d->m_dockWidgetsMap.isEmpty()) {
        qWarning("Setting a central widget not possible - the central widget need to be the first "
                 "dock widget that is added to the dock manager.");
        return nullptr;
    }

    widget->setFeature(DockWidget::DockWidgetClosable, false);
    widget->setFeature(DockWidget::DockWidgetMovable, false);
    widget->setFeature(DockWidget::DockWidgetFloatable, false);
    widget->setFeature(DockWidget::DockWidgetPinnable, false);
    d->m_centralWidget = widget;
    DockAreaWidget *centralArea = addDockWidget(CenterDockWidgetArea, widget);
    centralArea->setDockAreaFlag(DockAreaWidget::eDockAreaFlag::HideSingleWidgetTitleBar, true);
    return centralArea;
}

void DockManager::setDockWidgetFocused(DockWidget *dockWidget)
{
    if (d->m_focusController)
        d->m_focusController->setDockWidgetFocused(dockWidget);
}

void DockManager::hideManagerAndFloatingWidgets()
{
    hide();

    d->m_hiddenFloatingWidgets.clear();
    // Hide updates of floating widgets from user.
    for (auto &floatingWidget : d->m_floatingWidgets) {
        if (floatingWidget->isVisible()) {
            QList<DockWidget *> visibleWidgets;
            for (auto dockWidget : floatingWidget->dockWidgets()) {
                if (dockWidget->toggleViewAction()->isChecked())
                    visibleWidgets.push_back(dockWidget);
            }

            // Save as floating widget to be shown when DockManager will be shown back.
            d->m_hiddenFloatingWidgets.push_back(floatingWidget);
            floatingWidget->hide();

            // Hidding floating widget automatically marked contained DockWidgets as hidden
            // but they must remain marked as visible as we want them to be restored visible
            // when DockManager will be shown back.
            for (auto dockWidget : visibleWidgets)
                dockWidget->toggleViewAction()->setChecked(true);
        }
    }
}

void DockManager::endLeavingMinimizedState()
{
    d->m_isLeavingMinimized = false;
    this->activateWindow();
}

void DockManager::registerFloatingWidget(FloatingDockContainer *floatingWidget)
{
    d->m_floatingWidgets.append(floatingWidget);
    emit floatingWidgetCreated(floatingWidget);
    qCInfo(adsLog) << "d->FloatingWidgets.count() " << d->m_floatingWidgets.count();
}

void DockManager::removeFloatingWidget(FloatingDockContainer *floatingWidget)
{
    d->m_floatingWidgets.removeAll(floatingWidget);
}

void DockManager::registerDockContainer(DockContainerWidget *dockContainer)
{
    d->m_containers.append(dockContainer);
}

void DockManager::removeDockContainer(DockContainerWidget *dockContainer)
{
    if (this != dockContainer)
        d->m_containers.removeAll(dockContainer);
}

DockOverlay *DockManager::containerOverlay() const
{
    return d->m_containerOverlay;
}

DockOverlay *DockManager::dockAreaOverlay() const
{
    return d->m_dockAreaOverlay;
}

void DockManager::notifyWidgetOrAreaRelocation(QWidget *droppedWidget)
{
    if (d->m_focusController)
        d->m_focusController->notifyWidgetOrAreaRelocation(droppedWidget);
}

void DockManager::notifyFloatingWidgetDrop(FloatingDockContainer *floatingWidget)
{
    if (d->m_focusController)
        d->m_focusController->notifyFloatingWidgetDrop(floatingWidget);
}

void DockManager::notifyDockWidgetRelocation(DockWidget *, DockContainerWidget *) {}

void DockManager::notifyDockAreaRelocation(DockAreaWidget *, DockContainerWidget *) {}

void DockManager::showEvent(QShowEvent *event)
{
    Super::showEvent(event);
    // Fix issue #380
    restoreHiddenFloatingWidgets();

    if (d->m_uninitializedFloatingWidgets.empty())
        return;

    for (auto floatingWidget : std::as_const(d->m_uninitializedFloatingWidgets)) {
        // Check, if someone closed a floating dock widget before the dock manager is shown.
        if (floatingWidget->dockContainer()->hasOpenDockAreas())
            floatingWidget->show();
    }

    d->m_uninitializedFloatingWidgets.clear();
}

DockFocusController *DockManager::dockFocusController() const
{
    return d->m_focusController;
}

void DockManager::restoreHiddenFloatingWidgets()
{
    if (d->m_hiddenFloatingWidgets.isEmpty())
        return;

    // Restore floating widgets that were hidden upon hideManagerAndFloatingWidgets
    for (auto &floatingWidget : d->m_hiddenFloatingWidgets) {
        bool hasDockWidgetVisible = false;

        // Needed to prevent FloatingDockContainer being shown empty
        // Could make sense to move this to FloatingDockContainer::showEvent(QShowEvent *event)
        // if experiencing FloatingDockContainer being shown empty in other situations, but let's keep
        // it here for now to make sure changes to fix Issue #380 does not impact existing behaviours
        for (auto dockWidget : floatingWidget->dockWidgets()) {
            if (dockWidget->toggleViewAction()->isChecked()) {
                dockWidget->toggleView(true);
                hasDockWidgetVisible = true;
            }
        }

        if (hasDockWidgetVisible)
            floatingWidget->show();
    }

    d->m_hiddenFloatingWidgets.clear();
}

Workspace *DockManager::activeWorkspace() const
{
    return &d->m_workspace;
}

QString DockManager::startupWorkspace() const
{
    QTC_ASSERT(d->m_settings, return {});
    return d->m_settings->value(Constants::STARTUP_WORKSPACE_SETTINGS_KEY).toString();
}

bool DockManager::autoRestoreWorkspace() const
{
    QTC_ASSERT(d->m_settings, return false);
    return d->m_settings->value(Constants::AUTO_RESTORE_WORKSPACE_SETTINGS_KEY).toBool();
}

const QList<Workspace> &DockManager::workspaces() const
{
    return d->m_workspaces;
}

int DockManager::workspaceIndex(const QString &fileName) const
{
    return Utils::indexOf(d->m_workspaces, [&fileName](const Workspace &workspace) {
        return workspace == fileName;
    });
}

bool DockManager::workspaceExists(const QString &fileName) const
{
    return workspaceIndex(fileName) >= 0;
}

Workspace *DockManager::workspace(const QString &fileName) const
{
    auto result = std::find_if(std::begin(d->m_workspaces),
                               std::end(d->m_workspaces),
                               [&fileName](const Workspace &workspace) {
                                   return workspace == fileName;
                               });

    if (result != std::end(d->m_workspaces))
        return &*result;

    return nullptr;
}

Workspace *DockManager::workspace(int index) const
{
    if (index < 0 || index >= d->m_workspaces.count())
        return nullptr;

    return &d->m_workspaces[index];
}

QDateTime DockManager::workspaceDateTime(const QString &fileName) const
{
    Workspace *w = workspace(fileName);

    if (w)
        return w->lastModified();

    return QDateTime();
}

bool DockManager::moveWorkspace(int from, int to)
{
    qCInfo(adsLog) << "Move workspaces" << from << ">" << to;

    int c = d->m_workspaces.count();

    from = std::clamp(from, 0, c);
    to = std::clamp(to, 0, c);

    if (from == to)
        return false;

    d->m_workspaces.move(from, to);
    emit workspaceListChanged();

    return true;
}

bool DockManager::moveWorkspaceUp(const QString &fileName)
{
    qCInfo(adsLog) << "Move workspace up" << fileName;

    int i = workspaceIndex(fileName);

    // If workspace does not exist or is first item
    if (i <= 0)
        return false;

    d->m_workspaces.swapItemsAt(i, i - 1);
    emit workspaceListChanged();

    return true;
}

bool DockManager::moveWorkspaceDown(const QString &fileName)
{
    qCInfo(adsLog) << "Move workspace down" << fileName;

    int i = workspaceIndex(fileName);
    int c = d->m_workspaces.count();

    // If workspace does not exist or is last item
    if (i < 0 || i >= (c - 1))
        return false;

    d->m_workspaces.swapItemsAt(i, i + 1);
    emit workspaceListChanged();

    return true;
}

FilePath DockManager::workspaceNameToFilePath(const QString &workspaceName) const
{
    QTC_ASSERT(d->m_settings, return {});
    return userDirectory().pathAppended(workspaceNameToFileName(workspaceName));
}

QString DockManager::workspaceNameToFileName(const QString &workspaceName) const
{
    QString copy = workspaceName;
    copy.replace(" ", "-");
    copy.append("." + workspaceFileExtension);
    return copy;
}

void DockManager::uniqueWorkspaceFileName(QString &fileName) const
{
    auto filePath = FilePath::fromString(fileName);

    if (workspaceExists(fileName)) {
        int i = 1;
        QString copy;
        do {
            copy = filePath.baseName() + QString::number(i) + "." + filePath.completeSuffix();
            ++i;
        } while (workspaceExists(copy));
        fileName = copy;
    }
}

void DockManager::showWorkspaceMananger()
{
    save(); // Save current workspace

    WorkspaceDialog workspaceDialog(this, parentWidget());
    workspaceDialog.setAutoLoadWorkspace(autoRestoreWorkspace());
    workspaceDialog.exec();

    if (d->m_workspaceOrderDirty) {
        writeOrder();
        d->m_workspaceOrderDirty = false;
    }

    QTC_ASSERT(d->m_settings, return);
    d->m_settings->setValue(Constants::AUTO_RESTORE_WORKSPACE_SETTINGS_KEY,
                            workspaceDialog.autoLoadWorkspace());
}

expected_str<QString> DockManager::createWorkspace(const QString &workspaceName)
{
    qCInfo(adsLog) << "Create workspace" << workspaceName;

    QString fileName = workspaceNameToFileName(workspaceName);
    uniqueWorkspaceFileName(fileName);
    const FilePath filePath = userDirectory().pathAppended(fileName);

    expected_str<void> result = write(filePath, saveState(workspaceName));
    if (!result)
        return make_unexpected(result.error());

    Workspace workspace(filePath, false);

    d->m_workspaces.append(workspace);
    emit workspaceListChanged();
    return workspace.fileName();
}

expected_str<void> DockManager::openWorkspace(const QString &fileName)
{
    qCInfo(adsLog) << "Open workspace" << fileName;

    Workspace *wrk = workspace(fileName);
    if (!wrk)
        return make_unexpected(Tr::tr("Workspace \"%1\" does not exist.").arg(fileName));

    // Do nothing if workspace is already loaded, exception if it is a preset workspace. In this
    // case we still want to be able to load the default workspace to undo potential user changes.
    if (*wrk == *activeWorkspace() && !wrk->isPreset())
        return {}; // true

    if (activeWorkspace()->isValid()) {
        // Allow everyone to set something in the workspace and before saving
        emit aboutToUnloadWorkspace(activeWorkspace()->fileName());
        expected_str<void> saveResult = save();
        if (!saveResult)
            return saveResult;
    }

    // Try loading the file
    const expected_str<QByteArray> data = loadWorkspace(*wrk);
    if (!data)
        return make_unexpected(data.error());

    emit openingWorkspace(wrk->fileName());
    // If data was loaded from file try to restore its state
    if (!data->isNull() && !restoreState(*data))
        return make_unexpected(Tr::tr("Cannot restore \"%1\".").arg(wrk->filePath().toUserOutput()));

    d->m_workspace = *wrk;
    emit workspaceLoaded(wrk->fileName());

    return {};
}

expected_str<void> DockManager::reloadActiveWorkspace()
{
    qCInfo(adsLog) << "Reload active workspace";

    Workspace *wrk = activeWorkspace();

    if (!workspaces().contains(*wrk))
        return make_unexpected(
            Tr::tr("Cannot reload \"%1\". It is not in the list of workspaces.")
                .arg(wrk->filePath().toUserOutput()));

    const expected_str<QByteArray> data = loadWorkspace(*wrk);
    if (!data)
        return make_unexpected(data.error());

    if (!data->isNull() && !restoreState(*data))
        return make_unexpected(Tr::tr("Cannot restore \"%1\".").arg(wrk->filePath().toUserOutput()));

    emit workspaceReloaded(wrk->fileName());

    return {};
}

bool DockManager::deleteWorkspace(const QString &fileName)
{
    qCInfo(adsLog) << "Delete workspace" << fileName;

    Workspace *w = workspace(fileName);
    if (!w) {
        qWarning() << "Workspace" << fileName << "does not exist";
        return false;
    }

    // Remove corresponding workspace file
    if (w->exists()) {
        const FilePath filePath = w->filePath();
        if (filePath.removeFile()) {
            d->m_workspaces.removeOne(*w);
            emit workspaceRemoved();
            emit workspaceListChanged();
            return true;
        }
    }

    return false;
}

void DockManager::deleteWorkspaces(const QStringList &fileNames)
{
    for (const QString &fileName : fileNames)
        deleteWorkspace(fileName);
}

expected_str<QString> DockManager::cloneWorkspace(const QString &originalFileName,
                                                  const QString &cloneName)
{
    qCInfo(adsLog) << "Clone workspace" << originalFileName << cloneName;

    Workspace *w = workspace(originalFileName);
    if (!w)
        return make_unexpected(Tr::tr("Workspace \"%1\" does not exist.").arg(originalFileName));

    const FilePath originalPath = w->filePath();

    if (!originalPath.exists())
        return make_unexpected(
            Tr::tr("Workspace \"%1\" does not exist.").arg(originalPath.toUserOutput()));

    const FilePath clonePath = workspaceNameToFilePath(cloneName);

    const expected_str<void> copyResult = originalPath.copyFile(clonePath);
    if (!copyResult)
        return make_unexpected(Tr::tr("Could not clone \"%1\" due to: %2")
                                   .arg(originalPath.toUserOutput(), copyResult.error()));

    writeDisplayName(clonePath, cloneName);
    d->m_workspaces.append(Workspace(clonePath));
    emit workspaceListChanged();
    return clonePath.fileName();
}

expected_str<QString> DockManager::renameWorkspace(const QString &originalFileName,
                                                   const QString &newName)
{
    qCInfo(adsLog) << "Rename workspace" << originalFileName << newName;

    Workspace *w = workspace(originalFileName);
    if (!w)
        return make_unexpected(Tr::tr("Workspace \"%1\" does not exist.").arg(originalFileName));

    w->setName(newName);

    emit workspaceListChanged();
    return originalFileName;
}

expected_str<void> DockManager::resetWorkspacePreset(const QString &fileName)
{
    qCInfo(adsLog) << "Reset workspace" << fileName;

    Workspace *w = workspace(fileName);
    if (!w)
        return make_unexpected(Tr::tr("Workspace \"%1\" does not exist.").arg(fileName));

    if (!w->isPreset())
        return make_unexpected(Tr::tr("Workspace \"%1\" is not a preset.").arg(fileName));

    const FilePath filePath = w->filePath();

    if (!filePath.removeFile())
        return make_unexpected(Tr::tr("Cannot remove \"%1\".").arg(filePath.toUserOutput()));

    return presetDirectory().pathAppended(fileName).copyFile(filePath);
}

expected_str<void> DockManager::save()
{
    if (isModeChangeState())
        return make_unexpected(Tr::tr("Cannot save workspace while in mode change state."));

    emit aboutToSaveWorkspace();

    Workspace *w = activeWorkspace();

    return write(w->filePath(), saveState(w->name()));
}

void DockManager::setModeChangeState(bool value)
{
    d->m_modeChangeState = value;
}

bool DockManager::isModeChangeState() const
{
    return d->m_modeChangeState;
}

expected_str<QString> DockManager::importWorkspace(const QString &filePath)
{
    qCInfo(adsLog) << "Import workspace" << filePath;

    const FilePath sourceFilePath = FilePath::fromUserInput(filePath);

    if (!sourceFilePath.exists())
        return make_unexpected(
            Tr::tr("File \"%1\" does not exist.").arg(sourceFilePath.toUserOutput()));

    // Extract workspace file name. Check if the workspace is already contained in the list of
    // workspaces. If that is the case add a counter to the workspace file name.
    QString fileName = sourceFilePath.fileName();
    uniqueWorkspaceFileName(fileName);

    const FilePath targetFilePath = userDirectory().pathAppended(fileName);

    const expected_str<void> copyResult = sourceFilePath.copyFile(targetFilePath);
    if (!copyResult)
        return make_unexpected(
            Tr::tr("Could not copy \"%1\" to \"%2\" due to: %3")
                .arg(filePath, targetFilePath.toUserOutput(), copyResult.error()));

    d->m_workspaces.append(Workspace(targetFilePath));
    emit workspaceListChanged();

    return targetFilePath.fileName();
}

expected_str<QString> DockManager::exportWorkspace(const QString &targetFilePath,
                                                   const QString &sourceFileName)
{
    qCInfo(adsLog) << "Export workspace" << targetFilePath << sourceFileName;

    // If we came this far the user decided that in case the target already exists to overwrite it.
    // We first need to remove the existing file, otherwise QFile::copy() will fail.
    const FilePath targetFile = FilePath::fromUserInput(targetFilePath);

    // Remove the file which supposed to be overwritten
    if (targetFile.exists()) {
        if (!targetFile.removeFile()) {
            return make_unexpected(
                Tr::tr("Could not remove \"%1\".").arg(targetFile.toUserOutput()));
        }
    }

    // Check if the target directory exists
    if (!targetFile.parentDir().exists())
        return make_unexpected(
            Tr::tr("The directory \"%1\" does not exist.").arg(targetFile.parentDir().toUserOutput()));

    // Check if the workspace exists
    const FilePath workspaceFile = userDirectory().pathAppended(sourceFileName);
    if (!workspaceFile.exists())
        return make_unexpected(
            Tr::tr("The workspace \"%1\" does not exist ").arg(workspaceFile.toUserOutput()));

    // Finally copy the workspace to the target
    const expected_str<void> copyResult = workspaceFile.copyFile(targetFile);
    if (!copyResult)
        return make_unexpected(
            Tr::tr("Could not copy \"%1\" to \"%2\" due to: %3")
                .arg(sourceFileName, workspaceFile.toUserOutput(), copyResult.error()));

    return workspaceFile.fileName();
}

QStringList DockManager::loadOrder(const FilePath &dir) const
{
    qCInfo(adsLog) << "Load workspace order" << dir.toUserOutput();

    if (dir.isEmpty())
        return {};

    auto filePath = dir.pathAppended(workspaceOrderFileName.toString());
    QByteArray data = loadFile(filePath);

    if (data.isEmpty()) {
        qWarning() << "Order data is empty" << filePath.toUserOutput();
        return {};
    }

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse JSON file" << filePath << ":" << parseError.errorString();
        return {};
    }

    return jsonDoc.toVariant().toStringList();
}

bool DockManager::writeOrder() const
{
    qCInfo(adsLog) << "Write workspace order";

    QJsonArray order;
    for (const Workspace &workspace : d->m_workspaces)
        order.append(QJsonValue(workspace.fileName()));

    const FilePath orderFilePath = userDirectory().pathAppended(workspaceOrderFileName.toString());

    if (!orderFilePath.writeFileContents(QJsonDocument{order}.toJson())) {
        qWarning() << "Faild to write order" << orderFilePath.toUserOutput();
        return false;
    }

    return true;
}

QList<Workspace> DockManager::loadWorkspaces(const FilePath &dir) const
{
    if (dir.isEmpty())
        return {};

    QList<Workspace> workspaces;

    dir.iterateDirectory(
        [&workspaces](const FilePath &filePath, const FilePathInfo &) {
            Workspace workspace(filePath);
            //workspace.setName(displayName(filePath)); TODO
            workspaces.append(workspace);
            return IterationPolicy::Continue;
        },
        FileFilter(QStringList{"*." + workspaceFileExtension}, QDir::Files));

    return workspaces;
}

FilePath DockManager::presetDirectory() const
{
    return FilePath::fromString(d->m_workspacePresetsPath);
}

FilePath DockManager::userDirectory() const
{
    QTC_ASSERT(d->m_settings, return {});
    return FilePath::fromString(QFileInfo(d->m_settings->fileName()).path())
        .pathAppended(workspaceFolderName.toString());
}

QByteArray DockManager::loadFile(const FilePath &filePath)
{
    qCInfo(adsLog) << "Load" << filePath;

    if (!filePath.exists()) {
        qWarning() << "File does not exist" << filePath.toUserOutput();
        return {};
    }

    const expected_str<QByteArray> data = filePath.fileContents();

    if (!data) {
        qWarning() << "Could not open" << filePath.toUserOutput() << data.error();
        return {};
    }

    return data.value();
}

QString DockManager::readDisplayName(const FilePath &filePath)
{
    if (!filePath.exists())
        return {};

    auto data = loadFile(filePath);

    if (data.isEmpty())
        return {};

    auto tmp = data.startsWith("<?xml") ? data : qUncompress(data);

    DockingStateReader reader(tmp);
    if (!reader.readNextStartElement())
        return {};

    if (reader.name() != QLatin1String("QtAdvancedDockingSystem"))
        return {};

    return reader.attributes().value(workspaceDisplayNameAttribute.toString()).toString();
}

bool DockManager::writeDisplayName(const FilePath &filePath, const QString &displayName)
{
    const expected_str<QByteArray> content = filePath.fileContents();

    QTC_ASSERT_EXPECTED(content, return false);

    QDomDocument doc;
    QString error_msg;
    int error_line, error_col;
    if (!doc.setContent(*content, &error_msg, &error_line, &error_col)) {
        qWarning() << QString("XML error on line %1, col %2: %3")
                          .arg(error_line)
                          .arg(error_col)
                          .arg(error_msg);
        return false;
    }

    QDomElement docElem = doc.documentElement();
    docElem.setAttribute(workspaceDisplayNameAttribute.toString(), displayName);

    const expected_str<void> result = write(filePath, doc.toByteArray(workspaceXmlFormattingIndent));
    if (!result) {
        qWarning() << "Could not write display name" << displayName << "to" << filePath << ":"
                   << result.error();
        return false;
    }

    return true;
}

expected_str<void> DockManager::write(const FilePath &filePath, const QByteArray &data)
{
    qCInfo(adsLog) << "Write" << filePath;

    if (!filePath.parentDir().ensureWritableDir())
        return make_unexpected(Tr::tr("Cannot write to \"%1\".").arg(filePath.toUserOutput()));

    FileSaver fileSaver(filePath, QIODevice::Text);
    if (!fileSaver.hasError())
        fileSaver.write(data);

    if (!fileSaver.finalize())
        return make_unexpected(Tr::tr("Cannot write to \"%1\" due to: %2")
                                   .arg(filePath.toUserOutput(), fileSaver.errorString()));

    return {};
}

expected_str<QByteArray> DockManager::loadWorkspace(const Workspace &workspace) const
{
    qCInfo(adsLog) << "Load workspace" << workspace.fileName();

    if (!workspace.exists())
        return make_unexpected(
            Tr::tr("Workspace \"%1\" does not exist.").arg(workspace.filePath().toUserOutput()));

    return workspace.filePath().fileContents();
}

void DockManager::syncWorkspacePresets()
{
    qCInfo(adsLog) << "Sync workspaces";

    auto fileFilter = FileFilter(QStringList{"*." + workspaceFileExtension, "*.json"}, QDir::Files);

    // All files in the workspace preset directory with the file extension wrk or json
    auto presetFiles = presetDirectory().dirEntries(fileFilter);
    auto userFiles = userDirectory().dirEntries(fileFilter);

    // Try do create the 'workspaces' directory if it doesn't exist already
    if (!userDirectory().ensureWritableDir()) {
        qWarning() << QString("Could not make directory '%1')").arg(userDirectory().toUserOutput());
        return;
    }

    for (const auto &filePath : presetFiles) {
        const int index = Utils::indexOf(userFiles, [&filePath](const FilePath &other) {
            return other.fileName() == filePath.fileName();
        });

        // File already contained in user directory
        if (index >= 0) {
            auto userFile = userFiles[index];

            // If *.wrk file and displayName attribute is empty set the displayName. This
            // should fix old workspace files which don't have the displayName attribute.
            if (userFile.suffix() == workspaceFileExtension && readDisplayName(userFile).isEmpty())
                writeDisplayName(userFile, readDisplayName(filePath));

            continue;
        }

        const expected_str<void> copyResult = filePath.copyFile(
            userDirectory().pathAppended(filePath.fileName()));
        if (!copyResult)
            qWarning() << QString("Could not copy '%1' to '%2' due to %3")
                              .arg(filePath.toUserOutput(),
                                   userDirectory().toUserOutput(),
                                   copyResult.error());
    }
}

void DockManager::prepareWorkspaces()
{
    qCInfo(adsLog) << "Prepare workspace";

    auto presetWorkspaces = loadWorkspaces(presetDirectory());
    auto userWorkspaces = loadWorkspaces(userDirectory());
    auto userOrder = loadOrder(userDirectory());

    std::vector<std::pair<int, Workspace>> tmp;

    for (Workspace &userWorkspace : userWorkspaces) {
        userWorkspace.setPreset(presetWorkspaces.contains(userWorkspace));
        qsizetype index = userOrder.indexOf(userWorkspace.fileName());
        if (index == -1)
            index = userWorkspaces.count();
        tmp.push_back(std::make_pair(index, userWorkspace));
    }

    Utils::sort(tmp, [](auto const &a, auto const &b) { return a.first < b.first; });

    d->m_workspaces = Utils::transform<QList<Workspace>>(tmp,
                                                         [](const std::pair<int, Workspace> &p) {
                                                             return p.second;
                                                         });

    emit workspaceListChanged();
}

void DockManager::saveStartupWorkspace()
{
    QTC_ASSERT(d->m_settings, return);
    d->m_settings->setValue(Constants::STARTUP_WORKSPACE_SETTINGS_KEY,
                            activeWorkspace()->fileName());
}

} // namespace ADS

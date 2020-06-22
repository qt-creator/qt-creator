/****************************************************************************
**
** Copyright (C) 2020 Uwe Kindler
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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or (at your option) any later version.
** The licenses are as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPLv21 included in the packaging
** of this file. Please review the following information to ensure
** the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "ads_globals.h"
#include "dockcontainerwidget.h"
#include "dockwidget.h"
#include "floatingdockcontainer.h"

#include <utils/persistentsettings.h>

#include <QByteArray>
#include <QDateTime>
#include <QFlags>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QtGui/QIcon>
#include <qobjectdefs.h>

QT_BEGIN_NAMESPACE
class QMenu;
class QSettings;
QT_END_NAMESPACE

namespace ADS {

namespace Constants {
const char DEFAULT_WORKSPACE[] = "Essentials"; // This needs to align with a name of the shipped presets
const char STARTUP_WORKSPACE_SETTINGS_KEY[] = "QML/Designer/StartupWorkspace";
const char AUTO_RESTORE_WORKSPACE_SETTINGS_KEY[] = "QML/Designer/AutoRestoreLastWorkspace";
} // namespace Constants

class DockManagerPrivate;
class FloatingDockContainer;
class FloatingDockContainerPrivate;
class DockComponentsFactory;
class DockContainerWidget;
class DockContainerWidgetPrivate;
class DockOverlay;
class DockAreaTabBar;
class DockWidgetTab;
class DockWidgetTabPrivate;
struct DockAreaWidgetPrivate;
class IconProvider;

/**
 * The central dock manager that maintains the complete docking system.
 * With the configuration flags you can globally control the functionality
 * of the docking system. The dock manager uses an internal stylesheet to
 * style its components like splitters, tabs and buttons. If you want to
 * disable this stylesheet because your application uses its own,
 * just call the function for settings the stylesheet with an empty
 * string.
 * \code
 * dockManager->setStyleSheet("");
 * \endcode
 **/
class ADS_EXPORT DockManager : public DockContainerWidget
{
    Q_OBJECT
private:
    DockManagerPrivate *d; ///< private data (pimpl)
    friend class DockManagerPrivate;
    friend class FloatingDockContainer;
    friend class FloatingDockContainerPrivate;
    friend class DockContainerWidget;
    friend class DockContainerWidgetPrivate;
    friend class DockAreaTabBar;
    friend class DockWidgetTab;
    friend struct DockAreaWidgetPrivate;
    friend class DockWidgetTabPrivate;
    friend class FloatingDragPreview;
    friend class FloatingDragPreviewPrivate;
    friend class DockAreaTitleBar;

protected:
    /**
     * Registers the given floating widget in the internal list of
     * floating widgets
     */
    void registerFloatingWidget(FloatingDockContainer *floatingWidget);

    /**
     * Remove the given floating widget from the list of registered floating
     * widgets
     */
    void removeFloatingWidget(FloatingDockContainer *floatingWidget);

    /**
     * Registers the given dock container widget
     */
    void registerDockContainer(DockContainerWidget *dockContainer);

    /**
     * Remove dock container from the internal list of registered dock
     * containers
     */
    void removeDockContainer(DockContainerWidget *dockContainer);

    /**
     * Overlay for containers
     */
    DockOverlay *containerOverlay() const;

    /**
     * Overlay for dock areas
     */
    DockOverlay *dockAreaOverlay() const;

    /**
     * A container needs to call this function if a widget has been dropped
     * into it
     */
    void notifyWidgetOrAreaRelocation(QWidget *droppedWidget);

    /**
     * This function is called, if a floating widget has been dropped into
     * an new position.
     * When this function is called, all dock widgets of the FloatingWidget
     * are already inserted into its new position
     */
    void notifyFloatingWidgetDrop(FloatingDockContainer *floatingWidget);

    /**
     * This function is called, if the given DockWidget has been relocated from
     * the old container ContainerOld to the new container DockWidget->dockContainer()
     */
    void notifyDockWidgetRelocation(DockWidget *dockWidget, DockContainerWidget *containerOld);

    /**
     * This function is called, if the given DockAreahas been relocated from
     * the old container ContainerOld to the new container DockArea->dockContainer()
     */
    void notifyDockAreaRelocation(DockAreaWidget *dockArea, DockContainerWidget *containerOld);

    /**
     * Show the floating widgets that has been created floating
     */
    void showEvent(QShowEvent *event) override;

public:
    using Super = DockContainerWidget;

    /**
     * These global configuration flags configure some global dock manager settings.
     * Set the dock manager flags, before you create the dock manager instance.
     */
    enum eConfigFlag {
        ActiveTabHasCloseButton
        = 0x0001, //!< If this flag is set, the active tab in a tab area has a close button
        DockAreaHasCloseButton
        = 0x0002, //!< If the flag is set each dock area has a close button
        DockAreaCloseButtonClosesTab
        = 0x0004, //!< If the flag is set, the dock area close button closes the active tab, if not set, it closes the complete dock area
        OpaqueSplitterResize
        = 0x0008, //!< See QSplitter::setOpaqueResize() documentation
        XmlAutoFormattingEnabled
        = 0x0010, //!< If enabled, the XML writer automatically adds line-breaks and indentation to empty sections between elements (ignorable whitespace).
        XmlCompressionEnabled
        = 0x0020, //!< If enabled, the XML output will be compressed and is not human readable anymore
        TabCloseButtonIsToolButton
        = 0x0040, //! If enabled the tab close buttons will be QToolButtons instead of QPushButtons - disabled by default
        AllTabsHaveCloseButton
        = 0x0080, //!< if this flag is set, then all tabs that are closable show a close button
        RetainTabSizeWhenCloseButtonHidden
        = 0x0100, //!< if this flag is set, the space for the close button is reserved even if the close button is not visible
        OpaqueUndocking
        = 0x0200, ///< If enabled, the widgets are immediately undocked into floating widgets, if disabled, only a draw preview is undocked and the real undocking is deferred until the mouse is released
        DragPreviewIsDynamic
        = 0x0400, ///< If opaque undocking is disabled, this flag defines the behavior of the drag preview window, if this flag is enabled, the preview will be adjusted dynamically to the drop area
        DragPreviewShowsContentPixmap
        = 0x0800, ///< If opaque undocking is disabled, the created drag preview window shows a copy of the content of the dock widget / dock are that is dragged
        DragPreviewHasWindowFrame
        = 0x1000, ///< If opaque undocking is disabled, then this flag configures if the drag preview is frameless or looks like a real window
        AlwaysShowTabs
        = 0x2000, ///< If this option is enabled, the tab of a dock widget is always displayed - even if it is the only visible dock widget in a floating widget.
        DockAreaHasUndockButton
        = 0x4000, //!< If the flag is set each dock area has an undock button
        DockAreaHasTabsMenuButton
        = 0x8000, //!< If the flag is set each dock area has a tabs menu button
        DockAreaHideDisabledButtons
        = 0x10000, //!< If the flag is set disabled dock area buttons will not appear on the tollbar at all (enabling them will bring them back)
        DockAreaDynamicTabsMenuButtonVisibility
        = 0x20000, //!< If the flag is set, the tabs menu button will be shown only when it is required - that means, if the tabs are elided. If the tabs are not elided, it is hidden
        FloatingContainerHasWidgetTitle
        = 0x40000, //!< If set, the Floating Widget window title reflects the title of the current dock widget otherwise it displays application name as window title
        FloatingContainerHasWidgetIcon
        = 0x80000, //!< If set, the Floating Widget icon reflects the icon of the current dock widget otherwise it displays application icon
        HideSingleCentralWidgetTitleBar
        = 0x100000, //!< If there is only one single visible dock widget in the main dock container (the dock manager) and if this flag is set, then the titlebar of this dock widget will be hidden
                    //!< this only makes sense for non draggable and non floatable widgets and enables the creation of some kind of "central" widget
        FocusHighlighting
        = 0x200000, //!< enables styling of focused dock widget tabs or floating widget titlebar
        EqualSplitOnInsertion
        = 0x400000, ///!< if enabled, the space is equally distributed to all widgets in a splitter

        DefaultDockAreaButtons = DockAreaHasCloseButton
                               | DockAreaHasUndockButton
                               | DockAreaHasTabsMenuButton,///< default configuration of dock area title bar buttons

        DefaultBaseConfig = DefaultDockAreaButtons
                          | ActiveTabHasCloseButton
                          | XmlCompressionEnabled
                          | FloatingContainerHasWidgetTitle,///< default base configuration settings

        DefaultOpaqueConfig = DefaultBaseConfig
                            | OpaqueSplitterResize
                            | OpaqueUndocking, ///< the default configuration with opaque operations - this may cause issues if ActiveX or Qt 3D windows are involved

        DefaultNonOpaqueConfig = DefaultBaseConfig
                               | DragPreviewShowsContentPixmap, ///< the default configuration for non opaque operations

        NonOpaqueWithWindowFrame = DefaultNonOpaqueConfig
                                 | DragPreviewHasWindowFrame ///< the default configuration for non opaque operations that show a real window with frame
    };
    Q_DECLARE_FLAGS(ConfigFlags, eConfigFlag)

    /**
     * Default Constructor.
     * If the given parent is a QMainWindow, the dock manager sets itself as the
     * central widget.
     * Before you create any dock widgets, you should properly setup the
     * configuration flags via setConfigFlags().
     */
    DockManager(QWidget *parent = nullptr);

    /**
     * Virtual Destructor
     */
    ~DockManager() override;

    /**
     * This function returns the global configuration flags.
     */
    static ConfigFlags configFlags();

    /**
     * Sets the global configuration flags for the whole docking system.
     * Call this function before you create the dock manager and before
     * your create the first dock widget.
     */
    static void setConfigFlags(const ConfigFlags flags);

    /**
     * Set a certain config flag.
     * \see setConfigFlags()
     */
    static void setConfigFlag(eConfigFlag flag, bool on = true);

    /**
     * Returns true if the given config flag is set.
     */
    static bool testConfigFlag(eConfigFlag flag);

    /**
     * Returns the global icon provider.
     * The icon provider enables the use of custom icons in case using
     * styleheets for icons is not an option.
     */
    static IconProvider &iconProvider();

    /**
     * The distance the user needs to move the mouse with the left button
     * hold down before a dock widget start floating.
     */
    static int startDragDistance();

    /**
     * Helper function to set focus depending on the configuration of the
     * FocusStyling flag
     */
    template <class QWidgetPtr>
    static void setWidgetFocus(QWidgetPtr widget)
    {
        if (!DockManager::testConfigFlag(DockManager::FocusHighlighting))
            return;

        widget->setFocus(Qt::OtherFocusReason);
    }

    /**
     * Set the QtCreator settings.
     */
    void setSettings(QSettings *settings);

    /**
     * Set the path to the workspace presets folder.
     */
    void setWorkspacePresetsPath(const QString &path);

    void initialize();

    /**
     * Adds dockwidget into the given area.
     * If DockAreaWidget is not null, then the area parameter indicates the area
     * into the DockAreaWidget. If DockAreaWidget is null, the Dockwidget will
     * be dropped into the container. If you would like to add a dock widget
     * tabified, then you need to add it to an existing dock area object
     * into the CenterDockWidgetArea. The following code shows this:
     * \code
     * DockManager->addDockWidget(ads::CenterDockWidgetArea, NewDockWidget,
     *         ExisitingDockArea);
     * \endcode
     * \return Returns the dock area widget that contains the new DockWidget
     */
    DockAreaWidget *addDockWidget(DockWidgetArea area,
                                  DockWidget *dockWidget,
                                  DockAreaWidget *dockAreaWidget = nullptr);

    /**
     * This function will add the given Dockwidget to the given dock area as
     * a new tab.
     * If no dock area widget exists for the given area identifier, a new
     * dock area widget is created.
     */
    DockAreaWidget *addDockWidgetTab(DockWidgetArea area, DockWidget *dockWidget);

    /**
     * This function will add the given Dockwidget to the given DockAreaWidget
     * as a new tab.
     */
    DockAreaWidget *addDockWidgetTabToArea(DockWidget *dockWidget, DockAreaWidget *dockAreaWidget);

    /**
     * Adds the given DockWidget floating and returns the created
     * CFloatingDockContainer instance.
     */
    FloatingDockContainer *addDockWidgetFloating(DockWidget *dockWidget);

    /**
     * Searches for a registered doc widget with the given ObjectName
     * \return Return the found dock widget or nullptr if a dock widget with the
     * given name is not registered.
     */
    DockWidget *findDockWidget(const QString &objectName) const;

    /**
     * Remove the given DockWidget from the dock manager.
     */
    void removeDockWidget(DockWidget *dockWidget);

    /**
     * This function returns a readable reference to the internal dock
     * widgets map so that it is possible to iterate over all dock widgets.
     */
    QMap<QString, DockWidget *> dockWidgetsMap() const;

    /**
     * Returns the list of all active and visible dock containers
     * Dock containers are the main dock manager and all floating widgets.
     */
    const QList<DockContainerWidget *> dockContainers() const;

    /**
     * Returns the list of all floating widgets.
     */
    const QList<FloatingDockContainer *> floatingWidgets() const;

    /**
     * This function always return 0 because the main window is always behind
     * any floating widget.
     */
    unsigned int zOrderIndex() const override;

    /**
     * Saves the current state of the dockmanger and all its dock widgets
     * into the returned QByteArray.
     * The XmlMode enables / disables the auto formatting for the XmlStreamWriter.
     * If auto formatting is enabled, the output is intended and line wrapped.
     * The XmlMode XmlAutoFormattingDisabled is better if you would like to have
     * a more compact XML output - i.e. for storage in ini files.
     * The version number is stored as part of the data.
     * To restore the saved state, pass the return value and version number
     * to restoreState().
     * \see restoreState()
     */
    QByteArray saveState(int version = 0) const;

    /**
     * Restores the state of this dockmanagers dockwidgets.
     * The version number is compared with that stored in state. If they do
     * not match, the dockmanager's state is left unchanged, and this function
     * returns false; otherwise, the state is restored, and this function
     * returns true.
     * \see saveState()
     */
    bool restoreState(const QByteArray &state, int version = 0);

    /**
     * This function returns true between the restoringState() and
     * stateRestored() signals.
     */
    bool isRestoringState() const;

    /**
     * Request a focus change to the given dock widget.
     * This function only has an effect, if the flag CDockManager::FocusStyling
     * is enabled
     */
    void setDockWidgetFocused(DockWidget *dockWidget);

signals:
    /**
     * This signal is emitted if the list of workspaces changed.
     */
    void workspaceListChanged();

    /**
     * This signal is emitted if workspaces have been removed.
     */
    void workspacesRemoved();

    /**
     * This signal is emitted, if the restore function is called, just before
     * the dock manager starts restoring the state.
     * If this function is called, nothing has changed yet.
     */
    void restoringState();

    /**
     * This signal is emitted if the state changed in restoreState.
     * The signal is emitted if the restoreState() function is called or
     * if the openWorkspace() function is called.
     */
    void stateRestored();

    /**
     * This signal is emitted, if the dock manager starts opening a
     * workspace.
     * Opening a workspace may take more than a second if there are
     * many complex widgets. The application may use this signal
     * to show some progress indicator or to change the mouse cursor
     * into a busy cursor.
     */
    void openingWorkspace(const QString &workspaceName);

    /**
     * This signal is emitted if the dock manager finished opening a
     * workspace.
     */
    void workspaceOpened(const QString &workspaceName);

    /**
     * This signal is emitted, if a new floating widget has been created.
     * An application can use this signal to e.g. subscribe to events of
     * the newly created window.
     */
    void floatingWidgetCreated(FloatingDockContainer *floatingWidget);

    /**
     * This signal is emitted, if a new DockArea has been created.
     * An application can use this signal to set custom icons or custom
     * tooltips for the DockArea buttons.
     */
    void dockAreaCreated(DockAreaWidget *dockArea);

    /**
     * This signal is emitted just before removal of the given DockWidget.
     */
    void dockWidgetAboutToBeRemoved(DockWidget *dockWidget);

    /**
     * This signal is emitted if a dock widget has been removed with the remove
     * removeDockWidget() function.
     * If this signal is emitted, the dock widget has been removed from the
     * docking system but it is not deleted yet.
     */
    void dockWidgetRemoved(DockWidget *dockWidget);

    /**
     * This signal is emitted if the focused dock widget changed.
     * Both old and now can be nullptr.
     * The focused dock widget is the one that is highlighted in the GUI
     */
    void focusedDockWidgetChanged(DockWidget *old, DockWidget *now);

public:
    void showWorkspaceMananger();

    // higher level workspace management
    QString activeWorkspace() const;
    QString lastWorkspace() const;
    bool autoRestorLastWorkspace() const;
    QString workspaceFileExtension() const;
    QStringList workspaces();
    QSet<QString> workspacePresets() const;
    QDateTime workspaceDateTime(const QString &workspace) const;
    Utils::FilePath workspaceNameToFilePath(const QString &workspaceName) const;
    QString fileNameToWorkspaceName(const QString &fileName) const;
    QString workspaceNameToFileName(const QString &workspaceName) const;

    bool createWorkspace(const QString &workspace);

    bool openWorkspace(const QString &workspace);
    bool reloadActiveWorkspace();

    bool confirmWorkspaceDelete(const QStringList &workspaces);
    bool deleteWorkspace(const QString &workspace);
    void deleteWorkspaces(const QStringList &workspaces);

    bool cloneWorkspace(const QString &original, const QString &clone);
    bool renameWorkspace(const QString &original, const QString &newName);

    bool resetWorkspacePreset(const QString &workspace);

    bool save();

    bool isWorkspacePreset(const QString &workspace) const;

    void setModeChangeState(bool value);
    bool isModeChangeState() const;

    void importWorkspace(const QString &workspace);
    void exportWorkspace(const QString &target, const QString &workspace);

signals:
    void aboutToUnloadWorkspace(QString workspaceName);
    void aboutToLoadWorkspace(QString workspaceName);
    void workspaceLoaded(QString workspaceName);
    void workspaceReloaded(QString workspaceName);
    void aboutToSaveWorkspace();

private:
    bool write(const QString &workspace, const QByteArray &data, QString *errorString) const;
    bool write(const QString &workspace, const QByteArray &data, QWidget *parent) const;

    QByteArray loadWorkspace(const QString &workspace) const;

    void syncWorkspacePresets();

    void saveStartupWorkspace();
}; // class DockManager

} // namespace ADS

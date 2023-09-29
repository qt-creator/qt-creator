// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "ads_globals.h"
#include "dockcontainerwidget.h"
#include "dockwidget.h"
#include "floatingdockcontainer.h"
#include "workspace.h"

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
QT_END_NAMESPACE

namespace Utils { class QtcSettings; }

namespace ADS {

namespace Constants {
const char DEFAULT_WORKSPACE[] = "Basic.wrk"; // Needs to align with a shipped preset
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
class DockFocusController;
class AutoHideSideBar;
class AutoHideTab;
struct AutoHideTabPrivate;

inline constexpr QStringView workspaceFolderName{u"workspaces"};
inline constexpr QStringView workspaceFileExtension{u"wrk"};
inline constexpr QStringView workspaceOrderFileName{u"order.json"};
inline constexpr QStringView workspaceDisplayNameAttribute{u"displayName"};
inline const int workspaceXmlFormattingIndent = 2;

/**
 * The central dock manager that maintains the complete docking system. With the configuration flags
 * you can globally control the functionality of the docking system. The dock manager uses an
 * internal stylesheet to style its components like splitters, tabs and buttons. If you want to
 * disable this stylesheet because your application uses its own, just call the function for
 * settings the stylesheet with an empty string.
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
    friend class AutoHideDockContainer;
    friend AutoHideSideBar;
    friend AutoHideTab;
    friend AutoHideTabPrivate;

public:
    using Super = DockContainerWidget;

    /**
     * These global configuration flags configure some global dock manager settings.
     * Set the dock manager flags, before you create the dock manager instance.
     */
    enum eConfigFlag {
        ActiveTabHasCloseButton
        = 0x0001, //!< If this flag is set, the active tab in a tab area has a close button
        DockAreaHasCloseButton = 0x0002, //!< If the flag is set each dock area has a close button
        DockAreaCloseButtonClosesTab
        = 0x0004, //!< If the flag is set, the dock area close button closes the active tab, if not set, it closes the complete dock area
        OpaqueSplitterResize = 0x0008, //!< See QSplitter::setOpaqueResize() documentation
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
        DragPreviewIsDynamic
        = 0x0400, ///< If opaque undocking is disabled, this flag defines the behavior of the drag preview window, if this flag is enabled, the preview will be adjusted dynamically to the drop area
        DragPreviewShowsContentPixmap
        = 0x0800, ///< If opaque undocking is disabled, the created drag preview window shows a copy of the content of the dock widget / dock are that is dragged
        DragPreviewHasWindowFrame
        = 0x1000, ///< If opaque undocking is disabled, then this flag configures if the drag preview is frameless or looks like a real window
        AlwaysShowTabs
        = 0x2000, ///< If this option is enabled, the tab of a dock widget is always displayed - even if it is the only visible dock widget in a floating widget.
        DockAreaHasUndockButton = 0x4000, //!< If the flag is set each dock area has an undock button
        DockAreaHasTabsMenuButton
        = 0x8000,  //!< If the flag is set each dock area has a tabs menu button
        DockAreaHideDisabledButtons
        = 0x10000, //!< If the flag is set disabled dock area buttons will not appear on the toolbar at all (enabling them will bring them back)
        DockAreaDynamicTabsMenuButtonVisibility
        = 0x20000, //!< If the flag is set, the tabs menu button will be shown only when it is required - that means, if the tabs are elided. If the tabs are not elided, it is hidden
        FloatingContainerHasWidgetTitle
        = 0x40000, //!< If set, the Floating Widget window title reflects the title of the current dock widget otherwise it displays the title set with `CDockManager::setFloatingContainersTitle` or application name as window title
        FloatingContainerHasWidgetIcon
        = 0x80000, //!< If set, the Floating Widget icon reflects the icon of the current dock widget otherwise it displays application icon
        HideSingleCentralWidgetTitleBar
        = 0x100000, //!< If there is only one single visible dock widget in the main dock container (the dock manager) and if this flag is set, then the titlebar of this dock widget will be hidden
        //!< this only makes sense for non draggable and non floatable widgets and enables the creation of some kind of "central" widget

        FocusHighlighting
        = 0x200000, //!< enables styling of focused dock widget tabs or floating widget titlebar
        EqualSplitOnInsertion
        = 0x400000, ///!< if enabled, the space is equally distributed to all widgets in a  splitter

        MiddleMouseButtonClosesTab
        = 0x2000000, //! If the flag is set, the user can use the mouse middle button to close the tab under the mouse

        DefaultDockAreaButtons
        = DockAreaHasCloseButton | DockAreaHasUndockButton
          | DockAreaHasTabsMenuButton, ///< default configuration of dock area title bar buttons

        DefaultBaseConfig
        = DefaultDockAreaButtons | ActiveTabHasCloseButton | XmlAutoFormattingEnabled
          | FloatingContainerHasWidgetTitle, ///< default base configuration settings

        DefaultOpaqueConfig
        = DefaultBaseConfig | OpaqueSplitterResize
          | DragPreviewShowsContentPixmap, ///< the default configuration for non opaque operations

        DefaultNonOpaqueConfig
        = DefaultBaseConfig
          | DragPreviewShowsContentPixmap, ///< the default configuration for non opaque operations

        NonOpaqueWithWindowFrame
        = DefaultNonOpaqueConfig
          | DragPreviewHasWindowFrame ///< the default configuration for non opaque operations that show a real window with frame
    };
    Q_DECLARE_FLAGS(ConfigFlags, eConfigFlag)

    /**
     * These global configuration flags configure some dock manager auto hide settings.
     * Set the dock manager flags, before you create the dock manager instance.
     */
    enum eAutoHideFlag {
        AutoHideFeatureEnabled = 0x01, //!< enables / disables auto hide feature
        DockAreaHasAutoHideButton
        = 0x02, //!< If the flag is set each dock area has a auto hide menu button
        AutoHideButtonTogglesArea
        = 0x04, //!< If the flag is set, the auto hide button enables auto hiding for all dock widgets in an area, if disabled, only the current dock widget will be toggled
        AutoHideButtonCheckable
        = 0x08, //!< If the flag is set, the auto hide button will be checked and unchecked depending on the auto hide state. Mainly for styling purposes.
        AutoHideSideBarsIconOnly
        = 0x10, ///< show only icons in auto hide side tab - if a tab has no icon, then the text will be shown
        AutoHideShowOnMouseOver
        = 0x20, ///< show the auto hide window on mouse over tab and hide it if mouse leaves auto hide container
        AutoHideCloseButtonCollapsesDock
        = 0x40, ///< Close button of an auto hide container collapses the dock instead of hiding it completely

        DefaultAutoHideConfig
        = AutoHideFeatureEnabled | DockAreaHasAutoHideButton
          | AutoHideCloseButtonCollapsesDock ///< the default configuration for left and right side bars
    };
    Q_DECLARE_FLAGS(AutoHideFlags, eAutoHideFlag)

    /**
     * Default Constructor.
     * If the given parent is a QMainWindow, the dock manager sets itself as the central widget.
     * Before you create any dock widgets, you should properly setup the configuration flags
     * via setConfigFlags().
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
     * This function returns the auto hide configuration flags.
     */
    static AutoHideFlags autoHideConfigFlags();

    /**
     * Sets the global configuration flags for the whole docking system. Call this function before
     * you create the dock manager and before your create the first dock widget.
     */
    static void setConfigFlags(const ConfigFlags flags);

    /**
     * Sets the global configuration flags for the whole docking system. Call this function before
     * you create the dock manager and before your create the first dock widget.
     */
    static void setAutoHideConfigFlags(const AutoHideFlags flags);

    /**
     * Set a certain config flag.
     * \see setConfigFlags()
     */
    static void setConfigFlag(eConfigFlag flag, bool on = true);

    /**
     * Set a certain overlay config flag.
     * \see setConfigFlags()
     */
    static void setAutoHideConfigFlag(eAutoHideFlag flag, bool on = true);

    /**
     * Returns true if the given config flag is set.
     */
    static bool testConfigFlag(eConfigFlag flag);

    /**
     * Returns true if the given overlay config flag is set
     */
    static bool testAutoHideConfigFlag(eAutoHideFlag flag);

    /**
     * Returns the global icon provider.
     * The icon provider enables the use of custom icons in case using styleheets for icons is not
     * an option.
     */
    static IconProvider &iconProvider();

    /**
     * The distance the user needs to move the mouse with the left button hold down before a dock
     * widget start floating.
     */
    static int startDragDistance();

    /**
     * Helper function to set focus depending on the configuration of the FocusStyling flag
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
    void setSettings(Utils::QtcSettings *settings);

    /**
     * Set the path to the workspace presets folder.
     */
    void setWorkspacePresetsPath(const QString &path);

    void initialize();

    /**
     * Adds dockwidget into the given area.
     * If DockAreaWidget is not null, then the area parameter indicates the area into the
     * DockAreaWidget. If DockAreaWidget is null, the Dockwidget will be dropped into the container.
     * If you would like to add a dock widget tabified, then you need to add it to an existing
     * dock area object into the CenterDockWidgetArea. The following code shows this:
     * \code
     * DockManager->addDockWidget(ads::CenterDockWidgetArea, NewDockWidget,
     *         ExisitingDockArea);
     * \endcode
     * \return Returns the dock area widget that contains the new DockWidget
     */
    DockAreaWidget *addDockWidget(DockWidgetArea area,
                                  DockWidget *dockWidget,
                                  DockAreaWidget *dockAreaWidget = nullptr,
                                  int index = -1);

    /**
     * Adds dockwidget into the given container.
     * This allows you to place the dock widget into a container, even if that
     * container does not yet contain a DockAreaWidget.
     * \return Returns the dock area widget that contains the new DockWidget
     */
    DockAreaWidget *addDockWidgetToContainer(DockWidgetArea area,
                                             DockWidget *dockWidget,
                                             DockContainerWidget *dockContainerWidget);

    /**
     * Adds an Auto-Hide widget to the dock manager container pinned to
     * the given side bar location.
     * \return Returns the CAutoHideDockContainer that contains the new DockWidget
     */
    AutoHideDockContainer *addAutoHideDockWidget(SideBarLocation location, DockWidget *dockWidget);

    /**
     * Adds an Auto-Hide widget to the given DockContainerWidget pinned to
     * the given side bar location in this container.
     * \return Returns the CAutoHideDockContainer that contains the new DockWidget
     */
    AutoHideDockContainer *addAutoHideDockWidgetToContainer(
        SideBarLocation location, DockWidget *dockWidget, DockContainerWidget *dockContainerWidget);

    /**
     * This function will add the given Dockwidget to the given dock area as a new tab. If no dock
     * area widget exists for the given area identifier, a new dock area widget is created.
     */
    DockAreaWidget *addDockWidgetTab(DockWidgetArea area, DockWidget *dockWidget);

    /**
     * This function will add the given Dockwidget to the given DockAreaWidget as a new tab.
     */
    DockAreaWidget *addDockWidgetTabToArea(DockWidget *dockWidget,
                                           DockAreaWidget *dockAreaWidget,
                                           int index = -1);

    /**
     * Adds the given DockWidget floating and returns the created FloatingDockContainer instance.
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
     * This function returns a readable reference to the internal dock widgets map so that it is
     * possible to iterate over all dock widgets.
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
    const QList<QPointer<FloatingDockContainer> > floatingWidgets() const;

    /**
     * This function always return 0 because the main window is always behind
     * any floating widget.
     */
    unsigned int zOrderIndex() const override;

    /**
     * Saves the current state of the dockmanger and all its dock widgets into the returned
     * QByteArray. The XmlMode enables / disables the auto formatting for the XmlStreamWriter.
     * If auto formatting is enabled, the output is intended and line wrapped.
     * The XmlMode XmlAutoFormattingDisabled is better if you would like to have a more compact
     * XML output - i.e. for storage in ini files.
     * The version number is stored as part of the data.
     * To restore the saved state, pass the return value and version number to restoreState().
     * \see restoreState()
     */
    QByteArray saveState(const QString &displayName, int version = 0) const;

    /**
     * Restores the state of this dockmanagers dockwidgets.
     * The version number is compared with that stored in state. If they do not match, the
     * dockmanager's state is left unchanged, and this function returns false; otherwise, the state
     * is restored, and this function returns true.
     * \see saveState()
     */
    bool restoreState(const QByteArray &state, int version = 0);

    /**
     * This function returns true between the restoringState() and stateRestored() signals.
     */
    bool isRestoringState() const;

    /**
     * This function returns true, if the DockManager window is restoring from minimized state.
     * The DockManager is in this state starting from the QWindowStateChangeEvent that signals the
     * state change from minimized to normal until endLeavingMinimizedState() function is called.
     */
    bool isLeavingMinimizedState() const;

    bool eventFilter(QObject *obj, QEvent *e) override;

    /**
     * Returns the dock widget that has focus style in the ui or a nullptr if not dock widget is
     * painted focused. If the flag FocusHighlighting is disabled, this function always returns
     * nullptr.
     */
    DockWidget *focusedDockWidget() const;

    /**
     * Returns the sizes of the splitter that contains the dock area. If there is no splitter that
     * contains the area, an empty list will be returned.
     */
    QList<int> splitterSizes(DockAreaWidget *containedArea) const;

    /**
     * Update the sizes of a splitter
     * Programmatically updates the sizes of a given splitter by calling QSplitter::setSizes(). The
     * splitter will be the splitter that contains the supplied dock area widget. If there is not
     * splitter that contains the dock area, or the sizes supplied does not match the number of
     * children of the splitter, this method will have no effect.
     */
    void setSplitterSizes(DockAreaWidget *containedArea, const QList<int> &sizes);

    /**
     * Set a custom title for all FloatingContainer that does not reflect the title of the current
     * dock widget.
     */
    static void setFloatingContainersTitle(const QString &title);

    /**
     * Returns the title used by all FloatingContainer that does not reflect the title of the
     * current dock widget. If not title was set with setFloatingContainersTitle(), it returns
     * QGuiApplication::applicationDisplayName().
     */
    static QString floatingContainersTitle();

    /**
     * This function returns managers central widget or nullptr if no central widget is set.
     */
    DockWidget *centralWidget() const;

    /**
     * Adds dockwidget widget into the central area and marks it as central widget.
     * If central widget is set, it will be the only dock widget
     * that will resize with the dock container. A central widget if not
     * movable, floatable or closable and the titlebar of the central
     * dock area is not visible.
     * If the given widget could be set as central widget, the function returns
     * the created dock area. If the widget could not be set, because there
     * is already a central widget, this function returns a nullptr.
     * To clear the central widget, pass a nullptr to the function.
     * \note Setting a central widget is only possible if no other dock widgets
     * have been registered before. That means, this function should be the
     * first function that you call before you add other dock widgets.
     * \retval != 0 The dock area that contains the central widget
     * \retval nullptr Indicates that the given widget can not be set as central
     *         widget because there is already a central widget.
     */
    DockAreaWidget *setCentralWidget(DockWidget *widget);

    /**
     * Request a focus change to the given dock widget.  This function only has an effect, if the
     * flag DockManager::FocusStyling is enabled.
     */
    void setDockWidgetFocused(DockWidget *dockWidget);

    /**
     * Hide CDockManager and all floating widgets (See Issue #380). Calling regular QWidget::hide()
     * hides the DockManager but not the floating widgets.
     */
    void hideManagerAndFloatingWidgets();

    /**
     * Ends the isRestoringFromMinimizedState
     */
    void endLeavingMinimizedState();

signals:
    /**
     * This signal is emitted if the list of workspaces changed.
     */
    void workspaceListChanged();

    /**
     * This signal is emitted if workspaces have been removed.
     */
    void workspaceRemoved();

    /**
     * This signal is emitted, if the restore function is called, just before the dock manager
     * starts restoring the state. If this function is called, nothing has changed yet.
     */
    void restoringState();

    /**
     * This signal is emitted if the state changed in restoreState.
     * The signal is emitted if the restoreState() function is called or if the openWorkspace()
     * function is called.
     */
    void stateRestored();

    /**
     * This signal is emitted, if the dock manager starts opening a workspace.
     * Opening a workspace may take more than a second if there are many complex widgets. The
     * application may use this signal to show some progress indicator or to change the mouse
     * cursor into a busy cursor.
     */
    void openingWorkspace(const QString &fileName);

    /**
     * This signal is emitted if the dock manager finished opening a workspace.
     */
    void workspaceOpened(const QString &fileName);

    /**
     * This signal is emitted, if a new floating widget has been created. An application can use
     * this signal to e.g. subscribe to events of the newly created window.
     */
    void floatingWidgetCreated(ADS::FloatingDockContainer *floatingWidget);

    /**
     * This signal is emitted, if a new DockArea has been created. An application can use this
     * signal to set custom icons or custom tooltips for the DockArea buttons.
     */
    void dockAreaCreated(ADS::DockAreaWidget *dockArea);

    /**
     * This signal is emitted if a dock widget has been added to this
     * dock manager instance.
     */
    void dockWidgetAdded(ADS::DockWidget *dockWidget);

    /**
     * This signal is emitted just before removal of the given DockWidget.
     */
    void dockWidgetAboutToBeRemoved(ADS::DockWidget *dockWidget);

    /**
     * This signal is emitted if a dock widget has been removed with the remove removeDockWidget()
     * function. If this signal is emitted, the dock widget has been removed from the docking
     * system but it is not deleted yet.
     */
    void dockWidgetRemoved(ADS::DockWidget *dockWidget);

    /**
     * This signal is emitted if the focused dock widget changed. Both old and now can be nullptr.
     * The focused dock widget is the one that is highlighted in the GUI
     */
    void focusedDockWidgetChanged(ADS::DockWidget *old, ADS::DockWidget *now);

protected:
    /**
     * Registers the given floating widget in the internal list of floating widgets
     */
    void registerFloatingWidget(FloatingDockContainer *floatingWidget);

    /**
     * Remove the given floating widget from the list of registered floating widgets
     */
    void removeFloatingWidget(FloatingDockContainer *floatingWidget);

    /**
     * Registers the given dock container widget
     */
    void registerDockContainer(DockContainerWidget *dockContainer);

    /**
     * Remove dock container from the internal list of registered dock containers
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
     * A container needs to call this function if a widget has been dropped into it
     */
    void notifyWidgetOrAreaRelocation(QWidget *droppedWidget);

    /**
     * This function is called, if a floating widget has been dropped into an new position. When
     * this function is called, all dock widgets of the FloatingWidget are already inserted into
     * its new position
     */
    void notifyFloatingWidgetDrop(FloatingDockContainer *floatingWidget);

    /**
     * This function is called, if the given DockWidget has been relocated from the old container
     * ContainerOld to the new container DockWidget->dockContainer()
     */
    void notifyDockWidgetRelocation(DockWidget *dockWidget, DockContainerWidget *containerOld);

    /**
     * This function is called, if the given DockArea has been relocated from the old container
     * ContainerOld to the new container DockArea->dockContainer()
     */
    void notifyDockAreaRelocation(DockAreaWidget *dockArea, DockContainerWidget *containerOld);

    /**
     * Show the floating widgets that has been created floating
     */
    void showEvent(QShowEvent *event) override;

    /**
     * Access for the internal dock focus controller.
     * This function only returns a valid object, if the FocusHighlighting flag is set.
     */
    DockFocusController *dockFocusController() const;

    /**
     * Restore floating widgets hidden by an earlier call to hideManagerAndFloatingWidgets.
     */
    void restoreHiddenFloatingWidgets();

public:
    // Workspace state
    Workspace *activeWorkspace() const;
    QString startupWorkspace() const;
    bool autoRestoreWorkspace() const;
    const QList<Workspace> &workspaces() const;

    // Workspace convenience functions
    int workspaceIndex(const QString &fileName) const;
    bool workspaceExists(const QString &fileName) const;
    Workspace *workspace(const QString &fileName) const;
    Workspace *workspace(int index) const;
    QDateTime workspaceDateTime(const QString &fileName) const;

    bool moveWorkspace(int from, int to);
    bool moveWorkspaceUp(const QString &fileName);
    bool moveWorkspaceDown(const QString &fileName);

    Utils::FilePath workspaceNameToFilePath(const QString &workspaceName) const;

    QString workspaceNameToFileName(const QString &workspaceName) const;

    void uniqueWorkspaceFileName(QString &fileName) const;

    // Workspace management functionality
    void showWorkspaceMananger();

    /**
     * \brief Create a workspace.
     *
     * The display name does not need to be unique, but the file name must be. So this function will
     * generate a file name from the display name so it will not collide with an existing workspace.
     *
     * \param workspace display name of the workspace that will be created
     * \return file name of the created workspace or unexpected
     */
    Utils::expected_str<QString> createWorkspace(const QString &workspace);

    Utils::expected_str<void> openWorkspace(const QString &fileName);
    Utils::expected_str<void> reloadActiveWorkspace();

    /**
     * \brief Deletes a workspace from workspace list and the file from disk.
     */
    bool deleteWorkspace(const QString &fileName);
    void deleteWorkspaces(const QStringList &fileNames);

    /**
     * \brief Clone a workspace.
     *
     * \param originalFileName file name of the workspace that will be cloned
     * \param cloneName display name of cloned workspace
     * \return file name of the cloned workspace or unexpected
     */
    Utils::expected_str<QString> cloneWorkspace(const QString &originalFileName,
                                                const QString &cloneName);

    /**
     * \brief Rename a workspace.
     *
     * \param originalFileName file name of the workspace that will be renamed
     * \param newName new display name
     * \return file name of the renamed workspace or unexpected if rename failed
     */
    Utils::expected_str<QString> renameWorkspace(const QString &originalFileName,
                                                 const QString &newName);

    Utils::expected_str<void> resetWorkspacePreset(const QString &fileName);

    /**
     * \brief Save the currently active workspace.
     */
    Utils::expected_str<void> save();

    void setModeChangeState(bool value);
    bool isModeChangeState() const;

    Utils::expected_str<QString> importWorkspace(const QString &filePath);
    Utils::expected_str<QString> exportWorkspace(const QString &targetFilePath,
                                                 const QString &sourceFileName);

    // Workspace convenience functions
    QStringList loadOrder(const Utils::FilePath &dir) const;
    bool writeOrder() const;
    QList<Workspace> loadWorkspaces(const Utils::FilePath &dir) const;

    Utils::FilePath presetDirectory() const;
    Utils::FilePath userDirectory() const;

    static QByteArray loadFile(const Utils::FilePath &filePath);
    static QString readDisplayName(const Utils::FilePath &filePath);
    static bool writeDisplayName(const Utils::FilePath &filePath, const QString &displayName);

signals:
    void aboutToUnloadWorkspace(QString fileName);
    void aboutToLoadWorkspace(QString fileName);
    void workspaceLoaded(QString fileName);
    void workspaceReloaded(QString fileName);
    void aboutToSaveWorkspace();

private:
    static Utils::expected_str<void> write(const Utils::FilePath &filePath, const QByteArray &data);

    Utils::expected_str<QByteArray> loadWorkspace(const Workspace &workspace) const;

    /**
     * \brief Copy all missing workspace presets over to the local workspace folder.
     */
    void syncWorkspacePresets();
    void prepareWorkspaces();

    void saveStartupWorkspace();
}; // class DockManager

} // namespace ADS

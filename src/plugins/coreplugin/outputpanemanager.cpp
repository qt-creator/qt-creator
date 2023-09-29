// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "outputpanemanager.h"

#include "actionmanager/actioncontainer.h"
#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"
#include "actionmanager/commandbutton.h"
#include "coreplugintr.h"
#include "editormanager/editormanager.h"
#include "editormanager/ieditor.h"
#include "find/optionspopup.h"
#include "findplaceholder.h"
#include "icore.h"
#include "ioutputpane.h"
#include "modemanager.h"
#include "outputpane.h"
#include "statusbarmanager.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/proxyaction.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QFocusEvent>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QPainter>
#include <QStackedWidget>
#include <QStyle>
#include <QTimeLine>
#include <QToolButton>

using namespace Utils;
using namespace Core::Internal;

namespace Core {
namespace Internal {

class OutputPaneData
{
public:
    OutputPaneData(IOutputPane *pane = nullptr) : pane(pane) {}

    IOutputPane *pane = nullptr;
    Id id;
    OutputPaneToggleButton *button = nullptr;
    QAction *action = nullptr;
};

static QVector<OutputPaneData> g_outputPanes;
static bool g_managerConstructed = false; // For debugging reasons.

} // Internal

// OutputPane

IOutputPane::IOutputPane(QObject *parent)
    : QObject(parent),
      m_zoomInButton(new Core::CommandButton),
      m_zoomOutButton(new Core::CommandButton)
{
    // We need all pages first. Ignore latecomers and shout.
    QTC_ASSERT(!g_managerConstructed, return);
    g_outputPanes.append(OutputPaneData(this));

    m_zoomInButton->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    m_zoomInButton->setCommandId(Constants::ZOOM_IN);
    connect(m_zoomInButton, &QToolButton::clicked, this, [this] { emit zoomInRequested(1); });

    m_zoomOutButton->setIcon(Utils::Icons::MINUS_TOOLBAR.icon());
    m_zoomOutButton->setCommandId(Constants::ZOOM_OUT);
    connect(m_zoomOutButton, &QToolButton::clicked, this, [this] { emit zoomOutRequested(1); });
}

IOutputPane::~IOutputPane()
{
    const int i = Utils::indexOf(g_outputPanes, Utils::equal(&OutputPaneData::pane, this));
    QTC_ASSERT(i >= 0, return);
    delete g_outputPanes.at(i).button;
    g_outputPanes.removeAt(i);

    delete m_zoomInButton;
    delete m_zoomOutButton;
}

QList<QWidget *> IOutputPane::toolBarWidgets() const
{
    QList<QWidget *> widgets;
    if (m_filterOutputLineEdit)
        widgets << m_filterOutputLineEdit;
    return widgets << m_zoomInButton << m_zoomOutButton;
}

/*!
    Returns the ID of the output pane.
*/
Id IOutputPane::id() const
{
    return m_id;
}

/*!
    Sets the ID of the output pane to \a id.
    This is used for persisting the visibility state.
*/
void IOutputPane::setId(const Utils::Id &id)
{
    m_id = id;
}

/*!
    Returns the translated display name of the output pane.
*/
QString IOutputPane::displayName() const
{
    return m_displayName;
}

/*!
    Determines the position of the output pane on the status bar and the
    default visibility.
    \sa setPriorityInStatusBar()
*/
int IOutputPane::priorityInStatusBar() const
{
    return m_priority;
}

/*!
    Sets the position of the output pane on the status bar and the default
    visibility to \a priority.
    \list
        \li higher numbers are further to the front
        \li >= 0 are shown in status bar by default
        \li < 0 are not shown in status bar by default
    \endlist
*/
void IOutputPane::setPriorityInStatusBar(int priority)
{
    m_priority = priority;
}

/*!
    Sets the translated display name of the output pane to \a name.
*/
void IOutputPane::setDisplayName(const QString &name)
{
    m_displayName = name;
}

void IOutputPane::visibilityChanged(bool /*visible*/)
{
}

void IOutputPane::setFont(const QFont &font)
{
    emit fontChanged(font);
}

void IOutputPane::setWheelZoomEnabled(bool enabled)
{
    emit wheelZoomEnabledChanged(enabled);
}

void IOutputPane::setupFilterUi(const Key &historyKey)
{
    m_filterOutputLineEdit = new FancyLineEdit;
    m_filterActionRegexp = new QAction(this);
    m_filterActionRegexp->setCheckable(true);
    m_filterActionRegexp->setText(Tr::tr("Use Regular Expressions"));
    connect(m_filterActionRegexp, &QAction::toggled, this, &IOutputPane::setRegularExpressions);
    Core::ActionManager::registerAction(m_filterActionRegexp, filterRegexpActionId());

    m_filterActionCaseSensitive = new QAction(this);
    m_filterActionCaseSensitive->setCheckable(true);
    m_filterActionCaseSensitive->setText(Tr::tr("Case Sensitive"));
    connect(m_filterActionCaseSensitive, &QAction::toggled, this, &IOutputPane::setCaseSensitive);
    Core::ActionManager::registerAction(m_filterActionCaseSensitive,
                                        filterCaseSensitivityActionId());

    m_invertFilterAction = new QAction(this);
    m_invertFilterAction->setCheckable(true);
    m_invertFilterAction->setText(Tr::tr("Show Non-matching Lines"));
    connect(m_invertFilterAction, &QAction::toggled, this, [this] {
        m_invertFilter = m_invertFilterAction->isChecked();
        updateFilter();
    });
    Core::ActionManager::registerAction(m_invertFilterAction, filterInvertedActionId());

    m_filterOutputLineEdit->setPlaceholderText(Tr::tr("Filter output..."));
    m_filterOutputLineEdit->setButtonVisible(FancyLineEdit::Left, true);
    m_filterOutputLineEdit->setButtonIcon(FancyLineEdit::Left, Icons::MAGNIFIER.icon());
    m_filterOutputLineEdit->setFiltering(true);
    m_filterOutputLineEdit->setEnabled(false);
    m_filterOutputLineEdit->setHistoryCompleter(historyKey);
    connect(m_filterOutputLineEdit, &FancyLineEdit::textChanged,
            this, &IOutputPane::updateFilter);
    connect(m_filterOutputLineEdit, &FancyLineEdit::returnPressed,
            this, &IOutputPane::updateFilter);
    connect(m_filterOutputLineEdit, &FancyLineEdit::leftButtonClicked,
            this, &IOutputPane::filterOutputButtonClicked);
}

QString IOutputPane::filterText() const
{
    return m_filterOutputLineEdit->text();
}

void IOutputPane::setFilteringEnabled(bool enable)
{
    m_filterOutputLineEdit->setEnabled(enable);
}

void IOutputPane::setupContext(const char *context, QWidget *widget)
{
    return setupContext(Context(context), widget);
}

void IOutputPane::setupContext(const Context &context, QWidget *widget)
{
    QTC_ASSERT(!m_context, return);
    m_context = new IContext(this);
    m_context->setContext(context);
    m_context->setWidget(widget);
    ICore::addContextObject(m_context);

    const auto zoomInAction = new QAction(this);
    Core::ActionManager::registerAction(zoomInAction, Constants::ZOOM_IN, m_context->context());
    connect(zoomInAction, &QAction::triggered, this, [this] { emit zoomInRequested(1); });
    const auto zoomOutAction = new QAction(this);
    Core::ActionManager::registerAction(zoomOutAction, Constants::ZOOM_OUT, m_context->context());
    connect(zoomOutAction, &QAction::triggered, this, [this] { emit zoomOutRequested(1); });
    const auto resetZoomAction = new QAction(this);
    Core::ActionManager::registerAction(resetZoomAction, Constants::ZOOM_RESET,
                                        m_context->context());
    connect(resetZoomAction, &QAction::triggered, this, &IOutputPane::resetZoomRequested);
}

void IOutputPane::setZoomButtonsEnabled(bool enabled)
{
    m_zoomInButton->setEnabled(enabled);
    m_zoomOutButton->setEnabled(enabled);
}

void IOutputPane::updateFilter()
{
    QTC_ASSERT(false, qDebug() << "updateFilter() needs to get re-implemented");
}

void IOutputPane::filterOutputButtonClicked()
{
    auto popup = new Core::OptionsPopup(m_filterOutputLineEdit,
    {filterRegexpActionId(), filterCaseSensitivityActionId(), filterInvertedActionId()});
    popup->show();
}

void IOutputPane::setRegularExpressions(bool regularExpressions)
{
    m_filterRegexp = regularExpressions;
    updateFilter();
}

Id IOutputPane::filterRegexpActionId() const
{
    return Id("OutputFilter.RegularExpressions").withSuffix(metaObject()->className());
}

Id IOutputPane::filterCaseSensitivityActionId() const
{
    return Id("OutputFilter.CaseSensitive").withSuffix(metaObject()->className());
}

Id IOutputPane::filterInvertedActionId() const
{
    return Id("OutputFilter.Invert").withSuffix(metaObject()->className());
}

void IOutputPane::setCaseSensitive(bool caseSensitive)
{
    m_filterCaseSensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    updateFilter();
}

namespace Internal {

const char outputPaneSettingsKeyC[] = "OutputPaneVisibility";
const char outputPaneIdKeyC[] = "id";
const char outputPaneVisibleKeyC[] = "visible";
const int buttonBorderWidth = 3;

static int numberAreaWidth()
{
    return creatorTheme()->flag(Theme::FlatToolBars) ? 15 : 19;
}

////
// OutputPaneManager
////

static OutputPaneManager *m_instance = nullptr;

void OutputPaneManager::create()
{
   m_instance = new OutputPaneManager;
}

void OutputPaneManager::destroy()
{
    delete m_instance;
    m_instance = nullptr;
}

OutputPaneManager *OutputPaneManager::instance()
{
    return m_instance;
}

void OutputPaneManager::updateStatusButtons(bool visible)
{
    int idx = currentIndex();
    if (idx == -1)
        return;
    QTC_ASSERT(idx < g_outputPanes.size(), return);
    const OutputPaneData &data = g_outputPanes.at(idx);
    QTC_ASSERT(data.button, return);
    data.button->setChecked(visible);
    data.pane->visibilityChanged(visible);
}

void OutputPaneManager::updateMaximizeButton(bool maximized)
{
    if (maximized) {
        static const QIcon icon = Utils::Icons::ARROW_DOWN.icon();
        m_instance->m_minMaxAction->setIcon(icon);
        m_instance->m_minMaxAction->setText(Tr::tr("Minimize"));
    } else {
        static const QIcon icon = Utils::Icons::ARROW_UP.icon();
        m_instance->m_minMaxAction->setIcon(icon);
        m_instance->m_minMaxAction->setText(Tr::tr("Maximize"));
    }
}

// Return shortcut as Alt+<number> or Cmd+<number> if number is a non-zero digit
static QKeySequence paneShortCut(int number)
{
    if (number < 1 || number > 9)
        return QKeySequence();

    const int modifier = HostOsInfo::isMacHost() ? Qt::CTRL : Qt::ALT;
    return QKeySequence(modifier | (Qt::Key_0 + number));
}

OutputPaneManager::OutputPaneManager(QWidget *parent) :
    QWidget(parent),
    m_titleLabel(new QLabel),
    m_manageButton(new OutputPaneManageButton),
    m_outputWidgetPane(new QStackedWidget),
    m_opToolBarWidgets(new QStackedWidget)
{
    setWindowTitle(Tr::tr("Output"));

    m_titleLabel->setContentsMargins(5, 0, 5, 0);

    auto clearAction = new QAction(this);
    clearAction->setIcon(Utils::Icons::CLEAN.icon());
    clearAction->setText(Tr::tr("Clear"));
    connect(clearAction, &QAction::triggered, this, &OutputPaneManager::clearPage);

    m_nextAction = new QAction(this);
    m_nextAction->setIcon(Utils::Icons::ARROW_DOWN_TOOLBAR.icon());
    m_nextAction->setText(Tr::tr("Next Item"));
    connect(m_nextAction, &QAction::triggered, this, &OutputPaneManager::slotNext);

    m_prevAction = new QAction(this);
    m_prevAction->setIcon(Utils::Icons::ARROW_UP_TOOLBAR.icon());
    m_prevAction->setText(Tr::tr("Previous Item"));
    connect(m_prevAction, &QAction::triggered, this, &OutputPaneManager::slotPrev);

    m_minMaxAction = new QAction(this);

    auto closeButton = new QToolButton;
    closeButton->setIcon(Icons::CLOSE_SPLIT_BOTTOM.icon());
    connect(closeButton, &QAbstractButton::clicked, this, &OutputPaneManager::slotHide);

    connect(ICore::instance(), &ICore::saveSettingsRequested, this, &OutputPaneManager::saveSettings);

    auto toolBar = new StyledBar;
    auto clearButton = new QToolButton;
    auto prevToolButton = new QToolButton;
    auto nextToolButton = new QToolButton;
    auto minMaxButton = new QToolButton;

    m_buttonsWidget = new QWidget;
    m_buttonsWidget->setObjectName("OutputPaneButtons"); // used for UI introduction

    using namespace Layouting;
    Row {
        m_titleLabel,
        new StyledSeparator,
        clearButton,
        prevToolButton,
        nextToolButton,
        m_opToolBarWidgets,
        minMaxButton,
        closeButton,
        spacing(0), noMargin(),
    }.attachTo(toolBar);

    Column {
        toolBar,
        m_outputWidgetPane,
        new FindToolBarPlaceHolder(this),
        spacing(0), noMargin(),
    }.attachTo(this);

    Row {
        spacing(creatorTheme()->flag(Theme::FlatToolBars) ? 9 : 4), customMargin({5, 0, 0, 0}),
    }.attachTo(m_buttonsWidget);

    StatusBarManager::addStatusBarWidget(m_buttonsWidget, StatusBarManager::Second);

    ActionContainer *mview = ActionManager::actionContainer(Constants::M_VIEW);

    // Window->Output Panes
    ActionContainer *mpanes = ActionManager::createMenu(Constants::M_VIEW_PANES);
    mview->addMenu(mpanes, Constants::G_VIEW_PANES);
    mpanes->menu()->setTitle(Tr::tr("Out&put"));
    mpanes->appendGroup("Coreplugin.OutputPane.ActionsGroup");
    mpanes->appendGroup("Coreplugin.OutputPane.PanesGroup");

    Command *cmd;

    cmd = ActionManager::registerAction(clearAction, Constants::OUTPUTPANE_CLEAR);
    clearButton->setDefaultAction(
        ProxyAction::proxyActionWithIcon(clearAction, Utils::Icons::CLEAN_TOOLBAR.icon()));
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = ActionManager::registerAction(m_prevAction, "Coreplugin.OutputPane.previtem");
    cmd->setDefaultKeySequence(QKeySequence(Tr::tr("Shift+F6")));
    prevToolButton->setDefaultAction(
        ProxyAction::proxyActionWithIcon(m_prevAction, Utils::Icons::ARROW_UP_TOOLBAR.icon()));
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = ActionManager::registerAction(m_nextAction, "Coreplugin.OutputPane.nextitem");
    nextToolButton->setDefaultAction(
        ProxyAction::proxyActionWithIcon(m_nextAction, Utils::Icons::ARROW_DOWN_TOOLBAR.icon()));
    cmd->setDefaultKeySequence(QKeySequence(Tr::tr("F6")));
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = ActionManager::registerAction(m_minMaxAction, "Coreplugin.OutputPane.minmax");
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Shift+9") : Tr::tr("Alt+Shift+9")));
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setAttribute(Command::CA_UpdateIcon);
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");
    connect(m_minMaxAction, &QAction::triggered, this, &OutputPaneManager::toggleMaximized);
    minMaxButton->setDefaultAction(cmd->action());

    mpanes->addSeparator("Coreplugin.OutputPane.ActionsGroup");
}

void OutputPaneManager::initialize()
{
    ActionContainer *mpanes = ActionManager::actionContainer(Constants::M_VIEW_PANES);
    QFontMetrics titleFm = m_instance->m_titleLabel->fontMetrics();
    int minTitleWidth = 0;

    Utils::sort(g_outputPanes, [](const OutputPaneData &d1, const OutputPaneData &d2) {
        return d1.pane->priorityInStatusBar() > d2.pane->priorityInStatusBar();
    });
    const int n = g_outputPanes.size();

    int shortcutNumber = 1;
    const Id baseId = "QtCreator.Pane.";
    for (int i = 0; i != n; ++i) {
        OutputPaneData &data = g_outputPanes[i];
        IOutputPane *outPane = data.pane;
        const int idx = m_instance->m_outputWidgetPane->addWidget(outPane->outputWidget(m_instance));
        QTC_CHECK(idx == i);

        connect(outPane, &IOutputPane::showPage, m_instance, [idx](int flags) {
            m_instance->showPage(idx, flags);
        });
        connect(outPane, &IOutputPane::hidePage, m_instance, &OutputPaneManager::slotHide);

        connect(outPane, &IOutputPane::togglePage, m_instance, [idx](int flags) {
            if (OutputPanePlaceHolder::isCurrentVisible() && m_instance->currentIndex() == idx)
                m_instance->slotHide();
            else
                m_instance->showPage(idx, flags);
        });

        connect(outPane, &IOutputPane::navigateStateUpdate, m_instance, [idx, outPane] {
            if (m_instance->currentIndex() == idx) {
                m_instance->m_prevAction->setEnabled(outPane->canNavigate()
                                                     && outPane->canPrevious());
                m_instance->m_nextAction->setEnabled(outPane->canNavigate() && outPane->canNext());
            }
        });

        QWidget *toolButtonsContainer = new QWidget(m_instance->m_opToolBarWidgets);
        using namespace Layouting;
        Row toolButtonsRow { spacing(0), noMargin() };
        const QList<QWidget *> toolBarWidgets = outPane->toolBarWidgets();
        for (QWidget *toolButton : toolBarWidgets)
            toolButtonsRow.addItem(toolButton);
        toolButtonsRow.addItem(st);
        toolButtonsRow.attachTo(toolButtonsContainer);

        m_instance->m_opToolBarWidgets->addWidget(toolButtonsContainer);

        minTitleWidth = qMax(minTitleWidth, titleFm.horizontalAdvance(outPane->displayName()));

        data.id = baseId.withSuffix(outPane->id().toString());
        data.action = new QAction(outPane->displayName(), m_instance);
        Command *cmd = ActionManager::registerAction(data.action, data.id);

        mpanes->addAction(cmd, "Coreplugin.OutputPane.PanesGroup");

        cmd->setDefaultKeySequence(paneShortCut(shortcutNumber));
        auto button = new OutputPaneToggleButton(shortcutNumber,
                                                 outPane->displayName(),
                                                 cmd->action());
        data.button = button;
        connect(button, &OutputPaneToggleButton::contextMenuRequested, m_instance, [] {
            m_instance->popupMenu();
        });

        connect(outPane, &IOutputPane::flashButton, button, [button] { button->flash(); });
        connect(outPane,
                &IOutputPane::setBadgeNumber,
                button,
                &OutputPaneToggleButton::setIconBadgeNumber);

        ++shortcutNumber;
        m_instance->m_buttonsWidget->layout()->addWidget(data.button);
        connect(data.button, &QAbstractButton::clicked, m_instance, [i] {
            m_instance->buttonTriggered(i);
        });

        const bool visible = outPane->priorityInStatusBar() >= 0;
        data.button->setVisible(visible);

        connect(data.action, &QAction::triggered, m_instance, [i] {
            m_instance->shortcutTriggered(i);
        });
    }

    m_instance->m_titleLabel->setMinimumWidth(
        minTitleWidth + m_instance->m_titleLabel->contentsMargins().left()
        + m_instance->m_titleLabel->contentsMargins().right());
    const int currentIdx = m_instance->currentIndex();
    if (QTC_GUARD(currentIdx >= 0 && currentIdx < g_outputPanes.size()))
        m_instance->m_titleLabel->setText(g_outputPanes[currentIdx].pane->displayName());
    m_instance->m_buttonsWidget->layout()->addWidget(m_instance->m_manageButton);
    connect(m_instance->m_manageButton,
            &OutputPaneManageButton::menuRequested,
            m_instance,
            &OutputPaneManager::popupMenu);

    m_instance->readSettings();
}

OutputPaneManager::~OutputPaneManager() = default;

void OutputPaneManager::shortcutTriggered(int idx)
{
    IOutputPane *outputPane = g_outputPanes.at(idx).pane;
    // Now check the special case, the output window is already visible,
    // we are already on that page but the outputpane doesn't have focus
    // then just give it focus.
    int current = currentIndex();
    if (OutputPanePlaceHolder::isCurrentVisible() && current == idx) {
        if ((!m_outputWidgetPane->isActiveWindow() || !outputPane->hasFocus())
            && outputPane->canFocus()) {
            outputPane->setFocus();
            ICore::raiseWindow(m_outputWidgetPane);
        } else {
            slotHide();
        }
    } else {
        // Else do the same as clicking on the button does.
        buttonTriggered(idx);
    }
}

int OutputPaneManager::outputPaneHeightSetting()
{
    return m_instance->m_outputPaneHeightSetting;
}

void OutputPaneManager::setOutputPaneHeightSetting(int value)
{
    m_instance->m_outputPaneHeightSetting = value;
}

void OutputPaneManager::toggleMaximized()
{
    OutputPanePlaceHolder *ph = OutputPanePlaceHolder::getCurrent();
    QTC_ASSERT(ph, return);

    if (!ph->isVisible()) // easier than disabling/enabling the action
        return;
    ph->setMaximized(!ph->isMaximized());
}

void OutputPaneManager::buttonTriggered(int idx)
{
    QTC_ASSERT(idx >= 0, return);
    if (idx == currentIndex() && OutputPanePlaceHolder::isCurrentVisible()) {
        // we should toggle and the page is already visible and we are actually closeable
        slotHide();
    } else {
        showPage(idx, IOutputPane::ModeSwitch | IOutputPane::WithFocus);
    }
}

void OutputPaneManager::readSettings()
{
    QtcSettings *settings = ICore::settings();
    int num = settings->beginReadArray(outputPaneSettingsKeyC);
    for (int i = 0; i < num; ++i) {
        settings->setArrayIndex(i);
        Id id = Id::fromSetting(settings->value(outputPaneIdKeyC));
        const int idx = Utils::indexOf(g_outputPanes, Utils::equal(&OutputPaneData::id, id));
        if (idx < 0) // happens for e.g. disabled plugins (with outputpanes) that were loaded before
            continue;
        const bool visible = settings->value(outputPaneVisibleKeyC).toBool();
        g_outputPanes[idx].button->setVisible(visible);
    }
    settings->endArray();

    m_outputPaneHeightSetting
        = settings->value("OutputPanePlaceHolder/Height", 0).toInt();
    const int currentIdx
        = settings->value("OutputPanePlaceHolder/CurrentIndex", 0).toInt();
    if (QTC_GUARD(currentIdx >= 0 && currentIdx < g_outputPanes.size()))
        setCurrentIndex(currentIdx);
}

void OutputPaneManager::slotNext()
{
    int idx = currentIndex();
    ensurePageVisible(idx);
    IOutputPane *out = g_outputPanes.at(idx).pane;
    if (out->canNext())
        out->goToNext();
}

void OutputPaneManager::slotPrev()
{
    int idx = currentIndex();
    ensurePageVisible(idx);
    IOutputPane *out = g_outputPanes.at(idx).pane;
    if (out->canPrevious())
        out->goToPrev();
}

void OutputPaneManager::slotHide()
{
    OutputPanePlaceHolder *ph = OutputPanePlaceHolder::getCurrent();
    if (ph) {
        emit ph->visibilityChangeRequested(false);
        ph->setVisible(false);
        int idx = currentIndex();
        QTC_ASSERT(idx >= 0, return);
        g_outputPanes.at(idx).button->setChecked(false);
        g_outputPanes.at(idx).pane->visibilityChanged(false);
        if (IEditor *editor = EditorManager::currentEditor()) {
            QWidget *w = editor->widget()->focusWidget();
            if (!w)
                w = editor->widget();
            w->setFocus();
        }
    }
}

void OutputPaneManager::ensurePageVisible(int idx)
{
    //int current = currentIndex();
    //if (current != idx)
    //    m_outputWidgetPane->setCurrentIndex(idx);
    setCurrentIndex(idx);
}

void OutputPaneManager::showPage(int idx, int flags)
{
    QTC_ASSERT(idx >= 0, return);
    OutputPanePlaceHolder *ph = OutputPanePlaceHolder::getCurrent();

    if (!ph && flags & IOutputPane::ModeSwitch) {
        // In this mode we don't have a placeholder
        // switch to the output mode and switch the page
        ModeManager::activateMode(Id(Constants::MODE_EDIT));
        ph = OutputPanePlaceHolder::getCurrent();
    }

    bool onlyFlash = !ph
            || (g_outputPanes.at(currentIndex()).pane->hasFocus()
                && !(flags & IOutputPane::WithFocus)
                && idx != currentIndex());

    if (onlyFlash) {
        g_outputPanes.at(idx).button->flash();
    } else {
        emit ph->visibilityChangeRequested(true);
        // make the page visible
        ph->setVisible(true);

        ensurePageVisible(idx);
        IOutputPane *out = g_outputPanes.at(idx).pane;
        if (flags & IOutputPane::WithFocus) {
            if (out->canFocus())
                out->setFocus();
            ICore::raiseWindow(m_outputWidgetPane);
        }

        if (flags & IOutputPane::EnsureSizeHint)
            ph->ensureSizeHintAsMinimum();
    }
}

void OutputPaneManager::focusInEvent(QFocusEvent *e)
{
    if (QWidget *w = m_outputWidgetPane->currentWidget())
        w->setFocus(e->reason());
}

void OutputPaneManager::setCurrentIndex(int idx)
{
    static int lastIndex = -1;

    if (lastIndex != -1) {
        g_outputPanes.at(lastIndex).button->setChecked(false);
        g_outputPanes.at(lastIndex).pane->visibilityChanged(false);
    }

    if (idx != -1) {
        m_outputWidgetPane->setCurrentIndex(idx);
        m_opToolBarWidgets->setCurrentIndex(idx);

        OutputPaneData &data = g_outputPanes[idx];
        IOutputPane *pane = data.pane;
        data.button->show();
        if (OutputPanePlaceHolder::isCurrentVisible())
            pane->visibilityChanged(true);

        bool canNavigate = pane->canNavigate();
        m_prevAction->setEnabled(canNavigate && pane->canPrevious());
        m_nextAction->setEnabled(canNavigate && pane->canNext());
        g_outputPanes.at(idx).button->setChecked(OutputPanePlaceHolder::isCurrentVisible());
        m_titleLabel->setText(pane->displayName());
    }

    lastIndex = idx;
}

void OutputPaneManager::popupMenu()
{
    QMenu menu;
    int idx = 0;
    for (OutputPaneData &data : g_outputPanes) {
        QAction *act = menu.addAction(data.pane->displayName());
        act->setCheckable(true);
        act->setChecked(data.button->isPaneVisible());
        connect(act, &QAction::triggered, this, [this, data, idx] {
            if (data.button->isPaneVisible()) {
                data.pane->visibilityChanged(false);
                data.button->setChecked(false);
                data.button->hide();
            } else {
                showPage(idx, IOutputPane::ModeSwitch);
            }
        });
        ++idx;
    }

    menu.addSeparator();
    QAction *reset = menu.addAction(Tr::tr("Reset to Default"));
    connect(reset, &QAction::triggered, this, [this] {
        for (int i = 0; i < g_outputPanes.size(); ++i) {
            OutputPaneData &data = g_outputPanes[i];
            const bool buttonVisible = data.pane->priorityInStatusBar() >= 0;
            const bool paneVisible = currentIndex() == i
                                     && OutputPanePlaceHolder::isCurrentVisible();
            if (buttonVisible) {
                data.button->setChecked(paneVisible);
                data.button->setVisible(true);
            } else {
                data.button->setChecked(false);
                data.button->hide();
            }
        }
    });

    menu.exec(QCursor::pos());
}

void OutputPaneManager::saveSettings() const
{
    QtcSettings *settings = ICore::settings();
    const int n = g_outputPanes.size();
    settings->beginWriteArray(outputPaneSettingsKeyC, n);
    for (int i = 0; i < n; ++i) {
        const OutputPaneData &data = g_outputPanes.at(i);
        settings->setArrayIndex(i);
        settings->setValue(outputPaneIdKeyC, data.id.toSetting());
        settings->setValue(outputPaneVisibleKeyC, data.button->isPaneVisible());
    }
    settings->endArray();
    int heightSetting = m_outputPaneHeightSetting;
    // update if possible
    if (OutputPanePlaceHolder *curr = OutputPanePlaceHolder::getCurrent())
        heightSetting = curr->nonMaximizedSize();
    settings->setValue("OutputPanePlaceHolder/Height", heightSetting);
    settings->setValue("OutputPanePlaceHolder/CurrentIndex", currentIndex());
}

void OutputPaneManager::clearPage()
{
    int idx = currentIndex();
    if (idx >= 0)
        g_outputPanes.at(idx).pane->clearContents();
}

int OutputPaneManager::currentIndex() const
{
    return m_outputWidgetPane->currentIndex();
}


///////////////////////////////////////////////////////////////////////
//
// OutputPaneToolButton
//
///////////////////////////////////////////////////////////////////////

OutputPaneToggleButton::OutputPaneToggleButton(int number, const QString &text,
                                               QAction *action, QWidget *parent)
    : QToolButton(parent)
    , m_number(QString::number(number))
    , m_text(text)
    , m_action(action)
    , m_flashTimer(new QTimeLine(1000, this))
{
    setFocusPolicy(Qt::NoFocus);
    setCheckable(true);
    QFont fnt = QApplication::font();
    setFont(fnt);
    if (m_action)
        connect(m_action, &QAction::changed, this, &OutputPaneToggleButton::updateToolTip);

    m_flashTimer->setDirection(QTimeLine::Forward);
    m_flashTimer->setEasingCurve(QEasingCurve::SineCurve);
    m_flashTimer->setFrameRange(0, 92);
    auto updateSlot = QOverload<>::of(&QWidget::update);
    connect(m_flashTimer, &QTimeLine::valueChanged, this, updateSlot);
    connect(m_flashTimer, &QTimeLine::finished, this, updateSlot);
    updateToolTip();
}

void OutputPaneToggleButton::updateToolTip()
{
    QTC_ASSERT(m_action, return);
    setToolTip(m_action->toolTip());
}

QSize OutputPaneToggleButton::sizeHint() const
{
    ensurePolished();

    QSize s = fontMetrics().size(Qt::TextSingleLine, m_text);

    // Expand to account for border image
    s.rwidth() += numberAreaWidth() + 1 + buttonBorderWidth + buttonBorderWidth;

    if (!m_badgeNumberLabel.text().isNull())
        s.rwidth() += m_badgeNumberLabel.sizeHint().width() + 1;

    return s;
}

static QRect bgRect(const QRect &widgetRect)
{
    // Removes/compensates the left and right margins of StyleHelper::drawPanelBgRect
    return StyleHelper::toolbarStyle() == StyleHelper::ToolbarStyleCompact
               ? widgetRect : widgetRect.adjusted(-2, 0, 2, 0);
}

void OutputPaneToggleButton::paintEvent(QPaintEvent*)
{
    const QFontMetrics fm = fontMetrics();
    const int baseLine = (height() - fm.height() + 1) / 2 + fm.ascent();
    const int numberWidth = fm.horizontalAdvance(m_number);

    QPainter p(this);

    QStyleOption styleOption;
    styleOption.initFrom(this);
    const bool hovered = !HostOsInfo::isMacHost() && (styleOption.state & QStyle::State_MouseOver);

    if (creatorTheme()->flag(Theme::FlatToolBars)) {
        Theme::Color c = Theme::BackgroundColorDark;

        if (hovered)
            c = Theme::BackgroundColorHover;
        else if (isDown() || isChecked())
            c = Theme::BackgroundColorSelected;

        if (c != Theme::BackgroundColorDark)
            StyleHelper::drawPanelBgRect(&p, bgRect(rect()), creatorTheme()->color(c));
    } else {
        const QImage *image = nullptr;
        if (isDown()) {
            static const QImage pressed(
                        StyleHelper::dpiSpecificImageFile(":/utils/images/panel_button_pressed.png"));
            image = &pressed;
        } else if (isChecked()) {
            if (hovered) {
                static const QImage checkedHover(
                            StyleHelper::dpiSpecificImageFile(":/utils/images/panel_button_checked_hover.png"));
                image = &checkedHover;
            } else {
                static const QImage checked(
                            StyleHelper::dpiSpecificImageFile(":/utils/images/panel_button_checked.png"));
                image = &checked;
            }
        } else {
            if (hovered) {
                static const QImage hover(
                            StyleHelper::dpiSpecificImageFile(":/utils/images/panel_button_hover.png"));
                image = &hover;
            } else {
                static const QImage button(
                            StyleHelper::dpiSpecificImageFile(":/utils/images/panel_button.png"));
                image = &button;
            }
        }
        if (image)
            StyleHelper::drawCornerImage(*image, &p, rect(), numberAreaWidth(), buttonBorderWidth, buttonBorderWidth, buttonBorderWidth);
    }

    if (m_flashTimer->state() == QTimeLine::Running)
    {
        QColor c = creatorTheme()->color(Theme::OutputPaneButtonFlashColor);
        c.setAlpha (m_flashTimer->currentFrame());
        if (creatorTheme()->flag(Theme::FlatToolBars))
            StyleHelper::drawPanelBgRect(&p, bgRect(rect()), c);
        else
            p.fillRect(rect().adjusted(numberAreaWidth(), 1, -1, -1), c);
    }

    p.setFont(font());
    p.setPen(creatorTheme()->color(Theme::OutputPaneToggleButtonTextColorChecked));
    p.drawText((numberAreaWidth() - numberWidth) / 2, baseLine, m_number);
    if (!isChecked())
        p.setPen(creatorTheme()->color(Theme::OutputPaneToggleButtonTextColorUnchecked));
    int leftPart = numberAreaWidth() + buttonBorderWidth;
    int labelWidth = 0;
    if (!m_badgeNumberLabel.text().isEmpty()) {
        const QSize labelSize = m_badgeNumberLabel.sizeHint();
        labelWidth = labelSize.width() + 3;
        m_badgeNumberLabel.paint(&p, width() - labelWidth, (height() - labelSize.height()) / 2, isChecked());
    }
    p.drawText(leftPart, baseLine, fm.elidedText(m_text, Qt::ElideRight, width() - leftPart - 1 - labelWidth));
}

void OutputPaneToggleButton::checkStateSet()
{
    //Stop flashing when button is checked
    QToolButton::checkStateSet();
    m_flashTimer->stop();
}

void OutputPaneToggleButton::flash(int count)
{
    setVisible(true);
    //Start flashing if button is not checked
    if (!isChecked()) {
        m_flashTimer->setLoopCount(count);
        if (m_flashTimer->state() != QTimeLine::Running)
            m_flashTimer->start();
        update();
    }
}

void OutputPaneToggleButton::setIconBadgeNumber(int number)
{
    QString text = (number ? QString::number(number) : QString());
    m_badgeNumberLabel.setText(text);
    updateGeometry();
}

bool OutputPaneToggleButton::isPaneVisible() const
{
    return isVisibleTo(parentWidget());
}

void OutputPaneToggleButton::contextMenuEvent(QContextMenuEvent *)
{
    emit contextMenuRequested();
}

///////////////////////////////////////////////////////////////////////
//
// OutputPaneManageButton
//
///////////////////////////////////////////////////////////////////////

OutputPaneManageButton::OutputPaneManageButton()
{
    setFocusPolicy(Qt::NoFocus);
    setCheckable(true);
    setFixedWidth(StyleHelper::toolbarStyle() == Utils::StyleHelper::ToolbarStyleCompact ? 17 : 21);
    connect(this, &QToolButton::clicked, this, &OutputPaneManageButton::menuRequested);
}

void OutputPaneManageButton::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    if (!creatorTheme()->flag(Theme::FlatToolBars)) {
        static const QImage button(StyleHelper::dpiSpecificImageFile(QStringLiteral(":/utils/images/panel_manage_button.png")));
        StyleHelper::drawCornerImage(button, &p, rect(), buttonBorderWidth, buttonBorderWidth, buttonBorderWidth, buttonBorderWidth);
    }
    QStyle *s = style();
    QStyleOption arrowOpt;
    arrowOpt.initFrom(this);
    constexpr int arrowSize = 8;
    arrowOpt.rect = QRect(0, 0, arrowSize, arrowSize);
    arrowOpt.rect.moveCenter(rect().center());
    arrowOpt.rect.translate(0, -3);
    s->drawPrimitive(QStyle::PE_IndicatorArrowUp, &arrowOpt, &p, this);
    arrowOpt.rect.translate(0, 6);
    s->drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrowOpt, &p, this);
}

void OutputPaneManageButton::contextMenuEvent(QContextMenuEvent *)
{
    emit menuRequested();
}

BadgeLabel::BadgeLabel()
{
    m_font = QApplication::font();
    m_font.setBold(true);
    m_font.setPixelSize(11);
}

void BadgeLabel::paint(QPainter *p, int x, int y, bool isChecked)
{
    const QRectF rect(QRect(QPoint(x, y), m_size));
    p->save();

    p->setBrush(creatorTheme()->color(isChecked? Theme::BadgeLabelBackgroundColorChecked
                                               : Theme::BadgeLabelBackgroundColorUnchecked));
    p->setPen(Qt::NoPen);
    p->setRenderHint(QPainter::Antialiasing, true);
    p->drawRoundedRect(rect, m_padding, m_padding, Qt::AbsoluteSize);

    p->setFont(m_font);
    p->setPen(creatorTheme()->color(isChecked ? Theme::BadgeLabelTextColorChecked
                                              : Theme::BadgeLabelTextColorUnchecked));
    p->drawText(rect, Qt::AlignCenter, m_text);

    p->restore();
}

void BadgeLabel::setText(const QString &text)
{
    m_text = text;
    calculateSize();
}

QString BadgeLabel::text() const
{
    return m_text;
}

QSize BadgeLabel::sizeHint() const
{
    return m_size;
}

void BadgeLabel::calculateSize()
{
    const QFontMetrics fm(m_font);
    m_size = fm.size(Qt::TextSingleLine, m_text);
    m_size.setWidth(m_size.width() + m_padding * 1.5);
    m_size.setHeight(2 * m_padding + 1); // Needs to be uneven for pixel perfect vertical centering in the button
}

} // namespace Internal
} // namespace Core

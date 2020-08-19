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

#include "outputpanemanager.h"
#include "outputpane.h"
#include "findplaceholder.h"

#include "icore.h"
#include "ioutputpane.h"
#include "modemanager.h"
#include "statusbarmanager.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/commandbutton.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/find/optionspopup.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/proxyaction.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QDebug>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFocusEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QStackedWidget>
#include <QToolButton>
#include <QTimeLine>

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
    bool buttonVisible = false;
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
    connect(m_zoomInButton, &QToolButton::clicked, this, [this] { emit zoomIn(1); });

    m_zoomOutButton->setIcon(Utils::Icons::MINUS.icon());
    m_zoomOutButton->setCommandId(Constants::ZOOM_OUT);
    connect(m_zoomOutButton, &QToolButton::clicked, this, [this] { emit zoomOut(1); });
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

void IOutputPane::setFont(const QFont &font)
{
    emit fontChanged(font);
}

void IOutputPane::setWheelZoomEnabled(bool enabled)
{
    emit wheelZoomEnabledChanged(enabled);
}

void IOutputPane::setupFilterUi(const QString &historyKey)
{
    m_filterOutputLineEdit = new FancyLineEdit;
    m_filterActionRegexp = new QAction(this);
    m_filterActionRegexp->setCheckable(true);
    m_filterActionRegexp->setText(tr("Use Regular Expressions"));
    connect(m_filterActionRegexp, &QAction::toggled, this, &IOutputPane::setRegularExpressions);
    Core::ActionManager::registerAction(m_filterActionRegexp, filterRegexpActionId());

    m_filterActionCaseSensitive = new QAction(this);
    m_filterActionCaseSensitive->setCheckable(true);
    m_filterActionCaseSensitive->setText(tr("Case Sensitive"));
    connect(m_filterActionCaseSensitive, &QAction::toggled, this, &IOutputPane::setCaseSensitive);
    Core::ActionManager::registerAction(m_filterActionCaseSensitive,
                                        filterCaseSensitivityActionId());

    m_invertFilterAction = new QAction(this);
    m_invertFilterAction->setCheckable(true);
    m_invertFilterAction->setText(tr("Show Non-matching Lines"));
    connect(m_invertFilterAction, &QAction::toggled, this, [this] {
        m_invertFilter = m_invertFilterAction->isChecked();
        updateFilter();
    });
    Core::ActionManager::registerAction(m_invertFilterAction, filterInvertedActionId());

    m_filterOutputLineEdit->setPlaceholderText(tr("Filter output..."));
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
    QTC_ASSERT(!m_context, return);
    m_context = new IContext(this);
    m_context->setContext(Context(context));
    m_context->setWidget(widget);
    ICore::addContextObject(m_context);

    const auto zoomInAction = new QAction(this);
    Core::ActionManager::registerAction(zoomInAction, Constants::ZOOM_IN, m_context->context());
    connect(zoomInAction, &QAction::triggered, this, [this] { emit zoomIn(1); });
    const auto zoomOutAction = new QAction(this);
    Core::ActionManager::registerAction(zoomOutAction, Constants::ZOOM_OUT, m_context->context());
    connect(zoomOutAction, &QAction::triggered, this, [this] { emit zoomOut(1); });
    const auto resetZoomAction = new QAction(this);
    Core::ActionManager::registerAction(resetZoomAction, Constants::ZOOM_RESET,
                                        m_context->context());
    connect(resetZoomAction, &QAction::triggered, this, &IOutputPane::resetZoom);
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
        m_instance->m_minMaxAction->setIcon(m_instance->m_minimizeIcon);
        m_instance->m_minMaxAction->setText(tr("Minimize Output Pane"));
    } else {
        m_instance->m_minMaxAction->setIcon(m_instance->m_maximizeIcon);
        m_instance->m_minMaxAction->setText(tr("Maximize Output Pane"));
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
    m_closeButton(new QToolButton),
    m_minMaxButton(new QToolButton),
    m_outputWidgetPane(new QStackedWidget),
    m_opToolBarWidgets(new QStackedWidget),
    m_minimizeIcon(Utils::Icons::ARROW_DOWN.icon()),
    m_maximizeIcon(Utils::Icons::ARROW_UP.icon())
{
    setWindowTitle(tr("Output"));

    m_titleLabel->setContentsMargins(5, 0, 5, 0);

    m_clearAction = new QAction(this);
    m_clearAction->setIcon(Utils::Icons::CLEAN.icon());
    m_clearAction->setText(tr("Clear"));
    connect(m_clearAction, &QAction::triggered, this, &OutputPaneManager::clearPage);

    m_nextAction = new QAction(this);
    m_nextAction->setIcon(Utils::Icons::NEXT.icon());
    m_nextAction->setText(tr("Next Item"));
    connect(m_nextAction, &QAction::triggered, this, &OutputPaneManager::slotNext);

    m_prevAction = new QAction(this);
    m_prevAction->setIcon(Utils::Icons::PREV.icon());
    m_prevAction->setText(tr("Previous Item"));
    connect(m_prevAction, &QAction::triggered, this, &OutputPaneManager::slotPrev);

    m_minMaxAction = new QAction(this);
    m_minMaxAction->setIcon(m_maximizeIcon);
    m_minMaxAction->setText(tr("Maximize Output Pane"));

    m_closeButton->setIcon(Icons::CLOSE_SPLIT_BOTTOM.icon());
    connect(m_closeButton, &QAbstractButton::clicked, this, &OutputPaneManager::slotHide);

    connect(ICore::instance(), &ICore::saveSettingsRequested, this, &OutputPaneManager::saveSettings);

    auto mainlayout = new QVBoxLayout;
    mainlayout->setSpacing(0);
    mainlayout->setContentsMargins(0, 0, 0, 0);
    m_toolBar = new StyledBar;
    auto toolLayout = new QHBoxLayout(m_toolBar);
    toolLayout->setContentsMargins(0, 0, 0, 0);
    toolLayout->setSpacing(0);
    toolLayout->addWidget(m_titleLabel);
    toolLayout->addWidget(new StyledSeparator);
    m_clearButton = new QToolButton;
    toolLayout->addWidget(m_clearButton);
    m_prevToolButton = new QToolButton;
    toolLayout->addWidget(m_prevToolButton);
    m_nextToolButton = new QToolButton;
    toolLayout->addWidget(m_nextToolButton);
    toolLayout->addWidget(m_opToolBarWidgets);
    toolLayout->addWidget(m_minMaxButton);
    toolLayout->addWidget(m_closeButton);
    mainlayout->addWidget(m_toolBar);
    mainlayout->addWidget(m_outputWidgetPane, 10);
    mainlayout->addWidget(new FindToolBarPlaceHolder(this));
    setLayout(mainlayout);

    m_buttonsWidget = new QWidget;
    m_buttonsWidget->setObjectName("OutputPaneButtons"); // used for UI introduction
    m_buttonsWidget->setLayout(new QHBoxLayout);
    m_buttonsWidget->layout()->setContentsMargins(5,0,0,0);
    m_buttonsWidget->layout()->setSpacing(
            creatorTheme()->flag(Theme::FlatToolBars) ? 9 : 4);

    StatusBarManager::addStatusBarWidget(m_buttonsWidget, StatusBarManager::Second);

    ActionContainer *mview = ActionManager::actionContainer(Constants::M_VIEW);

    // Window->Output Panes
    ActionContainer *mpanes = ActionManager::createMenu(Constants::M_VIEW_PANES);
    mview->addMenu(mpanes, Constants::G_VIEW_PANES);
    mpanes->menu()->setTitle(tr("Output &Panes"));
    mpanes->appendGroup("Coreplugin.OutputPane.ActionsGroup");
    mpanes->appendGroup("Coreplugin.OutputPane.PanesGroup");

    Command *cmd;

    cmd = ActionManager::registerAction(m_clearAction, "Coreplugin.OutputPane.clear");
    m_clearButton->setDefaultAction(cmd->action());
    m_clearButton->setIcon(Utils::Icons::CLEAN_TOOLBAR.icon());
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = ActionManager::registerAction(m_prevAction, "Coreplugin.OutputPane.previtem");
    cmd->setDefaultKeySequence(QKeySequence(tr("Shift+F6")));
    m_prevToolButton->setDefaultAction(
                ProxyAction::proxyActionWithIcon(cmd->action(), Utils::Icons::PREV_TOOLBAR.icon()));
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = ActionManager::registerAction(m_nextAction, "Coreplugin.OutputPane.nextitem");
    m_nextToolButton->setDefaultAction(
                ProxyAction::proxyActionWithIcon(cmd->action(), Utils::Icons::NEXT_TOOLBAR.icon()));
    cmd->setDefaultKeySequence(QKeySequence(tr("F6")));
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = ActionManager::registerAction(m_minMaxAction, "Coreplugin.OutputPane.minmax");
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Ctrl+Shift+9") : tr("Alt+Shift+9")));
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setAttribute(Command::CA_UpdateIcon);
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");
    connect(m_minMaxAction, &QAction::triggered, this, &OutputPaneManager::toggleMaximized);
    m_minMaxButton->setDefaultAction(cmd->action());

    mpanes->addSeparator("Coreplugin.OutputPane.ActionsGroup");

    QFontMetrics titleFm = m_titleLabel->fontMetrics();
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
        const int idx = m_outputWidgetPane->addWidget(outPane->outputWidget(this));
        QTC_CHECK(idx == i);

        connect(outPane, &IOutputPane::showPage, this, [this, idx](int flags) {
            showPage(idx, flags);
        });
        connect(outPane, &IOutputPane::hidePage, this, &OutputPaneManager::slotHide);

        connect(outPane, &IOutputPane::togglePage, this, [this, idx](int flags) {
            if (OutputPanePlaceHolder::isCurrentVisible() && currentIndex() == idx)
                slotHide();
            else
                showPage(idx, flags);
        });

        connect(outPane, &IOutputPane::navigateStateUpdate, this, [this, idx, outPane] {
            if (currentIndex() == idx) {
                m_prevAction->setEnabled(outPane->canNavigate() && outPane->canPrevious());
                m_nextAction->setEnabled(outPane->canNavigate() && outPane->canNext());
            }
        });

        QWidget *toolButtonsContainer = new QWidget(m_opToolBarWidgets);
        auto toolButtonsLayout = new QHBoxLayout;
        toolButtonsLayout->setContentsMargins(0, 0, 0, 0);
        toolButtonsLayout->setSpacing(0);
        foreach (QWidget *toolButton, outPane->toolBarWidgets())
            toolButtonsLayout->addWidget(toolButton);
        toolButtonsLayout->addStretch(5);
        toolButtonsContainer->setLayout(toolButtonsLayout);

        m_opToolBarWidgets->addWidget(toolButtonsContainer);

        minTitleWidth = qMax(minTitleWidth, titleFm.horizontalAdvance(outPane->displayName()));

        QString suffix = outPane->displayName().simplified();
        suffix.remove(QLatin1Char(' '));
        data.id = baseId.withSuffix(suffix);
        data.action = new QAction(outPane->displayName(), this);
        Command *cmd = ActionManager::registerAction(data.action, data.id);

        mpanes->addAction(cmd, "Coreplugin.OutputPane.PanesGroup");

        cmd->setDefaultKeySequence(paneShortCut(shortcutNumber));
        auto button = new OutputPaneToggleButton(shortcutNumber, outPane->displayName(),
                                                 cmd->action());
        data.button = button;

        connect(outPane, &IOutputPane::flashButton, button, [button] { button->flash(); });
        connect(outPane, &IOutputPane::setBadgeNumber,
                button, &OutputPaneToggleButton::setIconBadgeNumber);

        ++shortcutNumber;
        m_buttonsWidget->layout()->addWidget(data.button);
        connect(data.button, &QAbstractButton::clicked, this, [this, i] {
            buttonTriggered(i);
         });

        bool visible = outPane->priorityInStatusBar() != -1;
        data.button->setVisible(visible);
        data.buttonVisible = visible;

        connect(data.action, &QAction::triggered, this, [this, i] {
            shortcutTriggered(i);
        });
    }

    m_titleLabel->setMinimumWidth(minTitleWidth + m_titleLabel->contentsMargins().left()
                                  + m_titleLabel->contentsMargins().right());
    m_buttonsWidget->layout()->addWidget(m_manageButton);
    connect(m_manageButton, &QAbstractButton::clicked, this, &OutputPaneManager::popupMenu);

    readSettings();
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
    QSettings *settings = ICore::settings();
    int num = settings->beginReadArray(QLatin1String(outputPaneSettingsKeyC));
    for (int i = 0; i < num; ++i) {
        settings->setArrayIndex(i);
        Id id = Id::fromSetting(settings->value(QLatin1String(outputPaneIdKeyC)));
        const int idx = Utils::indexOf(g_outputPanes, Utils::equal(&OutputPaneData::id, id));
        if (idx < 0) // happens for e.g. disabled plugins (with outputpanes) that were loaded before
            continue;
        const bool visible = settings->value(QLatin1String(outputPaneVisibleKeyC)).toBool();
        g_outputPanes[idx].buttonVisible = visible;
        g_outputPanes[idx].button->setVisible(visible);
    }
    settings->endArray();

    m_outputPaneHeightSetting = settings->value(QLatin1String("OutputPanePlaceHolder/Height"), 0).toInt();
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
        data.buttonVisible = true;
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
        act->setChecked(data.buttonVisible);
        act->setData(idx);
        ++idx;
    }
    QAction *result = menu.exec(QCursor::pos());
    if (!result)
        return;
    idx = result->data().toInt();
    QTC_ASSERT(idx >= 0 && idx < g_outputPanes.size(), return);
    OutputPaneData &data = g_outputPanes[idx];
    if (data.buttonVisible) {
        data.pane->visibilityChanged(false);
        data.button->setChecked(false);
        data.button->hide();
        data.buttonVisible = false;
    } else {
        showPage(idx, IOutputPane::ModeSwitch);
    }
}

void OutputPaneManager::saveSettings() const
{
    QSettings *settings = ICore::settings();
    const int n = g_outputPanes.size();
    settings->beginWriteArray(QLatin1String(outputPaneSettingsKeyC), n);
    for (int i = 0; i < n; ++i) {
        const OutputPaneData &data = g_outputPanes.at(i);
        settings->setArrayIndex(i);
        settings->setValue(QLatin1String(outputPaneIdKeyC), data.id.toSetting());
        settings->setValue(QLatin1String(outputPaneVisibleKeyC), data.buttonVisible);
    }
    settings->endArray();
    int heightSetting = m_outputPaneHeightSetting;
    // update if possible
    if (OutputPanePlaceHolder *curr = OutputPanePlaceHolder::getCurrent())
        heightSetting = curr->nonMaximizedSize();
    settings->setValue(QLatin1String("OutputPanePlaceHolder/Height"), heightSetting);
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
            p.fillRect(rect(), creatorTheme()->color(c));
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
        QRect r = creatorTheme()->flag(Theme::FlatToolBars)
                  ? rect() : rect().adjusted(numberAreaWidth(), 1, -1, -1);
        p.fillRect(r, c);
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


///////////////////////////////////////////////////////////////////////
//
// OutputPaneManageButton
//
///////////////////////////////////////////////////////////////////////

OutputPaneManageButton::OutputPaneManageButton()
{
    setFocusPolicy(Qt::NoFocus);
    setCheckable(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

QSize OutputPaneManageButton::sizeHint() const
{
    ensurePolished();
    return QSize(numberAreaWidth(), 16);
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
    arrowOpt.rect = QRect(6, rect().center().y() - 3, 8, 8);
    arrowOpt.rect.translate(0, -3);
    s->drawPrimitive(QStyle::PE_IndicatorArrowUp, &arrowOpt, &p, this);
    arrowOpt.rect.translate(0, 6);
    s->drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrowOpt, &p, this);
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

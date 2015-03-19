/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "outputpanemanager.h"
#include "outputpane.h"
#include "coreconstants.h"
#include "findplaceholder.h"

#include "icore.h"
#include "ioutputpane.h"
#include "modemanager.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

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

namespace Core {
namespace Internal {

static char outputPaneSettingsKeyC[] = "OutputPaneVisibility";
static char outputPaneIdKeyC[] = "id";
static char outputPaneVisibleKeyC[] = "visible";
static const int numberAreaWidth = 19;
static const int buttonBorderWidth = 3;

////
// OutputPaneManager
////

static OutputPaneManager *m_instance = 0;

void OutputPaneManager::create()
{
   m_instance = new OutputPaneManager;
}

void OutputPaneManager::destroy()
{
    delete m_instance;
    m_instance = 0;
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
    QTC_ASSERT(m_panes.size() == m_buttons.size(), return);
    m_buttons.at(idx)->setChecked(visible);
    m_panes.at(idx)->visibilityChanged(visible);
    OutputPanePlaceHolder *ph = OutputPanePlaceHolder::getCurrent();
    m_minMaxAction->setVisible(ph && ph->canMaximizeOrMinimize());
}

OutputPaneManager::OutputPaneManager(QWidget *parent) :
    QWidget(parent),
    m_titleLabel(new QLabel),
    m_manageButton(new OutputPaneManageButton),
    m_closeButton(new QToolButton),
    m_minMaxAction(0),
    m_minMaxButton(new QToolButton),
    m_nextAction(0),
    m_prevAction(0),
    m_outputWidgetPane(new QStackedWidget),
    m_opToolBarWidgets(new QStackedWidget),
    m_minimizeIcon(QLatin1String(":/core/images/arrowdown.png")),
    m_maximizeIcon(QLatin1String(":/core/images/arrowup.png")),
    m_maximised(false),
    m_outputPaneHeight(0)
{
    setWindowTitle(tr("Output"));

    m_titleLabel->setContentsMargins(5, 0, 5, 0);

    m_clearAction = new QAction(this);
    m_clearAction->setIcon(QIcon(QLatin1String(Constants::ICON_CLEAN_PANE)));
    m_clearAction->setText(tr("Clear"));
    connect(m_clearAction, SIGNAL(triggered()), this, SLOT(clearPage()));

    m_nextAction = new QAction(this);
    m_nextAction->setIcon(QIcon(QLatin1String(Constants::ICON_NEXT)));
    m_nextAction->setText(tr("Next Item"));
    connect(m_nextAction, SIGNAL(triggered()), this, SLOT(slotNext()));

    m_prevAction = new QAction(this);
    m_prevAction->setIcon(QIcon(QLatin1String(Constants::ICON_PREV)));
    m_prevAction->setText(tr("Previous Item"));
    connect(m_prevAction, SIGNAL(triggered()), this, SLOT(slotPrev()));

    m_minMaxAction = new QAction(this);
    m_minMaxAction->setIcon(m_maximizeIcon);
    m_minMaxAction->setText(tr("Maximize Output Pane"));

    m_closeButton->setIcon(QIcon(QLatin1String(Constants::ICON_CLOSE_SPLIT_BOTTOM)));
    connect(m_closeButton, SIGNAL(clicked()), this, SLOT(slotHide()));

    connect(ICore::instance(), SIGNAL(saveSettingsRequested()), this, SLOT(saveSettings()));

    QVBoxLayout *mainlayout = new QVBoxLayout;
    mainlayout->setSpacing(0);
    mainlayout->setMargin(0);
    m_toolBar = new StyledBar;
    QHBoxLayout *toolLayout = new QHBoxLayout(m_toolBar);
    toolLayout->setMargin(0);
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
    m_buttonsWidget->setLayout(new QHBoxLayout);
    m_buttonsWidget->layout()->setContentsMargins(5,0,0,0);
    m_buttonsWidget->layout()->setSpacing(4);
}

OutputPaneManager::~OutputPaneManager()
{
}

QWidget *OutputPaneManager::buttonsWidget()
{
    return m_buttonsWidget;
}

// Return shortcut as Ctrl+<number>
static inline int paneShortCut(int number)
{
    const int modifier = HostOsInfo::isMacHost() ? Qt::CTRL : Qt::ALT;
    return modifier | (Qt::Key_0 + number);
}

void OutputPaneManager::init()
{
    ActionContainer *mwindow = ActionManager::actionContainer(Constants::M_WINDOW);

    // Window->Output Panes
    ActionContainer *mpanes = ActionManager::createMenu(Constants::M_WINDOW_PANES);
    mwindow->addMenu(mpanes, Constants::G_WINDOW_PANES);
    mpanes->menu()->setTitle(tr("Output &Panes"));
    mpanes->appendGroup("Coreplugin.OutputPane.ActionsGroup");
    mpanes->appendGroup("Coreplugin.OutputPane.PanesGroup");

    Command *cmd;

    cmd = ActionManager::registerAction(m_clearAction, "Coreplugin.OutputPane.clear");
    m_clearButton->setDefaultAction(cmd->action());
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = ActionManager::registerAction(m_prevAction, "Coreplugin.OutputPane.previtem");
    cmd->setDefaultKeySequence(QKeySequence(tr("Shift+F6")));
    m_prevToolButton->setDefaultAction(cmd->action());
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = ActionManager::registerAction(m_nextAction, "Coreplugin.OutputPane.nextitem");
    m_nextToolButton->setDefaultAction(cmd->action());
    cmd->setDefaultKeySequence(QKeySequence(tr("F6")));
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = ActionManager::registerAction(m_minMaxAction, "Coreplugin.OutputPane.minmax");
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+9") : tr("Alt+9")));
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setAttribute(Command::CA_UpdateIcon);
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");
    connect(m_minMaxAction, SIGNAL(triggered()), this, SLOT(slotMinMax()));
    m_minMaxButton->setDefaultAction(cmd->action());

    mpanes->addSeparator("Coreplugin.OutputPane.ActionsGroup");

    QFontMetrics titleFm = m_titleLabel->fontMetrics();
    int minTitleWidth = 0;

    m_panes = ExtensionSystem::PluginManager::getObjects<IOutputPane>();
    Utils::sort(m_panes, [](IOutputPane *p1, IOutputPane *p2) {
        return p1->priorityInStatusBar() > p2->priorityInStatusBar();
    });
    const int n = m_panes.size();

    int shortcutNumber = 1;
    const Id baseId = "QtCreator.Pane.";
    for (int i = 0; i != n; ++i) {
        IOutputPane *outPane = m_panes.at(i);
        const int idx = m_outputWidgetPane->addWidget(outPane->outputWidget(this));
        QTC_CHECK(idx == i);

        connect(outPane, SIGNAL(showPage(int)), this, SLOT(showPage(int)));
        connect(outPane, SIGNAL(hidePage()), this, SLOT(slotHide()));
        connect(outPane, SIGNAL(togglePage(int)), this, SLOT(togglePage(int)));
        connect(outPane, SIGNAL(navigateStateUpdate()), this, SLOT(updateNavigateState()));
        connect(outPane, SIGNAL(flashButton()), this, SLOT(flashButton()));
        connect(outPane, SIGNAL(setBadgeNumber(int)), this, SLOT(setBadgeNumber(int)));

        QWidget *toolButtonsContainer = new QWidget(m_opToolBarWidgets);
        QHBoxLayout *toolButtonsLayout = new QHBoxLayout;
        toolButtonsLayout->setMargin(0);
        toolButtonsLayout->setSpacing(0);
        foreach (QWidget *toolButton, outPane->toolBarWidgets())
            toolButtonsLayout->addWidget(toolButton);
        toolButtonsLayout->addStretch(5);
        toolButtonsContainer->setLayout(toolButtonsLayout);

        m_opToolBarWidgets->addWidget(toolButtonsContainer);

        minTitleWidth = qMax(minTitleWidth, titleFm.width(outPane->displayName()));

        QString suffix = outPane->displayName().simplified();
        suffix.remove(QLatin1Char(' '));
        const Id id = baseId.withSuffix(suffix);
        QAction *action = new QAction(outPane->displayName(), this);
        Command *cmd = ActionManager::registerAction(action, id);

        mpanes->addAction(cmd, "Coreplugin.OutputPane.PanesGroup");
        m_actions.append(action);
        m_ids.append(id);

        cmd->setDefaultKeySequence(QKeySequence(paneShortCut(shortcutNumber)));
        OutputPaneToggleButton *button = new OutputPaneToggleButton(shortcutNumber, outPane->displayName(),
                                                                    cmd->action());
        ++shortcutNumber;
        m_buttonsWidget->layout()->addWidget(button);
        m_buttons.append(button);
        connect(button, SIGNAL(clicked()), this, SLOT(buttonTriggered()));

        bool visible = outPane->priorityInStatusBar() != -1;
        button->setVisible(visible);
        m_buttonVisibility.insert(id, visible);

        connect(action, SIGNAL(triggered()), this, SLOT(shortcutTriggered()));
    }

    m_titleLabel->setMinimumWidth(minTitleWidth + m_titleLabel->contentsMargins().left()
                                  + m_titleLabel->contentsMargins().right());
    m_buttonsWidget->layout()->addWidget(m_manageButton);
    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(popupMenu()));

    readSettings();
}

void OutputPaneManager::shortcutTriggered()
{
    QAction *action = qobject_cast<QAction*>(sender());
    QTC_ASSERT(action, return);
    int idx = m_actions.indexOf(action);
    QTC_ASSERT(idx != -1, return);
    IOutputPane *outputPane = m_panes.at(idx);
    // Now check the special case, the output window is already visible,
    // we are already on that page but the outputpane doesn't have focus
    // then just give it focus.
    int current = currentIndex();
    if (OutputPanePlaceHolder::isCurrentVisible() && current == idx) {
        if (!outputPane->hasFocus() && outputPane->canFocus()) {
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

bool OutputPaneManager::isMaximized()const
{
    return m_maximised;
}

void OutputPaneManager::slotMinMax()
{
    OutputPanePlaceHolder *ph = OutputPanePlaceHolder::getCurrent();
    QTC_ASSERT(ph, return);

    if (!ph->isVisible()) // easier than disabling/enabling the action
        return;
    m_maximised = !m_maximised;
    ph->maximizeOrMinimize(m_maximised);
    m_minMaxAction->setIcon(m_maximised ? m_minimizeIcon : m_maximizeIcon);
    m_minMaxAction->setText(m_maximised ? tr("Minimize Output Pane")
                                            : tr("Maximize Output Pane"));
}

void OutputPaneManager::buttonTriggered()
{
    OutputPaneToggleButton *button = qobject_cast<OutputPaneToggleButton *>(sender());
    buttonTriggered(m_buttons.indexOf(button));
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
        m_buttonVisibility.insert(Id::fromSetting(settings->value(QLatin1String(outputPaneIdKeyC))),
                          settings->value(QLatin1String(outputPaneVisibleKeyC)).toBool());
    }
    settings->endArray();

    for (int i = 0; i < m_ids.size(); ++i) {
        if (m_buttonVisibility.contains(m_ids.at(i)))
            m_buttons.at(i)->setVisible(m_buttonVisibility.value(m_ids.at(i)));
    }

    m_outputPaneHeight = settings->value(QLatin1String("OutputPanePlaceHolder/Height"), 0).toInt();
}

void OutputPaneManager::slotNext()
{
    int idx = currentIndex();
    ensurePageVisible(idx);
    IOutputPane *out = m_panes.at(idx);
    if (out->canNext())
        out->goToNext();
}

void OutputPaneManager::slotPrev()
{
    int idx = currentIndex();
    ensurePageVisible(idx);
    IOutputPane *out = m_panes.at(idx);
    if (out->canPrevious())
        out->goToPrev();
}

void OutputPaneManager::slotHide()
{
    OutputPanePlaceHolder *ph = OutputPanePlaceHolder::getCurrent();
    if (ph) {
        ph->setVisible(false);
        int idx = currentIndex();
        QTC_ASSERT(idx >= 0, return);
        m_buttons.at(idx)->setChecked(false);
        m_panes.value(idx)->visibilityChanged(false);
        if (IEditor *editor = EditorManager::currentEditor()) {
            QWidget *w = editor->widget()->focusWidget();
            if (!w)
                w = editor->widget();
            w->setFocus();
        }
    }
}

int OutputPaneManager::findIndexForPage(IOutputPane *out)
{
    return m_panes.indexOf(out);
}

void OutputPaneManager::ensurePageVisible(int idx)
{
    //int current = currentIndex();
    //if (current != idx)
    //    m_outputWidgetPane->setCurrentIndex(idx);
    setCurrentIndex(idx);
}

void OutputPaneManager::updateNavigateState()
{
    IOutputPane *pane = qobject_cast<IOutputPane*>(sender());
    int idx = findIndexForPage(pane);
    if (currentIndex() == idx) {
        m_prevAction->setEnabled(pane->canNavigate() && pane->canPrevious());
        m_nextAction->setEnabled(pane->canNavigate() && pane->canNext());
    }
}

void OutputPaneManager::flashButton()
{
    IOutputPane* pane = qobject_cast<IOutputPane*>(sender());
    int idx = findIndexForPage(pane);
    if (pane)
        m_buttons.value(idx)->flash();
}

void OutputPaneManager::setBadgeNumber(int number)
{
    IOutputPane* pane = qobject_cast<IOutputPane*>(sender());
    int idx = findIndexForPage(pane);
    if (pane)
        m_buttons.value(idx)->setIconBadgeNumber(number);
}

// Slot connected to showPage signal of each page
void OutputPaneManager::showPage(int flags)
{
    int idx = findIndexForPage(qobject_cast<IOutputPane*>(sender()));
    showPage(idx, flags);
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
            || (m_panes.at(currentIndex())->hasFocus()
                && !(flags & IOutputPane::WithFocus)
                && idx != currentIndex());

    if (onlyFlash) {
        m_buttons.value(idx)->flash();
    } else {
        // make the page visible
        ph->setVisible(true);

        ensurePageVisible(idx);
        IOutputPane *out = m_panes.at(idx);
        out->visibilityChanged(true);
        if (flags & IOutputPane::WithFocus) {
            if (out->canFocus())
                out->setFocus();
            ICore::raiseWindow(m_outputWidgetPane);
        }

        ph->setDefaultHeight(m_outputPaneHeight);
        if (flags & IOutputPane::EnsureSizeHint)
            ph->ensureSizeHintAsMinimum();
    }
}

void OutputPaneManager::togglePage(int flags)
{
    int idx = findIndexForPage(qobject_cast<IOutputPane*>(sender()));
    if (OutputPanePlaceHolder::isCurrentVisible() && currentIndex() == idx)
         slotHide();
    else
         showPage(idx, flags);
}

void OutputPaneManager::focusInEvent(QFocusEvent *e)
{
    if (QWidget *w = m_outputWidgetPane->currentWidget())
        w->setFocus(e->reason());
}

void OutputPaneManager::resizeEvent(QResizeEvent *e)
{
    if (e->size().height() == 0)
        return;
    m_outputPaneHeight = e->size().height();
}

void OutputPaneManager::setCurrentIndex(int idx)
{
    static int lastIndex = -1;

    if (lastIndex != -1) {
        m_buttons.at(lastIndex)->setChecked(false);
        m_panes.at(lastIndex)->visibilityChanged(false);
    }

    if (idx != -1) {
        m_outputWidgetPane->setCurrentIndex(idx);
        m_opToolBarWidgets->setCurrentIndex(idx);

        IOutputPane *pane = m_panes.at(idx);
        pane->visibilityChanged(true);

        bool canNavigate = pane->canNavigate();
        m_prevAction->setEnabled(canNavigate && pane->canPrevious());
        m_nextAction->setEnabled(canNavigate && pane->canNext());
        m_buttons.at(idx)->setChecked(OutputPanePlaceHolder::isCurrentVisible());
        m_titleLabel->setText(pane->displayName());
    }

    lastIndex = idx;
}

void OutputPaneManager::popupMenu()
{
    QMenu menu;
    int idx = 0;
    foreach (IOutputPane *pane, m_panes) {
        QAction *act = menu.addAction(pane->displayName());
        act->setCheckable(true);
        act->setChecked(m_buttonVisibility.value(m_ids.at(idx)));
        act->setData(idx);
        ++idx;
    }
    QAction *result = menu.exec(QCursor::pos());
    if (!result)
        return;
    idx = result->data().toInt();
    Id id = m_ids.at(idx);
    QTC_ASSERT(idx >= 0 && idx < m_buttons.size(), return);
    QToolButton *button = m_buttons.at(idx);
    if (m_buttonVisibility.value(id)) {
        m_panes.value(idx)->visibilityChanged(false);
        button->setChecked(false);
        button->hide();
        m_buttonVisibility.insert(id, false);
    } else {
        button->show();
        m_buttonVisibility.insert(id, true);
        showPage(idx, IOutputPane::ModeSwitch);
    }
}

void OutputPaneManager::saveSettings() const
{
    QSettings *settings = ICore::settings();
    settings->beginWriteArray(QLatin1String(outputPaneSettingsKeyC),
                              m_ids.size());
    for (int i = 0; i < m_ids.size(); ++i) {
        settings->setArrayIndex(i);
        settings->setValue(QLatin1String(outputPaneIdKeyC), m_ids.at(i).toSetting());
        settings->setValue(QLatin1String(outputPaneVisibleKeyC),
                           m_buttonVisibility.value(m_ids.at(i)));
    }
    settings->endArray();
    settings->setValue(QLatin1String("OutputPanePlaceHolder/Height"), m_outputPaneHeight);
}

void OutputPaneManager::clearPage()
{
    int idx = currentIndex();
    if (idx >= 0)
        m_panes.at(idx)->clearContents();
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
        connect(m_action, SIGNAL(changed()), this, SLOT(updateToolTip()));

    m_flashTimer->setDirection(QTimeLine::Forward);
    m_flashTimer->setCurveShape(QTimeLine::SineCurve);
    m_flashTimer->setFrameRange(0, 92);
    connect(m_flashTimer, SIGNAL(valueChanged(qreal)), this, SLOT(update()));
    connect(m_flashTimer, SIGNAL(finished()), this, SLOT(update()));
}

void OutputPaneToggleButton::updateToolTip()
{
    Q_ASSERT(m_action);
    setToolTip(m_action->toolTip());
}

QSize OutputPaneToggleButton::sizeHint() const
{
    ensurePolished();

    QSize s = fontMetrics().size(Qt::TextSingleLine, m_text);

    // Expand to account for border image
    s.rwidth() += numberAreaWidth + 1 + buttonBorderWidth + buttonBorderWidth;

    if (!m_badgeNumberLabel.text().isNull())
        s.rwidth() += m_badgeNumberLabel.sizeHint().width() + 1;

    return s.expandedTo(QApplication::globalStrut());
}

void OutputPaneToggleButton::paintEvent(QPaintEvent*)
{
    static const QImage panelButton(StyleHelper::dpiSpecificImageFile(QStringLiteral(":/core/images/panel_button.png")));
    static const QImage panelButtonHover(StyleHelper::dpiSpecificImageFile(QStringLiteral(":/core/images/panel_button_hover.png")));
    static const QImage panelButtonPressed(StyleHelper::dpiSpecificImageFile(QStringLiteral(":/core/images/panel_button_pressed.png")));
    static const QImage panelButtonChecked(StyleHelper::dpiSpecificImageFile(QStringLiteral(":/core/images/panel_button_checked.png")));
    static const QImage panelButtonCheckedHover(StyleHelper::dpiSpecificImageFile(QStringLiteral(":/core/images/panel_button_checked_hover.png")));

    const QFontMetrics fm = fontMetrics();
    const int baseLine = (height() - fm.height() + 1) / 2 + fm.ascent();
    const int numberWidth = fm.width(m_number);

    QPainter p(this);

    QStyleOption styleOption;
    styleOption.initFrom(this);
    const bool hovered = !HostOsInfo::isMacHost() && (styleOption.state & QStyle::State_MouseOver);

    const QImage *image = 0;
    if (creatorTheme()->widgetStyle() == Theme::StyleDefault) {
        if (isDown())
            image = &panelButtonPressed;
        else if (isChecked())
            image = hovered ? &panelButtonCheckedHover : &panelButtonChecked;
        else
            image = hovered ? &panelButtonHover : &panelButton;
        if (image)
            StyleHelper::drawCornerImage(*image, &p, rect(), numberAreaWidth, buttonBorderWidth, buttonBorderWidth, buttonBorderWidth);
    } else {
        QColor c;
        if (isChecked()) {
            c = creatorTheme()->color(hovered ? Theme::BackgroundColorHover
                                              : Theme::BackgroundColorSelected);
        } else if (isDown()) {
            c = creatorTheme()->color(Theme::BackgroundColorSelected);
        } else {
            c = creatorTheme()->color(hovered ? Theme::BackgroundColorHover
                                              : Theme::BackgroundColorDark);
        }
        p.fillRect(rect(), c);
    }

    if (m_flashTimer->state() == QTimeLine::Running)
    {
        QColor c = creatorTheme()->color(Theme::OutputPaneButtonFlashColor);
        c.setAlpha (m_flashTimer->currentFrame());
        QRect r = (creatorTheme()->widgetStyle() == Theme::StyleFlat)
                  ? rect() : rect().adjusted(numberAreaWidth, 1, -1, -1);
        p.fillRect(r, c);
    }

    p.setFont(font());
    p.setPen(creatorTheme()->color(Theme::OutputPaneToggleButtonTextColorChecked));
    p.drawText((numberAreaWidth - numberWidth) / 2, baseLine, m_number);
    if (!isChecked())
        p.setPen(creatorTheme()->color(Theme::OutputPaneToggleButtonTextColorUnchecked));
    int leftPart = numberAreaWidth + buttonBorderWidth;
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
}

QSize OutputPaneManageButton::sizeHint() const
{
    ensurePolished();
    return QSize(numberAreaWidth, QApplication::globalStrut().height());
}

void OutputPaneManageButton::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    if (creatorTheme()->widgetStyle() == Theme::StyleDefault) {
        static const QImage button(StyleHelper::dpiSpecificImageFile(QStringLiteral(":/core/images/panel_manage_button.png")));
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

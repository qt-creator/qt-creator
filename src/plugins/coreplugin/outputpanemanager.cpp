/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "outputpanemanager.h"
#include "outputpane.h"
#include "coreconstants.h"
#include "findplaceholder.h"

#include "coreconstants.h"
#include "icore.h"
#include "ioutputpane.h"
#include "mainwindow.h"
#include "modemanager.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/editormanager/ieditor.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/hostosinfo.h>
#include <utils/styledbar.h>
#include <utils/qtcassert.h>

#include <QDebug>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFocusEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QSplitter>
#include <QStyle>
#include <QStackedWidget>
#include <QToolButton>
#include <QTimeLine>
#include <QLabel>

namespace Core {
namespace Internal {

static char outputPaneSettingsKeyC[] = "OutputPaneVisibility";
static char outputPaneIdKeyC[] = "id";
static char outputPaneVisibleKeyC[] = "visible";

////
// OutputPaneManager
////

static OutputPaneManager *m_instance = 0;

bool comparePanes(IOutputPane *p1, IOutputPane *p2)
{
    return p1->priorityInStatusBar() > p2->priorityInStatusBar();
}

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
    m_maximised(false)
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

    m_closeButton->setIcon(QIcon(QLatin1String(Constants::ICON_CLOSE_DOCUMENT)));
    connect(m_closeButton, SIGNAL(clicked()), this, SLOT(slotHide()));

    connect(ICore::instance(), SIGNAL(saveSettingsRequested()), this, SLOT(saveSettings()));

    QVBoxLayout *mainlayout = new QVBoxLayout;
    mainlayout->setSpacing(0);
    mainlayout->setMargin(0);
    m_toolBar = new Utils::StyledBar;
    QHBoxLayout *toolLayout = new QHBoxLayout(m_toolBar);
    toolLayout->setMargin(0);
    toolLayout->setSpacing(0);
    toolLayout->addWidget(m_titleLabel);
    toolLayout->addWidget(new Utils::StyledSeparator);
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
    mainlayout->addWidget(new Core::FindToolBarPlaceHolder(this));
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
    const int modifier = Utils::HostOsInfo::isMacHost() ? Qt::CTRL : Qt::ALT;
    return modifier | (Qt::Key_0 + number);
}

void OutputPaneManager::init()
{
    ActionContainer *mwindow = ActionManager::actionContainer(Constants::M_WINDOW);
    const Context globalContext(Constants::C_GLOBAL);

    // Window->Output Panes
    ActionContainer *mpanes = ActionManager::createMenu(Constants::M_WINDOW_PANES);
    mwindow->addMenu(mpanes, Constants::G_WINDOW_PANES);
    mpanes->menu()->setTitle(tr("Output &Panes"));
    mpanes->appendGroup("Coreplugin.OutputPane.ActionsGroup");
    mpanes->appendGroup("Coreplugin.OutputPane.PanesGroup");

    Command *cmd;

    cmd = ActionManager::registerAction(m_clearAction, "Coreplugin.OutputPane.clear", globalContext);
    m_clearButton->setDefaultAction(cmd->action());
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = ActionManager::registerAction(m_prevAction, "Coreplugin.OutputPane.previtem", globalContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Shift+F6")));
    m_prevToolButton->setDefaultAction(cmd->action());
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = ActionManager::registerAction(m_nextAction, "Coreplugin.OutputPane.nextitem", globalContext);
    m_nextToolButton->setDefaultAction(cmd->action());
    cmd->setDefaultKeySequence(QKeySequence(tr("F6")));
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = ActionManager::registerAction(m_minMaxAction, "Coreplugin.OutputPane.minmax", globalContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+9") : tr("Alt+9")));
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setAttribute(Command::CA_UpdateIcon);
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");
    connect(m_minMaxAction, SIGNAL(triggered()), this, SLOT(slotMinMax()));
    m_minMaxButton->setDefaultAction(cmd->action());

    mpanes->addSeparator(globalContext, "Coreplugin.OutputPane.ActionsGroup");

    QFontMetrics titleFm = m_titleLabel->fontMetrics();
    int minTitleWidth = 0;

    m_panes = ExtensionSystem::PluginManager::getObjects<IOutputPane>();
    qSort(m_panes.begin(), m_panes.end(), &comparePanes);
    const int n = m_panes.size();

    int shortcutNumber = 1;
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

        QString actionId = QLatin1String("QtCreator.Pane.") + outPane->displayName().simplified();
        actionId.remove(QLatin1Char(' '));
        Id id = Id::fromString(actionId);
        QAction *action = new QAction(outPane->displayName(), this);
        Command *cmd = ActionManager::registerAction(action, id, globalContext);

        mpanes->addAction(cmd, "Coreplugin.OutputPane.PanesGroup");
        m_actions.append(cmd->action());
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

        connect(cmd->action(), SIGNAL(triggered()), this, SLOT(shortcutTriggered()));
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
    Core::IOutputPane *outputPane = m_panes.at(idx);
    // Now check the special case, the output window is already visible,
    // we are already on that page but the outputpane doesn't have focus
    // then just give it focus.
    int current = currentIndex();
    if (OutputPanePlaceHolder::isCurrentVisible() && current == idx) {
        if (!outputPane->hasFocus() && outputPane->canFocus())
            outputPane->setFocus();
        else
            slotHide();
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
    QMap<QString, bool> visibility;
    QSettings *settings = ICore::settings();
    int num = settings->beginReadArray(QLatin1String(outputPaneSettingsKeyC));
    for (int i = 0; i < num; ++i) {
        settings->setArrayIndex(i);
        visibility.insert(settings->value(QLatin1String(outputPaneIdKeyC)).toString(),
                          settings->value(QLatin1String(outputPaneVisibleKeyC)).toBool());
    }
    settings->endArray();

    for (int i = 0; i < m_ids.size(); ++i) {
        if (visibility.contains(m_ids.at(i).toString()))
            m_buttons.at(i)->setVisible(visibility.value(m_ids.at(i).toString()));
    }
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
        if (IEditor *editor = Core::EditorManager::currentEditor()) {
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

    if (ph) {
        // make the page visible
        ph->setVisible(true);
        ensurePageVisible(idx);
        IOutputPane *out = m_panes.at(idx);
        out->visibilityChanged(true);
        if (flags & IOutputPane::WithFocus && out->canFocus())
            out->setFocus();

        if (flags & IOutputPane::EnsureSizeHint)
            ph->ensureSizeHintAsMinimum();
    } else {
        m_buttons.value(idx)->flash();
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
        act->setChecked(m_buttons.at(idx)->isVisible());
        act->setData(idx);
        ++idx;
    }
    QAction *result = menu.exec(QCursor::pos());
    if (!result)
        return;
    idx = result->data().toInt();
    QTC_ASSERT(idx >= 0 && idx < m_buttons.size(), return);
    QToolButton *button = m_buttons.at(idx);
    if (button->isVisible()) {
        m_panes.value(idx)->visibilityChanged(false);
        button->setChecked(false);
        button->hide();
    } else {
        button->show();
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
        settings->setValue(QLatin1String(outputPaneIdKeyC), m_ids.at(i).toString());
        settings->setValue(QLatin1String(outputPaneVisibleKeyC), m_buttons.at(i)->isVisible());
    }
    settings->endArray();
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

static QString buttonStyleSheet()
{
    return QLatin1String("QToolButton { border-image: url(:/core/images/panel_button.png) 2 2 2 19;"
                         " border-width: 2px 2px 2px 19px; padding-left: -17; padding-right: 4 } "
            "QToolButton:checked { border-image: url(:/core/images/panel_button_checked.png) 2 2 2 19 } "
            "QToolButton::menu-indicator { width:0; height:0 }"
#ifndef Q_OS_MAC // Mac UIs usually don't hover
            "QToolButton:checked:hover { border-image: url(:/core/images/panel_button_checked_hover.png) 2 2 2 19 } "
            "QToolButton:pressed:hover { border-image: url(:/core/images/panel_button_pressed.png) 2 2 2 19 } "
            "QToolButton:hover { border-image: url(:/core/images/panel_button_hover.png) 2 2 2 19 } "
#endif
            );
}

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
    setStyleSheet(buttonStyleSheet());
    if (m_action)
        connect(m_action, SIGNAL(changed()), this, SLOT(updateToolTip()));

    m_flashTimer->setDirection(QTimeLine::Forward);
    m_flashTimer->setCurveShape(QTimeLine::SineCurve);
    m_flashTimer->setFrameRange(0, 92);
    connect(m_flashTimer, SIGNAL(valueChanged(qreal)), this, SLOT(update()));
    connect(m_flashTimer, SIGNAL(finished()), this, SLOT(update()));

    m_label = new QLabel(this);
    fnt.setBold(true);
    fnt.setPixelSize(11);
    m_label->setFont(fnt);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setStyleSheet(QLatin1String("background-color: #818181; color: white; border-radius: 6; padding-left: 4; padding-right: 4;"));
    m_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_label->hide();
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

    // Expand to account for border image set by stylesheet above
    s.rwidth() += 19 + 5 + 2;
    s.rheight() += 2 + 2;

    if (!m_label->text().isNull())
        s.rwidth() += m_label->width();

    return s.expandedTo(QApplication::globalStrut());
}

void OutputPaneToggleButton::resizeEvent(QResizeEvent *event)
{
    QToolButton::resizeEvent(event);
    if (!m_label->text().isNull()) {
        m_label->move(width() - m_label->width() - 3,  (height() - m_label->height() + 1) / 2);
        m_label->show();
    }
}

void OutputPaneToggleButton::paintEvent(QPaintEvent *event)
{
    // For drawing the style sheet stuff
    QToolButton::paintEvent(event);

    const QFontMetrics fm = fontMetrics();
    const int baseLine = (height() - fm.height() + 1) / 2 + fm.ascent();
    const int numberWidth = fm.width(m_number);

    QPainter p(this);
    if (m_flashTimer->state() == QTimeLine::Running) {
        p.setPen(Qt::transparent);
        p.fillRect(rect().adjusted(19, 1, -1, -1), QBrush(QColor(255,0,0, m_flashTimer->currentFrame())));
    }
    p.setFont(font());
    p.setPen(Qt::white);
    p.drawText((20 - numberWidth) / 2, baseLine, m_number);
    if (!isChecked())
        p.setPen(Qt::black);
    int leftPart = 22;
    int labelWidth = m_label->isVisible() ? m_label->width() + 3 : 0;
    p.drawText(leftPart, baseLine, fm.elidedText(m_text, Qt::ElideRight, width() - leftPart - 1 - labelWidth));
}

void OutputPaneToggleButton::checkStateSet()
{
    //Stop flashing when button is checked
    QToolButton::checkStateSet();
    m_flashTimer->stop();

    if (isChecked())
        m_label->setStyleSheet(QLatin1String("background-color: #e1e1e1; color: #606060; "
                                             "border-radius: 6; padding-left: 4; padding-right: 4;"));
    else
        m_label->setStyleSheet(QLatin1String("background-color: #818181; color: white; border-radius: 6; "
                                             "padding-left: 4; padding-right: 4;"));
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
    if (number) {
        const QString text = QString::number(number);
        m_label->setText(text);

        QSize size = m_label->sizeHint();
        if (size.width() < size.height())
            //Ensure we increase size by an even number of pixels
            size.setWidth(size.height() + ((size.width() - size.height()) & 1));
        m_label->resize(size);

        //Do not show yet, we wait until the button has been resized
    } else {
        m_label->setText(QString());
        m_label->hide();
    }
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
    setStyleSheet(QLatin1String("QToolButton { border-image: url(:/core/images/panel_manage_button.png) 2 2 2 2;"
                                " border-width: 2px 2px 2px 2px } "
                                "QToolButton::menu-indicator { width:0; height:0 }"));
}

QSize OutputPaneManageButton::sizeHint() const
{
    ensurePolished();
    return QSize(18, QApplication::globalStrut().height());
}

void OutputPaneManageButton::paintEvent(QPaintEvent *event)
{
    QToolButton::paintEvent(event); // Draw style sheet.
    QPainter p(this);
    QStyle *s = style();
    QStyleOption arrowOpt;
    arrowOpt.initFrom(this);
    arrowOpt.rect = QRect(5, rect().center().y() - 3, 9, 9);
    arrowOpt.rect.translate(0, -3);
    s->drawPrimitive(QStyle::PE_IndicatorArrowUp, &arrowOpt, &p, this);
    arrowOpt.rect.translate(0, 6);
    s->drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrowOpt, &p, this);
}

} // namespace Internal
} // namespace Core



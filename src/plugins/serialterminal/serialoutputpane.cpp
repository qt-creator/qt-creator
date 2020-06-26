/****************************************************************************
**
** Copyright (C) 2018 Benjamin Balga
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

#include "serialoutputpane.h"

#include "consolelineedit.h"
#include "serialcontrol.h"
#include "serialterminalconstants.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/outputwindow.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/algorithm.h>
#include <utils/icon.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QComboBox>
#include <QLoggingCategory>
#include <QMenu>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace SerialTerminal {
namespace Internal {

static Q_LOGGING_CATEGORY(log, Constants::LOGGING_CATEGORY, QtWarningMsg)

// Tab Widget helper for middle click tab close
class TabWidget : public QTabWidget
{
    Q_OBJECT
public:
    explicit TabWidget(QWidget *parent = nullptr);
signals:
    void contextMenuRequested(const QPoint &pos, int index);
protected:
    bool eventFilter(QObject *object, QEvent *event) final;
private:
    int m_tabIndexForMiddleClick = -1;
};

TabWidget::TabWidget(QWidget *parent) :
    QTabWidget(parent)
{
    tabBar()->installEventFilter(this);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            [this](const QPoint &pos) {
        emit contextMenuRequested(pos, tabBar()->tabAt(pos));
    });
}

bool TabWidget::eventFilter(QObject *object, QEvent *event)
{
    if (object == tabBar()) {
        if (event->type() == QEvent::MouseButtonPress) {
            const auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::MiddleButton) {
                m_tabIndexForMiddleClick = tabBar()->tabAt(me->pos());
                event->accept();
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            const auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::MiddleButton) {
                int tabIndex = tabBar()->tabAt(me->pos());
                if (tabIndex != -1 && tabIndex == m_tabIndexForMiddleClick)
                    emit tabCloseRequested(tabIndex);
                m_tabIndexForMiddleClick = -1;
                event->accept();
                return true;
            }
        }
    }
    return QTabWidget::eventFilter(object, event);
}

// QComboBox with a signal emitted before showPopup() to update on opening
class ComboBox : public QComboBox
{
    Q_OBJECT
public:
    void showPopup() final;
signals:
    void opened();
};

void ComboBox::showPopup()
{
    emit opened();
    QComboBox::showPopup();
}

SerialOutputPane::SerialControlTab::SerialControlTab(SerialControl *serialControl, Core::OutputWindow *w) :
    serialControl(serialControl), window(w)
{}

SerialOutputPane::SerialOutputPane(Settings &settings) :
    m_mainWidget(new QWidget),
    m_inputLine(new ConsoleLineEdit),
    m_tabWidget(new TabWidget),
    m_settings(settings),
    m_devicesModel(new SerialDeviceModel),
    m_closeCurrentTabAction(new QAction(tr("Close Tab"), this)),
    m_closeAllTabsAction(new QAction(tr("Close All Tabs"), this)),
    m_closeOtherTabsAction(new QAction(tr("Close Other Tabs"), this))
{
    createToolButtons();

    auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
            this, [this](int index) { closeTab(index); });
    layout->addWidget(m_tabWidget);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &SerialOutputPane::tabChanged);
    connect(m_tabWidget, &TabWidget::contextMenuRequested,
            this, &SerialOutputPane::contextMenuRequested);

    auto inputLayout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    m_inputLine->setPlaceholderText(tr("Type text and hit Enter to send."));
    inputLayout->addWidget(m_inputLine);

    connect(m_inputLine, &QLineEdit::returnPressed, this, &SerialOutputPane::sendInput);

    m_lineEndingsSelection = new QComboBox;
    updateLineEndingsComboBox();
    inputLayout->addWidget(m_lineEndingsSelection);

    connect(m_lineEndingsSelection, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SerialOutputPane::defaultLineEndingChanged);

    layout->addLayout(inputLayout);

    m_mainWidget->setLayout(layout);

    enableDefaultButtons();
}

QWidget *SerialOutputPane::outputWidget(QWidget *parent)
{
    Q_UNUSED(parent)
    return m_mainWidget.get();
}

QList<QWidget *> SerialOutputPane::toolBarWidgets() const
{
    return { m_newButton,
                m_portsSelection, m_baudRateSelection,
                m_connectButton, m_disconnectButton,
                m_resetButton };
}

QString SerialOutputPane::displayName() const
{
    return tr(Constants::OUTPUT_PANE_TITLE);
}

int SerialOutputPane::priorityInStatusBar() const
{
    return 30;
}

void SerialOutputPane::clearContents()
{
    auto currentWindow = qobject_cast<Core::OutputWindow *>(m_tabWidget->currentWidget());
    if (currentWindow)
        currentWindow->clear();
}

void SerialOutputPane::visibilityChanged(bool)
{
    // Unused but pure virtual
}

bool SerialOutputPane::canFocus() const
{
    return m_tabWidget->currentWidget();
}

bool SerialOutputPane::hasFocus() const
{
    const QWidget *widget = m_tabWidget->currentWidget();
    return widget ? widget->window()->focusWidget() == widget : false;
}

void SerialOutputPane::setFocus()
{
    if (m_tabWidget->currentWidget())
        m_tabWidget->currentWidget()->setFocus();
}

bool SerialOutputPane::canNext() const
{
    return false;
}

bool SerialOutputPane::canPrevious() const
{
    return false;
}

void SerialOutputPane::goToNext()
{
    // Unused but pure virtual
}

void SerialOutputPane::goToPrev()
{
    // Unused but pure virtual
}

bool SerialOutputPane::canNavigate() const
{
    return false;
}

void SerialOutputPane::appendMessage(SerialControl *rc, const QString &out, Utils::OutputFormat format)
{
    const int index = indexOf(rc);
    if (index != -1) {
        Core::OutputWindow *window = m_serialControlTabs.at(index).window;
        window->appendMessage(out, format);
        if (format != Utils::NormalMessageFormat) {
            if (m_serialControlTabs.at(index).behaviorOnOutput == Flash)
                flash();
            else
                popup(NoModeSwitch);
        }
    }
}

void SerialOutputPane::setSettings(const Settings &settings)
{
    m_settings = settings;
}

void SerialOutputPane::createNewOutputWindow(SerialControl *rc)
{
    if (!rc)
        return;

    // Signals to update buttons
    connect(rc, &SerialControl::started,
            [this, rc]() {
        if (isCurrent(rc))
            enableButtons(rc, true);
    });

    connect(rc, &SerialControl::finished,
            [this, rc]() {
        const int tabIndex = indexOf(rc);
        if (tabIndex != -1)
            m_serialControlTabs[tabIndex].window->flush();
        if (isCurrent(rc))
            enableButtons(rc, false);
    });

    connect(rc, &SerialControl::appendMessageRequested,
            this, &SerialOutputPane::appendMessage);

    // Create new
    static int counter = 0;
    Utils::Id contextId = Utils::Id(Constants::C_SERIAL_OUTPUT).withSuffix(counter++);
    Core::Context context(contextId);
    auto ow = new Core::OutputWindow(context, QString(), m_tabWidget);
    using TextEditor::TextEditorSettings;
    auto fontSettingsChanged = [ow] {
        ow->setBaseFont(TextEditorSettings::fontSettings().font());
    };

    connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
            this, fontSettingsChanged);
    fontSettingsChanged();
    ow->setWindowTitle(tr("Serial Terminal Window"));
    // TODO: wordwrap, maxLineCount, zoom/wheelZoom (add to settings)

    auto controlTab = SerialControlTab(rc, ow);
    controlTab.lineEndingIndex = m_settings.defaultLineEndingIndex;
    controlTab.lineEnd = m_settings.defaultLineEnding();

    m_serialControlTabs.push_back(controlTab);
    m_tabWidget->addTab(ow, rc->displayName());
    m_tabWidget->setCurrentIndex(m_tabWidget->count()-1); // Focus new tab

    qCDebug(log) << "Adding tab for " << rc;

    updateCloseActions();
}

bool SerialOutputPane::closeTabs(CloseTabMode mode)
{
    bool allClosed = true;
    for (int t = m_tabWidget->count() - 1; t >= 0; t--) {
        if (!closeTab(t, mode))
            allClosed = false;
    }

    qCDebug(log) << "all tabs closed: " << allClosed;

    return allClosed;
}

void SerialOutputPane::createToolButtons()
{
    // TODO: add actions for keyboard shortcuts?

    // Connect button
    m_connectButton = new QToolButton;
    m_connectButton->setIcon(Utils::Icons::RUN_SMALL_TOOLBAR.icon());
    m_connectButton->setToolTip(tr("Connect"));
    m_connectButton->setEnabled(false);
    connect(m_connectButton, &QToolButton::clicked,
            this, &SerialOutputPane::connectControl);

    // Disconnect button
    m_disconnectButton = new QToolButton;
    m_disconnectButton->setIcon(Utils::Icons::STOP_SMALL_TOOLBAR.icon());
    m_disconnectButton->setToolTip(tr("Disconnect"));
    m_disconnectButton->setEnabled(false);

    connect(m_disconnectButton, &QToolButton::clicked,
            this, &SerialOutputPane::disconnectControl);

    // Reset button
    m_resetButton = new QToolButton;
    m_resetButton->setIcon(Utils::Icons::RELOAD_TOOLBAR.icon());
    m_resetButton->setToolTip(tr("Reset Board"));
    m_resetButton->setEnabled(false);

    connect(m_resetButton, &QToolButton::clicked,
            this, &SerialOutputPane::resetControl);

    // New terminal button
    m_newButton = new QToolButton;
    m_newButton->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    m_newButton->setToolTip(tr("Add New Terminal"));
    m_newButton->setEnabled(true);

    connect(m_newButton, &QToolButton::clicked,
            this, &SerialOutputPane::openNewTerminalControl);

    // Available devices box
    m_portsSelection = new ComboBox;
    m_portsSelection->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_portsSelection->setModel(m_devicesModel);
    updatePortsList();
    connect(m_portsSelection, &ComboBox::opened, this, &SerialOutputPane::updatePortsList);
    connect(m_portsSelection, QOverload<int>::of(&ComboBox::currentIndexChanged),
            this, &SerialOutputPane::activePortNameChanged);
    // TODO: the ports are not updated with the box opened (if the user wait for it) -> add a timer?

    // Baud rates box
    // TODO: add only most common bauds and custom field
    m_baudRateSelection = new ComboBox;
    m_baudRateSelection->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_baudRateSelection->addItems(m_devicesModel->baudRates());
    connect(m_baudRateSelection, QOverload<int>::of(&ComboBox::currentIndexChanged),
            this, &SerialOutputPane::activeBaudRateChanged);

    if (m_settings.baudRate > 0)
        m_baudRateSelection->setCurrentIndex(m_devicesModel->indexForBaudRate(m_settings.baudRate));
    else // Set a default baudrate
        m_baudRateSelection->setCurrentIndex(m_devicesModel->indexForBaudRate(115200));
}

void SerialOutputPane::updateLineEndingsComboBox()
{
    m_lineEndingsSelection->clear();
    for (auto &value : m_settings.lineEndings)
        m_lineEndingsSelection->addItem(value.first, value.second);

    m_lineEndingsSelection->setCurrentIndex(m_settings.defaultLineEndingIndex);
}

void SerialOutputPane::updatePortsList()
{
    m_devicesModel->update();
    m_portsSelection->setCurrentIndex(m_devicesModel->indexForPort(m_settings.portName));
}

int SerialOutputPane::indexOf(const SerialControl *rc) const
{
    return Utils::indexOf(m_serialControlTabs, [rc](const SerialControlTab &tab) {
        return tab.serialControl == rc;
    });
}

int SerialOutputPane::indexOf(const QWidget *outputWindow) const
{
    return Utils::indexOf(m_serialControlTabs, [outputWindow](const SerialControlTab &tab) {
        return tab.window == outputWindow;
    });
}

int SerialOutputPane::currentIndex() const
{
    if (const QWidget *w = m_tabWidget->currentWidget())
        return indexOf(w);
    return -1;
}

SerialControl *SerialOutputPane::currentSerialControl() const
{
    const int index = currentIndex();
    if (index != -1)
        return m_serialControlTabs.at(index).serialControl;
    return nullptr;
}

bool SerialOutputPane::isCurrent(const SerialControl *rc) const
{
    const int index = currentIndex();
    return index >= 0 ? m_serialControlTabs.at(index).serialControl == rc : false;
}

int SerialOutputPane::findTabWithPort(const QString &portName) const
{
    return Utils::indexOf(m_serialControlTabs, [&portName](const SerialControlTab &tab) {
        return tab.serialControl->portName() == portName;
    });
}

int SerialOutputPane::findRunningTabWithPort(const QString &portName) const
{
    return Utils::indexOf(m_serialControlTabs, [&portName](const SerialControlTab &tab) {
        return tab.serialControl->isRunning() && tab.serialControl->portName() == portName;
    });
}

void SerialOutputPane::handleOldOutput(Core::OutputWindow *window) const
{
    // TODO: cleanOldAppOutput setting? (window->clear();)
    window->grayOutOldContent();
}

void SerialOutputPane::updateCloseActions()
{
    const int tabCount = m_tabWidget->count();
    m_closeCurrentTabAction->setEnabled(tabCount > 0);
    m_closeAllTabsAction->setEnabled(tabCount > 0);
    m_closeOtherTabsAction->setEnabled(tabCount > 1);
}

bool SerialOutputPane::closeTab(int tabIndex, CloseTabMode closeTabMode)
{
    Q_UNUSED(closeTabMode)

    int index = indexOf(m_tabWidget->widget(tabIndex));
    QTC_ASSERT(index != -1, return true);

    qCDebug(log) << "close tab " << tabIndex << m_serialControlTabs[index].serialControl
                 << m_serialControlTabs[index].window;

    // TODO: Prompt user to stop
    if (m_serialControlTabs[index].serialControl->isRunning()) {
        m_serialControlTabs[index].serialControl->stop(true); // Force stop
    }

    m_tabWidget->removeTab(tabIndex);
    delete m_serialControlTabs[index].serialControl;
    delete m_serialControlTabs[index].window;
    m_serialControlTabs.removeAt(index);
    updateCloseActions();

    if (m_serialControlTabs.isEmpty())
        hide();

    return true;
}

void SerialOutputPane::contextMenuRequested(const QPoint &pos, int index)
{
    QList<QAction *> actions { m_closeCurrentTabAction, m_closeAllTabsAction, m_closeOtherTabsAction };

    QAction *action = QMenu::exec(actions, m_tabWidget->mapToGlobal(pos), nullptr, m_tabWidget);
    const int currentIdx = index != -1 ? index : currentIndex();

    if (action == m_closeCurrentTabAction) {
        if (currentIdx >= 0)
            closeTab(currentIdx);
    } else if (action == m_closeAllTabsAction) {
        closeTabs(SerialOutputPane::CloseTabWithPrompt);
    } else if (action == m_closeOtherTabsAction) {
        for (int t = m_tabWidget->count() - 1; t >= 0; t--)
            if (t != currentIdx)
                closeTab(t);
    }
}

void SerialOutputPane::enableDefaultButtons()
{
    const SerialControl *rc = currentSerialControl();
    const bool isRunning = rc && rc->isRunning();
    enableButtons(rc, isRunning);
}

void SerialOutputPane::enableButtons(const SerialControl *rc, bool isRunning)
{
    // TODO: zoom buttons?
    if (rc) {
        m_connectButton->setEnabled(!isRunning);
        m_disconnectButton->setEnabled(isRunning);
        m_resetButton->setEnabled(isRunning);

        m_portsSelection->setEnabled(!isRunning);
        m_baudRateSelection->setEnabled(!isRunning);
    } else {
        m_connectButton->setEnabled(true);
        m_disconnectButton->setEnabled(false);

        m_portsSelection->setEnabled(true);
        m_baudRateSelection->setEnabled(true);
    }
}

void SerialOutputPane::tabChanged(int i)
{
    if (m_prevTabIndex >= 0 && m_prevTabIndex < m_serialControlTabs.size()) {
        m_serialControlTabs[m_prevTabIndex].inputText = m_inputLine->text();
        m_serialControlTabs[m_prevTabIndex].inputCursorPosition = m_inputLine->cursorPosition();
    }

    const int index = indexOf(m_tabWidget->widget(i));
    if (i != -1 && index != -1) {
        SerialControlTab &tab = m_serialControlTabs[index];
        const SerialControl *rc = tab.serialControl;

        // Update combobox index
        m_portsSelection->blockSignals(true);
        m_baudRateSelection->blockSignals(true);
        m_lineEndingsSelection->blockSignals(true);

        m_portsSelection->setCurrentText(rc->portName());
        m_baudRateSelection->setCurrentText(rc->baudRateText());
        m_lineEndingsSelection->setCurrentIndex(tab.lineEndingIndex);
        tab.lineEnd = m_lineEndingsSelection->currentData().toByteArray();

        m_portsSelection->blockSignals(false);
        m_baudRateSelection->blockSignals(false);
        m_lineEndingsSelection->blockSignals(false);

        m_inputLine->setText(tab.inputText);
        m_inputLine->setCursorPosition(tab.inputCursorPosition);

        qCDebug(log) << "Changed tab, is running:" << rc->isRunning();

        // Update buttons
        enableButtons(rc, rc->isRunning());
    } else {
        enableDefaultButtons();
    }

    m_prevTabIndex = index;
}

bool SerialOutputPane::isRunning() const
{
    return Utils::anyOf(m_serialControlTabs, [](const SerialControlTab &rt) {
        return rt.serialControl->isRunning();
    });
}

void SerialOutputPane::activePortNameChanged(int index)
{
    // When the model is updated, current index is reset -> set it back
    if (index < 0) {
        m_portsSelection->setCurrentText(m_currentPortName);
        return;
    }

    const QString pn = m_devicesModel->portName(index);

    if (SerialControl *current = currentSerialControl()) {
        if (current->portName() == pn || pn.isEmpty())
            return;

        m_currentPortName = current->portName();

        qCDebug(log) << "Set port to" << pn << "(" << index << ")";
        current->setPortName(pn);

        // Update tab text
        const int tabIndex = indexOf(current);
        if (tabIndex >= 0)
            m_tabWidget->setTabText(tabIndex, pn);
    }

    // Update current port name
    m_currentPortName = pn;
    m_settings.setPortName(pn);
    emit settingsChanged(m_settings);
}

void SerialOutputPane::activeBaudRateChanged(int index)
{
    if (index < 0)
        return;

    SerialControl *current = currentSerialControl();
    const qint32 br = m_devicesModel->baudRate(index);

    qCDebug(log) << "Set baudrate to" << br << "(" << index << ")";

    if (current)
        current->setBaudRate(br);

    m_settings.setBaudRate(br);
    emit settingsChanged(m_settings);
}

void SerialOutputPane::defaultLineEndingChanged(int index)
{
    if (index < 0)
        return;

    m_settings.setDefaultLineEndingIndex(index);
    const int currentControlIndex = currentIndex();
    if (currentControlIndex >= 0) {
        m_serialControlTabs[currentControlIndex].lineEnd =
                m_lineEndingsSelection->currentData().toByteArray();
    }

    qCDebug(log) << "Set default line ending to "
                 << m_settings.defaultLineEndingText()
                 << "(" << index << ")";

    emit settingsChanged(m_settings);
}

void SerialOutputPane::connectControl()
{
    const QString currentPortName = m_devicesModel->portName(m_portsSelection->currentIndex());
    if (currentPortName.isEmpty())
        return;

    SerialControl *current = currentSerialControl();
    const int index = currentIndex();
    // MAYBE: use current->canReUseOutputPane(...)?

    // Show tab if already opened and running
    const int i = findRunningTabWithPort(currentPortName);
    if (i >= 0) {
        m_tabWidget->setCurrentIndex(i);
        qCDebug(log) << "Port running in tab #" << i;
        return;
    }

    if (current) {
        current->setPortName(currentPortName);
        current->setBaudRate(m_devicesModel->baudRate(m_baudRateSelection->currentIndex()));
        // Gray out old and connect
        if (index != -1) {
            auto &tab = m_serialControlTabs[index];
            handleOldOutput(tab.window);
            tab.window->scrollToBottom();
        }
        qCDebug(log) << "Connect to" << current->portName();
    } else {
        // Create a new window
        auto rc = new SerialControl(m_settings);
        rc->setPortName(currentPortName);
        createNewOutputWindow(rc);

        qCDebug(log) << "Create and connect to" << rc->portName();

        current = rc;
    }

    // Update tab text
    if (index != -1)
        m_tabWidget->setTabText(index, current->portName());

    current->start();
}

void SerialOutputPane::disconnectControl()
{
    SerialControl *current = currentSerialControl();
    if (current) {
        current->stop(true);
        qCDebug(log) << "Disconnected.";
    }
}

void SerialOutputPane::resetControl()
{
    SerialControl *current = currentSerialControl();
    if (current)
        current->pulseDataTerminalReady();
}

void SerialOutputPane::openNewTerminalControl()
{
    const QString currentPortName = m_devicesModel->portName(m_portsSelection->currentIndex());
    if (currentPortName.isEmpty())
        return;

    auto rc = new SerialControl(m_settings);
    rc->setPortName(currentPortName);
    createNewOutputWindow(rc);

    qCDebug(log) << "Created new terminal on" << rc->portName();
}

// Send lineedit input to serial port
void SerialOutputPane::sendInput()
{
    SerialControl *current = currentSerialControl();
    const int index = currentIndex();
    if (current && current->isRunning() && index >= 0) {
        qCDebug(log) << "Sending:" << m_inputLine->text().toUtf8();

        current->writeData(m_inputLine->text().toUtf8() + m_serialControlTabs.at(index).lineEnd);
    }
    m_inputLine->selectAll();
    // TODO: add a blink or something to visually see if the data was sent or not
}

} // namespace Internal
} // namespace SerialTerminal

#include "serialoutputpane.moc"

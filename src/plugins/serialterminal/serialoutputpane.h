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

#pragma once

#include "serialdevicemodel.h"
#include "serialterminalsettings.h"

#include <coreplugin/ioutputpane.h>
#include <utils/outputformat.h>

#include <QWidget>

#include <memory>

QT_BEGIN_NAMESPACE
class QToolButton;
class QButtonGroup;
class QAbstractButton;
class QComboBox;
QT_END_NAMESPACE

namespace Core { class OutputWindow; }

namespace SerialTerminal {
namespace Internal {

class SerialControl;
class TabWidget;
class ComboBox;
class ConsoleLineEdit;

class SerialOutputPane : public Core::IOutputPane
{
    Q_OBJECT

public:
    enum CloseTabMode {
        CloseTabNoPrompt,
        CloseTabWithPrompt
    };

    enum BehaviorOnOutput {
        Flash,
        Popup
    };

    explicit SerialOutputPane(Settings &settings);

    // IOutputPane
    QWidget *outputWidget(QWidget *parent) final;
    QList<QWidget *> toolBarWidgets() const final;
    QString displayName() const final;

    int priorityInStatusBar() const final;
    void clearContents() final;
    void visibilityChanged(bool) final;
    bool canFocus() const final;
    bool hasFocus() const final;
    void setFocus() final;

    bool canNext() const final;
    bool canPrevious() const final;
    void goToNext() final;
    void goToPrev() final;
    bool canNavigate() const final;

    void createNewOutputWindow(SerialControl *rc);

    bool closeTabs(CloseTabMode mode);

    void appendMessage(SerialControl *rc, const QString &out, Utils::OutputFormat format);

    void setSettings(const Settings &settings);

signals:
    void settingsChanged(const Settings &settings);

private:
    class SerialControlTab {
    public:
        explicit SerialControlTab(SerialControl *serialControl = nullptr,
                                  Core::OutputWindow *window = nullptr);
        SerialControl *serialControl = nullptr;
        Core::OutputWindow *window = nullptr;
        BehaviorOnOutput behaviorOnOutput = Flash;
        int inputCursorPosition = 0;
        QString inputText;
        QByteArray lineEnd;
        int lineEndingIndex = 0;
    };

    void createToolButtons();
    void updateLineEndingsComboBox();
    void updatePortsList();

    void contextMenuRequested(const QPoint &pos, int index);

    void enableDefaultButtons();
    void enableButtons(const SerialControl *rc, bool isRunning);
    void tabChanged(int i);

    bool isRunning() const;

    void activePortNameChanged(int index);
    void activeBaudRateChanged(int index);
    void defaultLineEndingChanged(int index);

    void connectControl();
    void disconnectControl();
    void resetControl();
    void openNewTerminalControl();
    void sendInput();

    bool closeTab(int index, CloseTabMode cm = CloseTabWithPrompt);
    int indexOf(const SerialControl *rc) const;
    int indexOf(const QWidget *outputWindow) const;
    int currentIndex() const;
    SerialControl *currentSerialControl() const;
    bool isCurrent(const SerialControl *rc) const;
    int findTabWithPort(const QString &portName) const;
    int findRunningTabWithPort(const QString &portName) const;
    void handleOldOutput(Core::OutputWindow *window) const;

    void updateCloseActions();


    std::unique_ptr<QWidget> m_mainWidget;
    ConsoleLineEdit *m_inputLine = nullptr;
    QComboBox *m_lineEndingsSelection = nullptr;
    TabWidget *m_tabWidget = nullptr;
    Settings m_settings;
    QVector<SerialControlTab> m_serialControlTabs;
    int m_prevTabIndex = -1;

    SerialDeviceModel *m_devicesModel = nullptr;

    QAction *m_closeCurrentTabAction = nullptr;
    QAction *m_closeAllTabsAction = nullptr;
    QAction *m_closeOtherTabsAction = nullptr;

    QAction *m_disconnectAction = nullptr;
    QToolButton *m_connectButton = nullptr;
    QToolButton *m_disconnectButton = nullptr;
    QToolButton *m_resetButton = nullptr;
    QToolButton *m_newButton = nullptr;
    ComboBox *m_portsSelection = nullptr;
    ComboBox *m_baudRateSelection = nullptr;

    QString m_currentPortName;
    float m_zoom = 1.0;
};

} // namespace Internal
} // namespace SerialTerminal

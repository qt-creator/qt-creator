// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

#include <QToolButton>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QStackedWidget;
QT_END_NAMESPACE

namespace Core {

class ICore;
class IOutputPane;

namespace Internal {

class ICorePrivate;
class MainWindow;
class OutputPaneManageButton;

class OutputPaneManager : public QWidget
{
    Q_OBJECT

public:
    static OutputPaneManager *instance();
    void updateStatusButtons(bool visible);
    static void updateMaximizeButton(bool maximized);

    static int outputPaneHeightSetting();
    static void setOutputPaneHeightSetting(int value);
    static bool initialized();

public slots:
    void slotHide();
    void slotNext();
    void slotPrev();
    static void toggleMaximized();

protected:
    void focusInEvent(QFocusEvent *e) override;
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    // the only class that is allowed to create and destroy
    friend class Core::ICore;
    friend class ICorePrivate;
    friend class MainWindow;
    friend class OutputPaneManageButton;
    friend class Core::IOutputPane;

    explicit OutputPaneManager(QWidget *parent = nullptr);
    ~OutputPaneManager() override;

    static void create();
    static void initialize();
    static void setupButtons();
    static void destroy();

    void shortcutTriggered(int idx);
    void clearPage();
    void popupMenu();
    void saveSettings() const;
    void showPage(int idx, int flags);
    void ensurePageVisible(int idx);
    int currentIndex() const;
    void setCurrentIndex(int idx);
    void buttonTriggered(int idx);
    void readSettings();
    void updateActions(IOutputPane *pane);

    QLabel *m_titleLabel = nullptr;
    OutputPaneManageButton *m_manageButton = nullptr;

    QAction *m_clearAction = nullptr;
    QAction *m_minMaxAction = nullptr;
    QAction *m_nextAction = nullptr;
    QAction *m_prevAction = nullptr;

    QStackedWidget *m_outputWidgetPane = nullptr;
    QStackedWidget *m_opToolBarWidgets = nullptr;
    QWidget *m_buttonsWidget = nullptr;
    int m_outputPaneHeightSetting = 0;
    bool m_initialized = false;
};

} // namespace Internal
} // namespace Core

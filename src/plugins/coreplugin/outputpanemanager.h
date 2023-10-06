// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

#include <QToolButton>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QStackedWidget;
class QTimeLine;
QT_END_NAMESPACE

namespace Core {

class ICore;

namespace Internal {

class ICorePrivate;
class MainWindow;
class OutputPaneToggleButton;
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

    // FIXME: Hide again
    static void create();
    static void initialize();
    static void destroy();

public slots:
    void slotHide();
    void slotNext();
    void slotPrev();
    static void toggleMaximized();

protected:
    void focusInEvent(QFocusEvent *e) override;

private:
    // the only class that is allowed to create and destroy
    friend class ICore;
    friend class ICorePrivate;
    friend class MainWindow;
    friend class OutputPaneManageButton;

    explicit OutputPaneManager(QWidget *parent = nullptr);
    ~OutputPaneManager() override;

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

    QLabel *m_titleLabel = nullptr;
    OutputPaneManageButton *m_manageButton = nullptr;

    QAction *m_minMaxAction = nullptr;
    QAction *m_nextAction = nullptr;
    QAction *m_prevAction = nullptr;

    QStackedWidget *m_outputWidgetPane = nullptr;
    QStackedWidget *m_opToolBarWidgets = nullptr;
    QWidget *m_buttonsWidget = nullptr;
    int m_outputPaneHeightSetting = 0;
};

class BadgeLabel
{
public:
    BadgeLabel();
    void paint(QPainter *p, int x, int y, bool isChecked);
    void setText(const QString &text);
    QString text() const;
    QSize sizeHint() const;

private:
    void calculateSize();

    QSize m_size;
    QString m_text;
    QFont m_font;
    static const int m_padding = 6;
};

class OutputPaneToggleButton : public QToolButton
{
    Q_OBJECT
public:
    OutputPaneToggleButton(int number, const QString &text, QAction *action,
                           QWidget *parent = nullptr);
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent*) override;
    void flash(int count = 3);
    void setIconBadgeNumber(int number);
    bool isPaneVisible() const;

    void contextMenuEvent(QContextMenuEvent *e) override;

signals:
    void contextMenuRequested();

private:
    void updateToolTip();
    void checkStateSet() override;

    QString m_number;
    QString m_text;
    QAction *m_action;
    QTimeLine *m_flashTimer;
    BadgeLabel m_badgeNumberLabel;
};

class OutputPaneManageButton : public QToolButton
{
    Q_OBJECT
public:
    OutputPaneManageButton();
    void paintEvent(QPaintEvent *) override;

    void contextMenuEvent(QContextMenuEvent *e) override;

signals:
    void menuRequested();
};

} // namespace Internal
} // namespace Core

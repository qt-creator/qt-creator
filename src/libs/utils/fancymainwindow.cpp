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

#include "fancymainwindow.h"

#include "algorithm.h"
#include "qtcassert.h"

#include <QAbstractButton>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QSettings>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QToolButton>

static const char AutoHideTitleBarsKey[] = "AutoHideTitleBars";
static const char ShowCentralWidgetKey[] = "ShowCentralWidget";
static const char StateKey[] = "State";

static const int settingsVersion = 2;
static const char dockWidgetActiveState[] = "DockWidgetActiveState";

namespace Utils {

class TitleBarWidget;

struct FancyMainWindowPrivate
{
    FancyMainWindowPrivate(FancyMainWindow *parent);

    FancyMainWindow *q;

    bool m_handleDockVisibilityChanges;
    QAction m_showCentralWidget;
    QAction m_menuSeparator1;
    QAction m_menuSeparator2;
    QAction m_resetLayoutAction;
    QAction m_autoHideTitleBars;
};

class DockWidget : public QDockWidget
{
public:
    DockWidget(QWidget *inner, FancyMainWindow *parent, bool immutable = false);

    bool eventFilter(QObject *, QEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    void handleMouseTimeout();
    void handleToplevelChanged(bool floating);

    FancyMainWindow *q;

private:
    QPoint m_startPos;
    TitleBarWidget *m_titleBar;
    QTimer m_timer;
    bool m_immutable = false;
};

// Stolen from QDockWidgetTitleButton
class DockWidgetTitleButton : public QAbstractButton
{
public:
    DockWidgetTitleButton(QWidget *parent)
        : QAbstractButton(parent)
    {
        setFocusPolicy(Qt::NoFocus);
    }

    QSize sizeHint() const
    {
        ensurePolished();

        int size = 2*style()->pixelMetric(QStyle::PM_DockWidgetTitleBarButtonMargin, 0, this);
        if (!icon().isNull()) {
            int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, this);
            QSize sz = icon().actualSize(QSize(iconSize, iconSize));
            size += qMax(sz.width(), sz.height());
        }

        return QSize(size, size);
    }

    QSize minimumSizeHint() const { return sizeHint(); }

    void enterEvent(QEvent *event)
    {
        if (isEnabled())
            update();
        QAbstractButton::enterEvent(event);
    }

    void leaveEvent(QEvent *event)
    {
        if (isEnabled())
            update();
        QAbstractButton::leaveEvent(event);
    }

    void paintEvent(QPaintEvent *event);
};

void DockWidgetTitleButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    QStyleOptionToolButton opt;
    opt.init(this);
    opt.state |= QStyle::State_AutoRaise;
    opt.icon = icon();
    opt.subControls = 0;
    opt.activeSubControls = 0;
    opt.features = QStyleOptionToolButton::None;
    opt.arrowType = Qt::NoArrow;
    int size = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, this);
    opt.iconSize = QSize(size, size);
    style()->drawComplexControl(QStyle::CC_ToolButton, &opt, &p, this);
}

class TitleBarWidget : public QWidget
{
public:
    TitleBarWidget(DockWidget *parent, const QStyleOptionDockWidget &opt)
      : QWidget(parent), q(parent), m_active(true)
    {
        m_titleLabel = new QLabel(this);

        m_floatButton = new DockWidgetTitleButton(this);
        m_floatButton->setIcon(q->style()->standardIcon(QStyle::SP_TitleBarNormalButton, &opt, q));

        m_closeButton = new DockWidgetTitleButton(this);
        m_closeButton->setIcon(q->style()->standardIcon(QStyle::SP_TitleBarCloseButton, &opt, q));

#ifndef QT_NO_ACCESSIBILITY
        m_floatButton->setAccessibleName(QDockWidget::tr("Float"));
        m_floatButton->setAccessibleDescription(QDockWidget::tr("Undocks and re-attaches the dock widget"));
        m_closeButton->setAccessibleName(QDockWidget::tr("Close"));
        m_closeButton->setAccessibleDescription(QDockWidget::tr("Closes the dock widget"));
#endif

        setActive(false);

        const int minWidth = 10;
        const int maxWidth = 10000;
        const int inactiveHeight = 0;
        const int activeHeight = m_closeButton->sizeHint().height() + 2;

        m_minimumInactiveSize = QSize(minWidth, inactiveHeight);
        m_maximumInactiveSize = QSize(maxWidth, inactiveHeight);
        m_minimumActiveSize   = QSize(minWidth, activeHeight);
        m_maximumActiveSize   = QSize(maxWidth, activeHeight);

        auto layout = new QHBoxLayout(this);
        layout->setMargin(0);
        layout->setSpacing(0);
        layout->setContentsMargins(4, 0, 0, 0);
        layout->addWidget(m_titleLabel);
        layout->addStretch();
        layout->addWidget(m_floatButton);
        layout->addWidget(m_closeButton);
        setLayout(layout);

        setProperty("managed_titlebar", 1);
    }

    void enterEvent(QEvent *event)
    {
        setActive(true);
        QWidget::enterEvent(event);
    }

    void setActive(bool on)
    {
        m_active = on;
        updateChildren();
    }

    void updateChildren()
    {
        bool clickable = isClickable();
        m_titleLabel->setVisible(clickable);
        m_floatButton->setVisible(clickable);
        m_closeButton->setVisible(clickable);
    }

    bool isClickable() const
    {
        return m_active || !q->q->autoHideTitleBars();
    }

    QSize sizeHint() const
    {
        ensurePolished();
        return isClickable() ? m_maximumActiveSize : m_maximumInactiveSize;
    }

    QSize minimumSizeHint() const
    {
        ensurePolished();
        return isClickable() ? m_minimumActiveSize : m_minimumInactiveSize;
    }

private:
    DockWidget *q;
    bool m_active;
    QSize m_minimumActiveSize;
    QSize m_maximumActiveSize;
    QSize m_minimumInactiveSize;
    QSize m_maximumInactiveSize;

public:
    QLabel *m_titleLabel;
    DockWidgetTitleButton *m_floatButton;
    DockWidgetTitleButton *m_closeButton;
};

DockWidget::DockWidget(QWidget *inner, FancyMainWindow *parent, bool immutable)
    : QDockWidget(parent), q(parent), m_immutable(immutable)
{
    setWidget(inner);
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    setObjectName(inner->objectName() + QLatin1String("DockWidget"));
    setWindowTitle(inner->windowTitle());
    setMouseTracking(true);

    QStyleOptionDockWidget opt;
    initStyleOption(&opt);
    m_titleBar = new TitleBarWidget(this, opt);
    m_titleBar->m_titleLabel->setText(inner->windowTitle());
    setTitleBarWidget(m_titleBar);

    if (immutable)
        return;

    m_timer.setSingleShot(true);
    m_timer.setInterval(500);

    connect(&m_timer, &QTimer::timeout, this, &DockWidget::handleMouseTimeout);

    connect(this, &QDockWidget::topLevelChanged, this, &DockWidget::handleToplevelChanged);

    connect(toggleViewAction(), &QAction::triggered,
            [this]() {
                if (isVisible())
                    raise();
            });

    auto origFloatButton = findChild<QAbstractButton *>(QLatin1String("qt_dockwidget_floatbutton"));
    connect(m_titleBar->m_floatButton, &QAbstractButton::clicked,
            origFloatButton, &QAbstractButton::clicked);

    auto origCloseButton = findChild<QAbstractButton *>(QLatin1String("qt_dockwidget_closebutton"));
    connect(m_titleBar->m_closeButton, &QAbstractButton::clicked,
            origCloseButton, &QAbstractButton::clicked);
}

bool DockWidget::eventFilter(QObject *, QEvent *event)
{
    if (!m_immutable && event->type() == QEvent::MouseMove && q->autoHideTitleBars()) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        int y = me->pos().y();
        int x = me->pos().x();
        int h = qMin(8, m_titleBar->m_floatButton->height());
        if (!isFloating() && widget() && 0 <= x && x < widget()->width() && 0 <= y && y <= h) {
            m_timer.start();
            m_startPos = mapToGlobal(me->pos());
        }
    }
    return false;
}

void DockWidget::enterEvent(QEvent *event)
{
    if (!m_immutable)
        QApplication::instance()->installEventFilter(this);
    QDockWidget::enterEvent(event);
}

void DockWidget::leaveEvent(QEvent *event)
{
    if (!m_immutable) {
        if (!isFloating()) {
            m_timer.stop();
            m_titleBar->setActive(false);
        }
        QApplication::instance()->removeEventFilter(this);
    }
    QDockWidget::leaveEvent(event);
}

void DockWidget::handleMouseTimeout()
{
    QPoint dist = m_startPos - QCursor::pos();
    if (!isFloating() && dist.manhattanLength() < 4)
        m_titleBar->setActive(true);
}

void DockWidget::handleToplevelChanged(bool floating)
{
    m_titleBar->setActive(floating);
}



/*! \class Utils::FancyMainWindow

    \brief The FancyMainWindow class is a MainWindow with dock widgets and
    additional "lock" functionality
    (locking the dock widgets in place) and "reset layout" functionality.

    The dock actions and the additional actions should be accessible
    in a Window-menu.
*/

FancyMainWindowPrivate::FancyMainWindowPrivate(FancyMainWindow *parent) :
    q(parent),
    m_handleDockVisibilityChanges(true),
    m_showCentralWidget(FancyMainWindow::tr("Central Widget"), 0),
    m_menuSeparator1(0),
    m_menuSeparator2(0),
    m_resetLayoutAction(FancyMainWindow::tr("Reset to Default Layout"), 0),
    m_autoHideTitleBars(FancyMainWindow::tr("Automatically Hide View Title Bars"), 0)
{
    m_showCentralWidget.setCheckable(true);
    m_showCentralWidget.setChecked(true);

    m_menuSeparator1.setSeparator(true);
    m_menuSeparator2.setSeparator(true);

    m_autoHideTitleBars.setCheckable(true);
    m_autoHideTitleBars.setChecked(true);

    QObject::connect(&m_autoHideTitleBars, &QAction::toggled, q, [this](bool) {
        foreach (QDockWidget *dock, q->dockWidgets()) {
            if (auto titleBar = dynamic_cast<TitleBarWidget *>(dock->titleBarWidget()))
                titleBar->updateChildren();
        }
    });

    QObject::connect(&m_showCentralWidget, &QAction::toggled, q, [this](bool visible) {
        q->centralWidget()->setVisible(visible);
    });
}

FancyMainWindow::FancyMainWindow(QWidget *parent) :
    QMainWindow(parent), d(new FancyMainWindowPrivate(this))
{
    connect(&d->m_resetLayoutAction, &QAction::triggered,
            this, &FancyMainWindow::resetLayout);
}

FancyMainWindow::~FancyMainWindow()
{
    delete d;
}

QDockWidget *FancyMainWindow::addDockForWidget(QWidget *widget, bool immutable)
{
    QTC_ASSERT(widget, return 0);
    QTC_CHECK(widget->objectName().size());
    QTC_CHECK(widget->windowTitle().size());

    auto dockWidget = new DockWidget(widget, this, immutable);

    if (!immutable) {
        connect(dockWidget, &QDockWidget::visibilityChanged,
            [this, dockWidget](bool visible) {
                if (d->m_handleDockVisibilityChanges)
                    dockWidget->setProperty(dockWidgetActiveState, visible);
            });

        connect(dockWidget->toggleViewAction(), &QAction::triggered,
                this, &FancyMainWindow::onDockActionTriggered,
                Qt::QueuedConnection);

        dockWidget->setProperty(dockWidgetActiveState, true);
    }

    return dockWidget;
}

void FancyMainWindow::onDockActionTriggered()
{
    QDockWidget *dw = qobject_cast<QDockWidget *>(sender()->parent());
    if (dw) {
        if (dw->isVisible())
            dw->raise();
    }
}

void FancyMainWindow::setTrackingEnabled(bool enabled)
{
    if (enabled) {
        d->m_handleDockVisibilityChanges = true;
        foreach (QDockWidget *dockWidget, dockWidgets())
            dockWidget->setProperty(dockWidgetActiveState, dockWidget->isVisible());
    } else {
        d->m_handleDockVisibilityChanges = false;
    }
}

void FancyMainWindow::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    handleVisibilityChanged(false);
}

void FancyMainWindow::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    handleVisibilityChanged(true);
}

void FancyMainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    addDockActionsToMenu(&menu);
    menu.exec(event->globalPos());
}

void FancyMainWindow::handleVisibilityChanged(bool visible)
{
    d->m_handleDockVisibilityChanges = false;
    foreach (QDockWidget *dockWidget, dockWidgets()) {
        if (dockWidget->isFloating()) {
            dockWidget->setVisible(visible
                && dockWidget->property(dockWidgetActiveState).toBool());
        }
    }
    if (visible)
        d->m_handleDockVisibilityChanges = true;
}

void FancyMainWindow::saveSettings(QSettings *settings) const
{
    QHash<QString, QVariant> hash = saveSettings();
    QHashIterator<QString, QVariant> it(hash);
    while (it.hasNext()) {
        it.next();
        settings->setValue(it.key(), it.value());
    }
}

void FancyMainWindow::restoreSettings(const QSettings *settings)
{
    QHash<QString, QVariant> hash;
    foreach (const QString &key, settings->childKeys()) {
        hash.insert(key, settings->value(key));
    }
    restoreSettings(hash);
}

QHash<QString, QVariant> FancyMainWindow::saveSettings() const
{
    QHash<QString, QVariant> settings;
    settings.insert(QLatin1String(StateKey), saveState(settingsVersion));
    settings.insert(QLatin1String(AutoHideTitleBarsKey),
        d->m_autoHideTitleBars.isChecked());
    settings.insert(ShowCentralWidgetKey, d->m_showCentralWidget.isChecked());
    foreach (QDockWidget *dockWidget, dockWidgets()) {
        settings.insert(dockWidget->objectName(),
                dockWidget->property(dockWidgetActiveState));
    }
    return settings;
}

void FancyMainWindow::restoreSettings(const QHash<QString, QVariant> &settings)
{
    QByteArray ba = settings.value(QLatin1String(StateKey), QByteArray()).toByteArray();
    if (!ba.isEmpty())
        restoreState(ba, settingsVersion);
    bool on = settings.value(QLatin1String(AutoHideTitleBarsKey), true).toBool();
    d->m_autoHideTitleBars.setChecked(on);
    d->m_showCentralWidget.setChecked(settings.value(ShowCentralWidgetKey, true).toBool());
    foreach (QDockWidget *widget, dockWidgets()) {
        widget->setProperty(dockWidgetActiveState,
            settings.value(widget->objectName(), false));
    }
}

QList<QDockWidget *> FancyMainWindow::dockWidgets() const
{
    return findChildren<QDockWidget *>();
}

bool FancyMainWindow::autoHideTitleBars() const
{
    return d->m_autoHideTitleBars.isChecked();
}

void FancyMainWindow::addDockActionsToMenu(QMenu *menu)
{
    QList<QAction *> actions;
    QList<QDockWidget *> dockwidgets = findChildren<QDockWidget *>();
    for (int i = 0; i < dockwidgets.size(); ++i) {
        QDockWidget *dockWidget = dockwidgets.at(i);
        if (dockWidget->property("managed_dockwidget").isNull()
                && dockWidget->parentWidget() == this) {
            actions.append(dockwidgets.at(i)->toggleViewAction());
        }
    }
    Utils::sort(actions, [](const QAction *action1, const QAction *action2) {
        QTC_ASSERT(action1, return true);
        QTC_ASSERT(action2, return false);
        return action1->text().toLower() < action2->text().toLower();
    });
    foreach (QAction *action, actions)
        menu->addAction(action);
    menu->addAction(&d->m_showCentralWidget);
    menu->addAction(&d->m_menuSeparator1);
    menu->addAction(&d->m_autoHideTitleBars);
    menu->addAction(&d->m_menuSeparator2);
    menu->addAction(&d->m_resetLayoutAction);
}

QAction *FancyMainWindow::menuSeparator1() const
{
    return &d->m_menuSeparator1;
}

QAction *FancyMainWindow::autoHideTitleBarsAction() const
{
    return &d->m_autoHideTitleBars;
}

QAction *FancyMainWindow::menuSeparator2() const
{
    return &d->m_menuSeparator2;
}

QAction *FancyMainWindow::resetLayoutAction() const
{
    return &d->m_resetLayoutAction;
}

QAction *FancyMainWindow::showCentralWidgetAction() const
{
    return &d->m_showCentralWidget;
}

void FancyMainWindow::setDockActionsVisible(bool v)
{
    foreach (const QDockWidget *dockWidget, dockWidgets())
        dockWidget->toggleViewAction()->setVisible(v);
    d->m_showCentralWidget.setVisible(v);
    d->m_autoHideTitleBars.setVisible(v);
    d->m_menuSeparator1.setVisible(v);
    d->m_menuSeparator2.setVisible(v);
    d->m_resetLayoutAction.setVisible(v);
}

} // namespace Utils

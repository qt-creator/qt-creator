// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fancymainwindow.h"

#include "algorithm.h"
#include "qtcassert.h"
#include "qtcsettings.h"
#include "stringutils.h"
#include "styledbar.h"
#include "utilsicons.h"
#include "utilstr.h"

#include <QAbstractButton>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>

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
    QAction m_resetLayoutAction;
};

class DockWidget : public QDockWidget
{
public:
    DockWidget(QWidget *inner, FancyMainWindow *parent, bool immutable = false);
    ~DockWidget();

    FancyMainWindow *q;

    QWidget *m_hiddenInnerWidget = nullptr;

private:
    QPoint m_startPos;
    TitleBarWidget *m_titleBar;
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

    QSize sizeHint() const override
    {
        ensurePolished();

        int size = 2*style()->pixelMetric(QStyle::PM_DockWidgetTitleBarButtonMargin, nullptr, this);
        if (!icon().isNull()) {
            int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
            QSize sz = icon().actualSize(QSize(iconSize, iconSize));
            size += qMax(sz.width(), sz.height());
        }

        return QSize(size, size);
    }

    QSize minimumSizeHint() const override { return sizeHint(); }

    void enterEvent(QEnterEvent *event) override
    {
        if (isEnabled())
            update();
        QAbstractButton::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        if (isEnabled())
            update();
        QAbstractButton::leaveEvent(event);
    }

    void paintEvent(QPaintEvent *event) override;
};

const int titleMinWidth = 10;
const int titleMaxWidth = 10000;
const int titleInactiveHeight = 0;

void DockWidgetTitleButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    QStyleOptionToolButton opt;
    opt.initFrom(this);
    opt.state |= QStyle::State_AutoRaise;
    opt.icon = icon();
    opt.subControls = {};
    opt.activeSubControls = {};
    opt.features = QStyleOptionToolButton::None;
    opt.arrowType = Qt::NoArrow;
    int size = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
    opt.iconSize = QSize(size, size);
    style()->drawComplexControl(QStyle::CC_ToolButton, &opt, &p, this);
}

class TitleBarWidget : public StyledBar
{
public:
    TitleBarWidget(DockWidget *parent)
        : StyledBar(parent)
        , q(parent)
    {
        m_titleLabel = new QLabel(this);

        m_collapseButton = new DockWidgetTitleButton(this);
        updateCollapse();
        connect(m_collapseButton, &DockWidgetTitleButton::clicked, this, [this] {
            if (supportsCollapse()) {
                if (!q->m_hiddenInnerWidget) {
                    q->m_hiddenInnerWidget = q->widget();
                    auto w = new QWidget;
                    w->setMaximumHeight(0);
                    q->setWidget(w);
                } else {
                    ensureWidgetShown();
                }
            }
            updateCollapse();
        });
        connect(q->q, &FancyMainWindow::dockWidgetsChanged, this, &TitleBarWidget::updateCollapse);

        m_floatButton = new DockWidgetTitleButton(this);
        m_floatButton->setIcon(
            Icon({{":/utils/images/app-on-top.png", Theme::IconsBaseColor}}).icon());

        m_closeButton = new DockWidgetTitleButton(this);
        m_closeButton->setIcon(Icons::CLOSE_TOOLBAR.icon());

#ifndef QT_NO_ACCESSIBILITY
        m_floatButton->setAccessibleName(QDockWidget::tr("Float"));
        m_floatButton->setAccessibleDescription(QDockWidget::tr("Undocks and re-attaches the dock widget"));
        m_closeButton->setAccessibleName(QDockWidget::tr("Close"));
        m_closeButton->setAccessibleDescription(QDockWidget::tr("Closes the dock widget"));
#endif

        auto layout = new QHBoxLayout(this);
        layout->setSpacing(0);
        layout->setContentsMargins(4, 0, 0, 0);
        layout->addWidget(m_collapseButton);
        layout->addWidget(m_titleLabel);
        layout->addStretch();
        layout->addWidget(m_floatButton);
        layout->addWidget(m_closeButton);
        setLayout(layout);

        setProperty("managed_titlebar", 1);

        m_closeButton->setVisible(false);
        m_floatButton->setVisible(false);
        connect(parent, &QDockWidget::featuresChanged, this, [this] { updateChildren(); });
    }

    void enterEvent(QEnterEvent *event) override
    {
        m_hovered = true;
        QWidget::enterEvent(event);
        updateChildren();
    }

    void leaveEvent(QEvent *event) override
    {
        m_hovered = false;
        QWidget::leaveEvent(event);
        updateChildren();
    }

    void setActive(bool on)
    {
        m_active = on;
        updateChildren();
    }

    void updateChildren()
    {
        m_titleLabel->setVisible(m_active);
        m_floatButton->setVisible(m_active && m_hovered
                                  && q->features().testFlag(QDockWidget::DockWidgetFloatable));
        m_closeButton->setVisible(m_active && m_hovered
                                  && q->features().testFlag(QDockWidget::DockWidgetClosable));
        updateCollapse();
    }

    QSize sizeHint() const override
    {
        ensurePolished();
        return m_active ? QSize(titleMaxWidth, StyledBar::minimumHeight())
                        : QSize(titleMaxWidth, titleInactiveHeight);
    }

    QSize minimumSizeHint() const override
    {
        ensurePolished();
        return m_active ? QSize(titleMinWidth, StyledBar::minimumHeight())
                        : QSize(titleMinWidth, titleInactiveHeight);
    }

    bool supportsCollapse() const
    {
        // not if floating
        if (q->isFloating())
            return false;
        // not if tabbed
        if (!filtered(q->q->tabifiedDockWidgets(q), [](QDockWidget *w) {
                 return w->isVisible();
             }).isEmpty())
            return false;
        const QList<QDockWidget *> docksInArea
            = filtered(q->q->dockWidgets(), [this, area = q->q->dockWidgetArea(q)](QDockWidget *w) {
                  return w != q && w->isVisible() && q->q->dockWidgetArea(w) == area;
              });
        // not if only dock in area
        if (docksInArea.isEmpty())
            return false;
        // not if in horizontal layout
        if (anyOf(docksInArea, [y = q->y()](QDockWidget *w) { return w->y() == y; }))
            return false;
        return true;
    }

    void ensureWidgetShown()
    {
        if (q->m_hiddenInnerWidget) {
            delete q->widget();
            q->setWidget(q->m_hiddenInnerWidget);
            q->m_hiddenInnerWidget = nullptr;
        }
    }

    void updateCollapse()
    {
        const bool supported = m_active && supportsCollapse();
        m_collapseButton->setVisible(supported);
        if (!supported)
            ensureWidgetShown();
        if (q->m_hiddenInnerWidget)
            m_collapseButton->setIcon(Icons::NEXT_TOOLBAR.icon());
        else
            m_collapseButton->setIcon(Icons::ARROW_DOWN_TOOLBAR.icon());
    }

private:
    DockWidget *q;
    bool m_active = true;
    bool m_hovered = false;

public:
    QLabel *m_titleLabel;
    DockWidgetTitleButton *m_collapseButton;
    DockWidgetTitleButton *m_floatButton;
    DockWidgetTitleButton *m_closeButton;
};

DockWidget::DockWidget(QWidget *inner, FancyMainWindow *parent, bool immutable)
    : QDockWidget(parent)
    , q(parent)
{
    setWidget(inner);
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    setObjectName(inner->objectName() + QLatin1String("DockWidget"));
    setMouseTracking(true);

    QString title = inner->windowTitle();
    toggleViewAction()->setProperty("original_title", title);
    title = stripAccelerator(title);
    setWindowTitle(title);

    QStyleOptionDockWidget opt;
    initStyleOption(&opt);
    m_titleBar = new TitleBarWidget(this);
    m_titleBar->m_titleLabel->setText(title);
    setTitleBarWidget(m_titleBar);

    if (immutable) {
        m_titleBar->setActive(false);
        return;
    }

    connect(toggleViewAction(), &QAction::triggered, this, [this] {
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

DockWidget::~DockWidget()
{
    delete m_hiddenInnerWidget;
}

/*!
    \class Utils::FancyMainWindow
    \inmodule QtCreator

    \brief The FancyMainWindow class is a MainWindow with dock widgets and
    additional "lock" functionality
    (locking the dock widgets in place) and "reset layout" functionality.

    The dock actions and the additional actions should be accessible
    in a Window-menu.
*/

FancyMainWindowPrivate::FancyMainWindowPrivate(FancyMainWindow *parent)
    : q(parent)
    , m_handleDockVisibilityChanges(true)
    , m_showCentralWidget(Tr::tr("Central Widget"), nullptr)
    , m_menuSeparator1(nullptr)
    , m_resetLayoutAction(Tr::tr("Reset to Default Layout"), nullptr)
{
    m_showCentralWidget.setCheckable(true);
    m_showCentralWidget.setChecked(true);

    m_menuSeparator1.setSeparator(true);

    QObject::connect(&m_showCentralWidget, &QAction::toggled, q, [this](bool visible) {
        if (q->centralWidget())
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
    QTC_ASSERT(widget, return nullptr);
    QTC_CHECK(widget->objectName().size());
    QTC_CHECK(widget->windowTitle().size());

    auto dockWidget = new DockWidget(widget, this, immutable);

    if (!immutable) {
        connect(dockWidget, &QDockWidget::visibilityChanged, this,
                [this, dockWidget](bool visible) {
            if (d->m_handleDockVisibilityChanges)
                dockWidget->setProperty(dockWidgetActiveState, visible);
        });

        connect(dockWidget->toggleViewAction(), &QAction::triggered, dockWidget, [dockWidget] {
            if (dockWidget->isVisible())
                dockWidget->raise();
        }, Qt::QueuedConnection);

        dockWidget->setProperty(dockWidgetActiveState, true);

        connect(dockWidget,
                &QDockWidget::dockLocationChanged,
                this,
                &FancyMainWindow::dockWidgetsChanged);
        connect(dockWidget,
                &QDockWidget::topLevelChanged,
                this,
                &FancyMainWindow::dockWidgetsChanged);
        connect(dockWidget,
                &QDockWidget::visibilityChanged,
                this,
                &FancyMainWindow::dockWidgetsChanged);
    }

    return dockWidget;
}

void FancyMainWindow::setTrackingEnabled(bool enabled)
{
    if (enabled) {
        d->m_handleDockVisibilityChanges = true;
        for (QDockWidget *dockWidget : dockWidgets())
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
    for (QDockWidget *dockWidget : dockWidgets()) {
        if (dockWidget->isFloating()) {
            dockWidget->setVisible(visible
                && dockWidget->property(dockWidgetActiveState).toBool());
        }
    }
    if (visible)
        d->m_handleDockVisibilityChanges = true;
}

void FancyMainWindow::saveSettings(QtcSettings *settings) const
{
    const QHash<Key, QVariant> hash = saveSettings();
    for (auto it = hash.cbegin(), end = hash.cend(); it != end; ++it)
        settings->setValue(it.key(), it.value());
}

void FancyMainWindow::restoreSettings(const QtcSettings *settings)
{
    QHash<Key, QVariant> hash;
    const KeyList childKeys = settings->childKeys();
    for (const Key &key : childKeys)
        hash.insert(key, settings->value(key));
    restoreSettings(hash);
}

QHash<Key, QVariant> FancyMainWindow::saveSettings() const
{
    QHash<Key, QVariant> settings;
    settings.insert(StateKey, saveState(settingsVersion));
    settings.insert(ShowCentralWidgetKey, d->m_showCentralWidget.isChecked());
    for (QDockWidget *dockWidget : dockWidgets()) {
        settings.insert(keyFromString(dockWidget->objectName()),
                dockWidget->property(dockWidgetActiveState));
    }
    return settings;
}

void FancyMainWindow::restoreSettings(const QHash<Key, QVariant> &settings)
{
    QByteArray ba = settings.value(StateKey, QByteArray()).toByteArray();
    if (!ba.isEmpty()) {
        if (!restoreFancyState(ba, settingsVersion))
            qWarning() << "Restoring the state of dock widgets failed.";
    }
    d->m_showCentralWidget.setChecked(settings.value(ShowCentralWidgetKey, true).toBool());
    for (QDockWidget *widget : dockWidgets()) {
        widget->setProperty(dockWidgetActiveState,
                            settings.value(keyFromString(widget->objectName()), false));
    }
}

bool FancyMainWindow::restoreFancyState(const QByteArray &state, int version)
{
    const bool result = restoreState(state, version);
    emit dockWidgetsChanged();
    return result;
}

static void findDockChildren(QWidget *parent, QList<QDockWidget *> &result)
{
    for (QObject *child : parent->children()) {
        QWidget *childWidget = qobject_cast<QWidget *>(child);
        if (!childWidget)
            continue;

        if (auto dockWidget = qobject_cast<QDockWidget *>(child))
            result.append(dockWidget);
        else if (!qobject_cast<QMainWindow *>(child))
            findDockChildren(qobject_cast<QWidget *>(child), result);
    }
}

const QList<QDockWidget *> FancyMainWindow::dockWidgets() const
{
    QList<QDockWidget *> result;
    findDockChildren((QWidget *) this, result);
    return result;
}

bool FancyMainWindow::isCentralWidgetShown() const
{
    return d->m_showCentralWidget.isChecked();
}

void FancyMainWindow::showCentralWidget(bool on)
{
    d->m_showCentralWidget.setChecked(on);
}

void FancyMainWindow::addDockActionsToMenu(QMenu *menu)
{
    QList<QAction *> actions;
    QList<QDockWidget *> dockwidgets = findChildren<QDockWidget *>();
    for (int i = 0; i < dockwidgets.size(); ++i) {
        QDockWidget *dockWidget = dockwidgets.at(i);
        if (dockWidget->property("managed_dockwidget").isNull()
                && dockWidget->parentWidget() == this) {
            QAction *action = dockWidget->toggleViewAction();
            action->setText(action->property("original_title").toString());
            actions.append(action);
        }
    }
    sort(actions, [](const QAction *action1, const QAction *action2) {
        QTC_ASSERT(action1, return true);
        QTC_ASSERT(action2, return false);
        return stripAccelerator(action1->text()).toLower() < stripAccelerator(action2->text()).toLower();
    });
    for (QAction *action : std::as_const(actions))
        menu->addAction(action);
    menu->addAction(&d->m_showCentralWidget);
    menu->addAction(&d->m_menuSeparator1);
    menu->addAction(&d->m_resetLayoutAction);
}

QAction *FancyMainWindow::menuSeparator1() const
{
    return &d->m_menuSeparator1;
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
    for (const QDockWidget *dockWidget : dockWidgets())
        dockWidget->toggleViewAction()->setVisible(v);
    d->m_showCentralWidget.setVisible(v);
    d->m_menuSeparator1.setVisible(v);
    d->m_resetLayoutAction.setVisible(v);
}

} // namespace Utils

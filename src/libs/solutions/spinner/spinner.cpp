// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "spinner.h"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include <QWidget>

namespace SpinnerSolution {

class WidgetOverlay : public QWidget
{
public:
    using PaintFunction = std::function<void(QWidget *, QPainter &, QPaintEvent *)>;

    explicit WidgetOverlay(QWidget *parent = nullptr)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        if (parent)
            attachToWidget(parent);
    }

    void attachToWidget(QWidget *parent)
    {
        if (parentWidget())
            parentWidget()->removeEventFilter(this);
        setParent(parent);
        if (parent) {
            parent->installEventFilter(this);
            resizeToParent();
            raise();
        }
    }
    void setPaintFunction(const PaintFunction &paint) { m_paint = paint; }

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        if (obj == parent() && ev->type() == QEvent::Resize)
            resizeToParent();
        return QWidget::eventFilter(obj, ev);
    }

    void paintEvent(QPaintEvent *ev) override
    {
        if (m_paint) {
            QPainter p(this);
            m_paint(this, p, ev);
        }
    }

private:
    void resizeToParent() { setGeometry(QRect({}, parentWidget()->size())); }

    PaintFunction m_paint;
};

class SpinnerPainter
{
public:
    using UpdateCallback = std::function<void()>;

    SpinnerPainter(SpinnerSize size);

    void setSize(SpinnerSize size);

    void setUpdateCallback(const UpdateCallback &cb) { m_callback = cb; }

    void setColor(const QColor &color);

    QSize size() const { return m_pixmap.size() / m_pixmap.devicePixelRatio(); }
    void paint(QPainter &painter, const QRect &rect) const;
    void startAnimation() { m_timer.start(); }
    void stopAnimation() { m_timer.stop(); }

protected:
    void nextAnimationStep() { m_rotation = (m_rotation + m_rotationStep + 360) % 360; }

private:
    SpinnerSize m_size = SpinnerSize::Small;
    int m_rotationStep = 45;
    int m_rotation = 0;
    QTimer m_timer;
    mutable QPixmap m_pixmap;
    UpdateCallback m_callback;
    QColor m_color;
};

static QString imageFileNameForSpinnerSize(SpinnerSize size)
{
    switch (size) {
    case SpinnerSize::Large:
        return ":/icons/spinner_large.png";
    case SpinnerSize::Medium:
        return ":/icons/spinner_medium.png";
    case SpinnerSize::Small:
        return ":/icons/spinner_small.png";
    }
    return {};
}

static QPixmap themedPixmapForSpinnerSize(SpinnerSize size, const QColor &color, qreal dpr)
{
    QImage mask(qt_findAtNxFile(imageFileNameForSpinnerSize(size), dpr));
    mask.invertPixels();
    QImage themedImage(mask.size(), QImage::Format_ARGB32);
    themedImage.fill(color);
    themedImage.setAlphaChannel(mask);
    QPixmap themedPixmap = QPixmap::fromImage(themedImage);
    themedPixmap.setDevicePixelRatio(mask.devicePixelRatio());
    return themedPixmap;
}

SpinnerPainter::SpinnerPainter(SpinnerSize size)
    : m_color(qApp->palette().text().color())
{
    m_timer.setSingleShot(false);
    QObject::connect(&m_timer, &QTimer::timeout, &m_timer, [this] {
        nextAnimationStep();
        if (m_callback)
            m_callback();
    });
    setSize(size);
}

void SpinnerPainter::setSize(SpinnerSize size)
{
    m_size = size;
    m_rotationStep = size == SpinnerSize::Small ? 45 : 30;
    m_timer.setInterval(size == SpinnerSize::Small ? 100 : 80);
    m_pixmap = themedPixmapForSpinnerSize(size, m_color, qApp->devicePixelRatio());
}

void SpinnerPainter::setColor(const QColor &color)
{
    m_color = color;
    m_pixmap = themedPixmapForSpinnerSize(m_size, m_color, qApp->devicePixelRatio());
}

void SpinnerPainter::paint(QPainter &painter, const QRect &rect) const
{
    const qreal dpr = painter.device()->devicePixelRatioF();
    if (!qFuzzyCompare(m_pixmap.devicePixelRatio(), dpr))
        m_pixmap = themedPixmapForSpinnerSize(m_size, m_color, dpr);
    painter.save();
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPoint translate(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
    QTransform t;
    t.translate(translate.x(), translate.y());
    t.rotate(m_rotation);
    t.translate(-translate.x(), -translate.y());
    painter.setTransform(t);
    QSize pixmapUserSize(m_pixmap.size() / m_pixmap.devicePixelRatio());
    painter.drawPixmap(QPoint(rect.x() + ((rect.width() - pixmapUserSize.width()) / 2),
                              rect.y() + ((rect.height() - pixmapUserSize.height()) / 2)),
                       m_pixmap);
    painter.restore();
}

class SpinnerOverlay : public WidgetOverlay
{
public:
    explicit SpinnerOverlay(SpinnerSize size, QWidget *parent = nullptr)
        : WidgetOverlay(parent)
        , m_paint(size)
    {
        setPaintFunction(
            [this](QWidget *w, QPainter &p, QPaintEvent *) { m_paint.paint(p, w->rect()); });
        m_paint.setUpdateCallback([this] { update(); });
        updateGeometry();
    }

    void setSize(SpinnerSize size)
    {
        m_paint.setSize(size);
        updateGeometry();
    }
    QSize sizeHint() const final { return m_paint.size(); }

    void setColor(const QColor &color)
    {
        m_paint.setColor(color);
    }

protected:
    void showEvent(QShowEvent *) final { m_paint.startAnimation(); }
    void hideEvent(QHideEvent *) final { m_paint.stopAnimation(); }

private:
    SpinnerPainter m_paint;
};

/*!
    \module SpinnerSolution
    \title Spinner Solution
    \ingroup solutions-modules
    \brief Contains a Spinner solution.

    The Spinner solution depends on Qt only, and doesn't depend on any \QC specific code.
*/

/*!
    \namespace SpinnerSolution
    \inmodule SpinnerSolution
    \brief The SpinnerSolution namespace encloses the Spinner class.
*/

/*!
    \enum SpinnerSolution::SpinnerSize

    This enum describes the possible spinner sizes.

    \value Small \inlineimage spinner/icons/spinner_small.png
    \value Medium \inlineimage spinner/icons/spinner_medium.png
    \value Large \inlineimage spinner/icons/spinner_large.png
*/

/*!
    \class SpinnerSolution::Spinner
    \inheaderfile solutions/spinner/spinner.h
    \inmodule SpinnerSolution
    \brief The Spinner class renders a circular, endlessly animated progress indicator,
           that may be attached to any widget as an overlay.
*/

/*!
    Creates a spinner overlay with a given \a size for the passed \a parent widget.

    The \a parent widget takes the ownership of the created spinner.
*/
Spinner::Spinner(SpinnerSize size, QWidget *parent)
    : QObject(parent)
    , m_widget(new SpinnerOverlay(size, parent)) {}

Spinner::~Spinner()
{
    if (m_widget)
        delete m_widget;
}

/*!
    Sets the size of the spinner to the given \a size.
*/
void Spinner::setSize(SpinnerSize size)
{
    if (m_widget)
        m_widget->setSize(size);
}

/*!
    Sets the color of the spinner to \a color.
*/
void Spinner::setColor(const QColor &color)
{
    if (m_widget)
        m_widget->setColor(color);
}

/*!
    Shows the animated spinner as an overlay for the parent widget
    set previously in the constructor.
*/
void Spinner::show()
{
    if (m_widget)
        m_widget->show();
}

/*!
    Hides the spinner.
*/
void Spinner::hide()
{
    if (m_widget)
        m_widget->hide();
}

/*!
    Returns \c true if the spinner is visible; otherwise, returns \c false.
*/
bool Spinner::isVisible() const
{
    return m_widget ? m_widget->isVisible() : false;
}

/*!
    Shows or hides the spinner depending on the value of \a visible.
    By default, the spinner is visible.
*/
void Spinner::setVisible(bool visible)
{
    if (m_widget)
        m_widget->setVisible(visible);
}

static QString colorButtonStyleSheet(const QColor &bgColor)
{
    QString rc("border-width: 1px; border-radius: 1px; border-color: black; ");
    rc += bgColor.isValid() ? "border-style: solid; background:" + bgColor.name() + ";"
                            : QString("border-style: dotted;");
    return rc;
}

static QColor stateToColor(SpinnerState state)
{
    switch (state) {
    case SpinnerState::NotRunning: return Qt::gray;
    case SpinnerState::Running: return Qt::yellow;
    }
    return {};
}

class SpinnerWidgetPrivate : public QLabel
{
public:
    SpinnerWidgetPrivate(QWidget *parent = nullptr)
        : QLabel(parent)
        , m_spinner(new Spinner(SpinnerSize::Small, this))
    {
        updateState();
    }

    void setState(SpinnerState state)
    {
        if (m_state == state)
            return;
        m_state = state;
        updateState();
    }

    void setDecorated(bool on)
    {
        if (m_decorated == on)
            return;
        m_decorated = on;
        updateState();
    }

private:
    void updateState()
    {
        const int size = m_decorated ? 26 : 24;
        setFixedSize(size, size);
        setStyleSheet(m_decorated ? colorButtonStyleSheet(stateToColor(m_state)) : QString());
        if (m_state == SpinnerState::Running)
            m_spinner->show();
        else
            m_spinner->hide();
    }
    bool m_decorated = true;
    SpinnerState m_state = SpinnerState::NotRunning;
    Spinner *m_spinner = nullptr;
};

SpinnerWidget::SpinnerWidget(QWidget *parent)
    : QWidget(parent)
    , d(new SpinnerWidgetPrivate(this))
{
    QBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(d);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void SpinnerWidget::setState(SpinnerState state)
{
    d->setState(state);
}

void SpinnerWidget::setDecorated(bool on)
{
    d->setDecorated(on);
}

} // namespace SpinnerSolution

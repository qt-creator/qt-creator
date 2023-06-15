// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "spinner.h"

#include <QEvent>
#include <QPainter>
#include <QTimer>
#include <QWidget>

namespace SpinnerSolution {

class OverlayWidget : public QWidget
{
public:
    using PaintFunction = std::function<void(QWidget *, QPainter &, QPaintEvent *)>;

    explicit OverlayWidget(QWidget *parent = nullptr)
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
    QPixmap m_pixmap;
    UpdateCallback m_callback;
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

SpinnerPainter::SpinnerPainter(SpinnerSize size)
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
    m_pixmap = QPixmap(imageFileNameForSpinnerSize(size));
}

void SpinnerPainter::paint(QPainter &painter, const QRect &rect) const
{
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

class SpinnerWidget : public OverlayWidget
{
public:
    explicit SpinnerWidget(SpinnerSize size, QWidget *parent = nullptr)
        : OverlayWidget(parent)
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

protected:
    void showEvent(QShowEvent *) final { m_paint.startAnimation(); }
    void hideEvent(QHideEvent *) final { m_paint.stopAnimation(); }

private:
    SpinnerPainter m_paint;
};

Spinner::Spinner(SpinnerSize size, QWidget *parent)
    : QObject(parent)
    , m_widget(new SpinnerWidget(size, parent)) {}

void Spinner::setSize(SpinnerSize size)
{
    m_widget->setSize(size);
}

void Spinner::show()
{
    m_widget->show();
}

void Spinner::hide()
{
    m_widget->hide();
}

bool Spinner::isVisible() const
{
    return m_widget->isVisible();
}

void Spinner::setVisible(bool visible)
{
    m_widget->setVisible(visible);
}

} // namespace SpinnerSolution

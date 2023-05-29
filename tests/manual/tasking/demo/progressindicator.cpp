// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "progressindicator.h"

#include <QEvent>
#include <QPainter>
#include <QTimer>
#include <QWidget>

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
    void resizeToParent() { setGeometry(QRect(QPoint(0, 0), parentWidget()->size())); }

    PaintFunction m_paint;
};

class ProgressIndicatorPainter
{
public:
    using UpdateCallback = std::function<void()>;

    ProgressIndicatorPainter();
    virtual ~ProgressIndicatorPainter() = default;

    void setUpdateCallback(const UpdateCallback &cb) { m_callback = cb; }

    QSize size() const { return m_pixmap.size() / m_pixmap.devicePixelRatio(); }

    void paint(QPainter &painter, const QRect &rect) const;
    void startAnimation() { m_timer.start(); }
    void stopAnimation() { m_timer.stop(); }

protected:
    void nextAnimationStep() { m_rotation = (m_rotation + m_rotationStep + 360) % 360; }

private:
    const int m_rotationStep = 45;
    int m_rotation = 0;
    QTimer m_timer;
    QPixmap m_pixmap;
    UpdateCallback m_callback;
};

ProgressIndicatorPainter::ProgressIndicatorPainter()
{
    m_timer.setSingleShot(false);
    QObject::connect(&m_timer, &QTimer::timeout, &m_timer, [this] {
        nextAnimationStep();
        if (m_callback)
            m_callback();
    });

    m_timer.setInterval(100);
    m_pixmap = QPixmap(":/icons/progressindicator.png");
}

void ProgressIndicatorPainter::paint(QPainter &painter, const QRect &rect) const
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

class ProgressIndicatorWidget : public OverlayWidget
{
public:
    explicit ProgressIndicatorWidget(QWidget *parent = nullptr)
        : OverlayWidget(parent)
    {
        setPaintFunction(
            [this](QWidget *w, QPainter &p, QPaintEvent *) { m_paint.paint(p, w->rect()); });
        m_paint.setUpdateCallback([this] { update(); });
        updateGeometry();
    }

    QSize sizeHint() const final { return m_paint.size(); }

protected:
    void showEvent(QShowEvent *) final { m_paint.startAnimation(); }
    void hideEvent(QHideEvent *) final { m_paint.stopAnimation(); }

private:
    ProgressIndicatorPainter m_paint;
};

ProgressIndicator::ProgressIndicator(QWidget *parent)
    : QObject(parent)
    , m_widget(new ProgressIndicatorWidget(parent)) {}


void ProgressIndicator::show()
{
    m_widget->show();
}

void ProgressIndicator::hide()
{
    m_widget->hide();
}

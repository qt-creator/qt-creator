// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "trafficlight.h"

#include "glueinterface.h"

#include <QAbstractButton>
#include <QBoxLayout>
#include <QPainter>

using namespace Qt::Literals::StringLiterals;

class LightWidget final : public QWidget
{
public:
    LightWidget(const QString &image, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_image(image)
    {}

    void setOn(bool on)
    {
        if (on == m_on)
            return;
        m_on = on;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        if (!m_on)
            return;
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawImage(0, 0, m_image);
    }
    QSize sizeHint() const override { return m_image.size(); }

private:
    QImage m_image;
    bool m_on = false;
};

class ButtonWidget final : public QAbstractButton
{
public:
    ButtonWidget(QWidget *parent = nullptr)
        : QAbstractButton(parent), m_playIcon(":/play.png"_L1)
        , m_pauseIcon(":/pause.png"_L1)
    {
        setCheckable(true);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawImage(0, 0, isChecked() ? m_playIcon : m_pauseIcon);
    }
    QSize sizeHint() const override { return isChecked() ? m_playIcon.size() : m_pauseIcon.size(); }

private:
    QImage m_playIcon;
    QImage m_pauseIcon;
};

class TrafficLightWidget final : public QWidget
{
public:
    TrafficLightWidget(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_background(":/background.png"_L1)
    {
        QVBoxLayout *vbox = new QVBoxLayout(this);
        vbox->setContentsMargins(0, 40, 0, 80);
        m_red = new LightWidget(":/red.png"_L1);
        vbox->addWidget(m_red, 0, Qt::AlignHCenter);
        m_yellow = new LightWidget(":/yellow.png"_L1);
        vbox->addWidget(m_yellow, 0, Qt::AlignHCenter);
        m_green = new LightWidget(":/green.png"_L1);
        vbox->addWidget(m_green, 0, Qt::AlignHCenter);
        setLayout(vbox);
    }

    LightWidget *redLight() const { return m_red; }
    LightWidget *yellowLight() const { return m_yellow; }
    LightWidget *greenLight() const { return m_green; }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawImage(0, 0, m_background);
    }

    QSize sizeHint() const override { return m_background.size(); }

private:
    QImage m_background;
    LightWidget *m_red;
    LightWidget *m_yellow;
    LightWidget *m_green;
};

TrafficLight::TrafficLight(GlueInterface *iface)
{
    TrafficLightWidget *widget = new TrafficLightWidget(this);
    setFixedSize(widget->sizeHint());

    QAbstractButton *button = new ButtonWidget(this);
    auto setButtonGeometry = [this, button] {
        const QSize buttonSize = button->sizeHint();
        button->setGeometry(width() - buttonSize.width() - 20,
                            height() - buttonSize.height() - 20,
                            buttonSize.width(), buttonSize.height());
    };
    connect(button, &QAbstractButton::toggled, this, setButtonGeometry);
    setButtonGeometry();

    connect(iface, &GlueInterface::redChanged, widget->redLight(), &LightWidget::setOn);
    connect(iface, &GlueInterface::yellowChanged, widget->yellowLight(), &LightWidget::setOn);
    connect(iface, &GlueInterface::greenChanged, widget->greenLight(), &LightWidget::setOn);

    connect(button, &QAbstractButton::toggled, this, [iface](bool pause) {
        pause ? iface->smash() : iface->repair();
    });
}

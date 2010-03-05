/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtgradientstopscontroller.h"
#include "ui_qtgradienteditor.h"
#include "qtgradientstopsmodel.h"

#include <QtCore/QTimer>

QT_BEGIN_NAMESPACE

class QtGradientStopsControllerPrivate
{
    QtGradientStopsController *q_ptr;
    Q_DECLARE_PUBLIC(QtGradientStopsController)
public:
    typedef QMap<qreal, QColor> PositionColorMap;
    typedef QMap<qreal, QtGradientStop *> PositionStopMap;

    void slotHsvClicked();
    void slotRgbClicked();

    void slotCurrentStopChanged(QtGradientStop *stop);
    void slotStopMoved(QtGradientStop *stop, qreal newPos);
    void slotStopsSwapped(QtGradientStop *stop1, QtGradientStop *stop2);
    void slotStopChanged(QtGradientStop *stop, const QColor &newColor);
    void slotStopSelected(QtGradientStop *stop, bool selected);
    void slotStopAdded(QtGradientStop *stop);
    void slotStopRemoved(QtGradientStop *stop);
    void slotUpdatePositionSpinBox();

    void slotChangeColor(const QColor &color);
    void slotChangeHue(const QColor &color);
    void slotChangeSaturation(const QColor &color);
    void slotChangeValue(const QColor &color);
    void slotChangeAlpha(const QColor &color);
    void slotChangeHue(int color);
    void slotChangeSaturation(int color);
    void slotChangeValue(int color);
    void slotChangeAlpha(int color);
    void slotChangePosition(double value);

    void slotChangeZoom(int value);
    void slotZoomIn();
    void slotZoomOut();
    void slotZoomAll();
    void slotZoomChanged(double zoom);

    void enableCurrent(bool enable);
    void setColorSpinBoxes(const QColor &color);
    PositionColorMap stopsData(const PositionStopMap &stops) const;
    QGradientStops makeGradientStops(const PositionColorMap &data) const;
    void updateZoom(double zoom);

    QtGradientStopsModel *m_model;
    QColor::Spec m_spec;

    Ui::QtGradientEditor *m_ui;
};

void QtGradientStopsControllerPrivate::enableCurrent(bool enable)
{
    m_ui->positionLabel->setEnabled(enable);
    m_ui->colorLabel->setEnabled(enable);
    m_ui->hLabel->setEnabled(enable);
    m_ui->sLabel->setEnabled(enable);
    m_ui->vLabel->setEnabled(enable);
    m_ui->aLabel->setEnabled(enable);
    m_ui->hueLabel->setEnabled(enable);
    m_ui->saturationLabel->setEnabled(enable);
    m_ui->valueLabel->setEnabled(enable);
    m_ui->alphaLabel->setEnabled(enable);

    m_ui->positionSpinBox->setEnabled(enable);
    m_ui->colorButton->setEnabled(enable);

    m_ui->hueColorLine->setEnabled(enable);
    m_ui->saturationColorLine->setEnabled(enable);
    m_ui->valueColorLine->setEnabled(enable);
    m_ui->alphaColorLine->setEnabled(enable);

    m_ui->hueSpinBox->setEnabled(enable);
    m_ui->saturationSpinBox->setEnabled(enable);
    m_ui->valueSpinBox->setEnabled(enable);
    m_ui->alphaSpinBox->setEnabled(enable);
}

QtGradientStopsControllerPrivate::PositionColorMap QtGradientStopsControllerPrivate::stopsData(const PositionStopMap &stops) const
{
    PositionColorMap data;
    PositionStopMap::ConstIterator itStop = stops.constBegin();
    while (itStop != stops.constEnd()) {
        QtGradientStop *stop = itStop.value();
        data[stop->position()] = stop->color();

        ++itStop;
    }
    return data;
}

QGradientStops QtGradientStopsControllerPrivate::makeGradientStops(const PositionColorMap &data) const
{
    QGradientStops stops;
    PositionColorMap::ConstIterator itData = data.constBegin();
    while (itData != data.constEnd()) {
        stops << QPair<qreal, QColor>(itData.key(), itData.value());

        ++itData;
    }
    return stops;
}

void QtGradientStopsControllerPrivate::updateZoom(double zoom)
{
    m_ui->gradientStopsWidget->setZoom(zoom);
    m_ui->zoomSpinBox->blockSignals(true);
    m_ui->zoomSpinBox->setValue(qRound(zoom * 100));
    m_ui->zoomSpinBox->blockSignals(false);
    bool zoomInEnabled = true;
    bool zoomOutEnabled = true;
    bool zoomAllEnabled = true;
    if (zoom <= 1) {
        zoomAllEnabled = false;
        zoomOutEnabled = false;
    } else if (zoom >= 100) {
        zoomInEnabled = false;
    }
    m_ui->zoomInButton->setEnabled(zoomInEnabled);
    m_ui->zoomOutButton->setEnabled(zoomOutEnabled);
    m_ui->zoomAllButton->setEnabled(zoomAllEnabled);
}

void QtGradientStopsControllerPrivate::slotHsvClicked()
{
    QString h = QApplication::translate("qdesigner_internal::QtGradientStopsController", "H", 0, QApplication::UnicodeUTF8);
    QString s = QApplication::translate("qdesigner_internal::QtGradientStopsController", "S", 0, QApplication::UnicodeUTF8);
    QString v = QApplication::translate("qdesigner_internal::QtGradientStopsController", "V", 0, QApplication::UnicodeUTF8);

    m_ui->hLabel->setText(h);
    m_ui->sLabel->setText(s);
    m_ui->vLabel->setText(v);

    h = QApplication::translate("qdesigner_internal::QtGradientStopsController", "Hue", 0, QApplication::UnicodeUTF8);
    s = QApplication::translate("qdesigner_internal::QtGradientStopsController", "Sat", 0, QApplication::UnicodeUTF8);
    v = QApplication::translate("qdesigner_internal::QtGradientStopsController", "Val", 0, QApplication::UnicodeUTF8);

    const QString hue = QApplication::translate("qdesigner_internal::QtGradientStopsController", "Hue", 0, QApplication::UnicodeUTF8);
    const QString saturation = QApplication::translate("qdesigner_internal::QtGradientStopsController", "Saturation", 0, QApplication::UnicodeUTF8);
    const QString value = QApplication::translate("qdesigner_internal::QtGradientStopsController", "Value", 0, QApplication::UnicodeUTF8);

    m_ui->hLabel->setToolTip(hue);
    m_ui->hueLabel->setText(h);
    m_ui->hueColorLine->setToolTip(hue);
    m_ui->hueColorLine->setColorComponent(QtColorLine::Hue);

    m_ui->sLabel->setToolTip(saturation);
    m_ui->saturationLabel->setText(s);
    m_ui->saturationColorLine->setToolTip(saturation);
    m_ui->saturationColorLine->setColorComponent(QtColorLine::Saturation);

    m_ui->vLabel->setToolTip(value);
    m_ui->valueLabel->setText(v);
    m_ui->valueColorLine->setToolTip(value);
    m_ui->valueColorLine->setColorComponent(QtColorLine::Value);

    setColorSpinBoxes(m_ui->colorButton->color());
}

void QtGradientStopsControllerPrivate::slotRgbClicked()
{
    QString r = QApplication::translate("qdesigner_internal::QtGradientStopsController", "R", 0, QApplication::UnicodeUTF8);
    QString g = QApplication::translate("qdesigner_internal::QtGradientStopsController", "G", 0, QApplication::UnicodeUTF8);
    QString b = QApplication::translate("qdesigner_internal::QtGradientStopsController", "B", 0, QApplication::UnicodeUTF8);

    m_ui->hLabel->setText(r);
    m_ui->sLabel->setText(g);
    m_ui->vLabel->setText(b);

    QString red = QApplication::translate("qdesigner_internal::QtGradientStopsController", "Red", 0, QApplication::UnicodeUTF8);
    QString green = QApplication::translate("qdesigner_internal::QtGradientStopsController", "Green", 0, QApplication::UnicodeUTF8);
    QString blue = QApplication::translate("qdesigner_internal::QtGradientStopsController", "Blue", 0, QApplication::UnicodeUTF8);

    m_ui->hLabel->setToolTip(red);
    m_ui->hueLabel->setText(red);
    m_ui->hueColorLine->setToolTip(red);
    m_ui->hueColorLine->setColorComponent(QtColorLine::Red);

    m_ui->sLabel->setToolTip(green);
    m_ui->saturationLabel->setText(green);
    m_ui->saturationColorLine->setToolTip(green);
    m_ui->saturationColorLine->setColorComponent(QtColorLine::Green);

    m_ui->vLabel->setToolTip(blue);
    m_ui->valueLabel->setText(blue);
    m_ui->valueColorLine->setToolTip(blue);
    m_ui->valueColorLine->setColorComponent(QtColorLine::Blue);

    setColorSpinBoxes(m_ui->colorButton->color());
}

void QtGradientStopsControllerPrivate::setColorSpinBoxes(const QColor &color)
{
    m_ui->hueSpinBox->blockSignals(true);
    m_ui->saturationSpinBox->blockSignals(true);
    m_ui->valueSpinBox->blockSignals(true);
    m_ui->alphaSpinBox->blockSignals(true);
    if (m_ui->hsvRadioButton->isChecked()) {
        if (m_ui->hueSpinBox->maximum() != 359)
            m_ui->hueSpinBox->setMaximum(359);
        if (m_ui->hueSpinBox->value() != color.hue())
            m_ui->hueSpinBox->setValue(color.hue());
        if (m_ui->saturationSpinBox->value() != color.saturation())
            m_ui->saturationSpinBox->setValue(color.saturation());
        if (m_ui->valueSpinBox->value() != color.value())
            m_ui->valueSpinBox->setValue(color.value());
    } else {
        if (m_ui->hueSpinBox->maximum() != 255)
            m_ui->hueSpinBox->setMaximum(255);
        if (m_ui->hueSpinBox->value() != color.red())
            m_ui->hueSpinBox->setValue(color.red());
        if (m_ui->saturationSpinBox->value() != color.green())
            m_ui->saturationSpinBox->setValue(color.green());
        if (m_ui->valueSpinBox->value() != color.blue())
            m_ui->valueSpinBox->setValue(color.blue());
    }
    m_ui->alphaSpinBox->setValue(color.alpha());
    m_ui->hueSpinBox->blockSignals(false);
    m_ui->saturationSpinBox->blockSignals(false);
    m_ui->valueSpinBox->blockSignals(false);
    m_ui->alphaSpinBox->blockSignals(false);
}

void QtGradientStopsControllerPrivate::slotCurrentStopChanged(QtGradientStop *stop)
{
    if (!stop) {
        enableCurrent(false);
        return;
    }
    enableCurrent(true);

    QTimer::singleShot(0, q_ptr, SLOT(slotUpdatePositionSpinBox()));

    m_ui->colorButton->setColor(stop->color());
    m_ui->hueColorLine->setColor(stop->color());
    m_ui->saturationColorLine->setColor(stop->color());
    m_ui->valueColorLine->setColor(stop->color());
    m_ui->alphaColorLine->setColor(stop->color());
    setColorSpinBoxes(stop->color());
}

void QtGradientStopsControllerPrivate::slotStopMoved(QtGradientStop *stop, qreal newPos)
{
    QTimer::singleShot(0, q_ptr, SLOT(slotUpdatePositionSpinBox()));

    PositionColorMap stops = stopsData(m_model->stops());
    stops.remove(stop->position());
    stops[newPos] = stop->color();

    QGradientStops gradStops = makeGradientStops(stops);
    emit q_ptr->gradientStopsChanged(gradStops);
}

void QtGradientStopsControllerPrivate::slotStopsSwapped(QtGradientStop *stop1, QtGradientStop *stop2)
{
    QTimer::singleShot(0, q_ptr, SLOT(slotUpdatePositionSpinBox()));

    PositionColorMap stops = stopsData(m_model->stops());
    const qreal pos1 = stop1->position();
    const qreal pos2 = stop2->position();
    stops[pos1] = stop2->color();
    stops[pos2] = stop1->color();

    QGradientStops gradStops = makeGradientStops(stops);
    emit q_ptr->gradientStopsChanged(gradStops);
}

void QtGradientStopsControllerPrivate::slotStopAdded(QtGradientStop *stop)
{
    PositionColorMap stops = stopsData(m_model->stops());
    stops[stop->position()] = stop->color();

    QGradientStops gradStops = makeGradientStops(stops);
    emit q_ptr->gradientStopsChanged(gradStops);
}

void QtGradientStopsControllerPrivate::slotStopRemoved(QtGradientStop *stop)
{
    PositionColorMap stops = stopsData(m_model->stops());
    stops.remove(stop->position());

    QGradientStops gradStops = makeGradientStops(stops);
    emit q_ptr->gradientStopsChanged(gradStops);
}

void QtGradientStopsControllerPrivate::slotStopChanged(QtGradientStop *stop, const QColor &newColor)
{
    if (m_model->currentStop() == stop) {
        m_ui->colorButton->setColor(newColor);
        m_ui->hueColorLine->setColor(newColor);
        m_ui->saturationColorLine->setColor(newColor);
        m_ui->valueColorLine->setColor(newColor);
        m_ui->alphaColorLine->setColor(newColor);
        setColorSpinBoxes(newColor);
    }

    PositionColorMap stops = stopsData(m_model->stops());
    stops[stop->position()] = newColor;

    QGradientStops gradStops = makeGradientStops(stops);
    emit q_ptr->gradientStopsChanged(gradStops);
}

void QtGradientStopsControllerPrivate::slotStopSelected(QtGradientStop *stop, bool selected)
{
    Q_UNUSED(stop)
    Q_UNUSED(selected)
    QTimer::singleShot(0, q_ptr, SLOT(slotUpdatePositionSpinBox()));
}

void QtGradientStopsControllerPrivate::slotUpdatePositionSpinBox()
{
    QtGradientStop *current = m_model->currentStop();
    if (!current)
        return;

    qreal min = 0.0;
    qreal max = 1.0;
    const qreal pos = current->position();

    QtGradientStop *first = m_model->firstSelected();
    QtGradientStop *last = m_model->lastSelected();

    if (first && last) {
        const qreal minPos = pos - first->position() - 0.0004999;
        const qreal maxPos = pos + 1.0 - last->position() + 0.0004999;

        if (max > maxPos)
            max = maxPos;
        if (min < minPos)
            min = minPos;

        if (first->position() == 0.0)
            min = pos;
        if (last->position() == 1.0)
            max = pos;
    }

    const int spinMin = qRound(m_ui->positionSpinBox->minimum() * 1000);
    const int spinMax = qRound(m_ui->positionSpinBox->maximum() * 1000);

    const int newMin = qRound(min * 1000);
    const int newMax = qRound(max * 1000);

    m_ui->positionSpinBox->blockSignals(true);
    if (spinMin != newMin || spinMax != newMax) {
        m_ui->positionSpinBox->setRange((double)newMin / 1000, (double)newMax / 1000);
    }
    if (m_ui->positionSpinBox->value() != pos)
        m_ui->positionSpinBox->setValue(pos);
    m_ui->positionSpinBox->blockSignals(false);
}

void QtGradientStopsControllerPrivate::slotChangeColor(const QColor &color)
{
    QtGradientStop *stop = m_model->currentStop();
    if (!stop)
        return;
    m_model->changeStop(stop, color);
    QList<QtGradientStop *> stops = m_model->selectedStops();
    QListIterator<QtGradientStop *> itStop(stops);
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (s != stop)
            m_model->changeStop(s, color);
    }
}

void QtGradientStopsControllerPrivate::slotChangeHue(const QColor &color)
{
    QtGradientStop *stop = m_model->currentStop();
    if (!stop)
        return;
    m_model->changeStop(stop, color);
    QList<QtGradientStop *> stops = m_model->selectedStops();
    QListIterator<QtGradientStop *> itStop(stops);
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (s != stop) {
            QColor c = s->color();
            if (m_ui->hsvRadioButton->isChecked())
                c.setHsvF(color.hueF(), c.saturationF(), c.valueF(), c.alphaF());
            else
                c.setRgbF(color.redF(), c.greenF(), c.blueF(), c.alphaF());
            m_model->changeStop(s, c);
        }
    }
}

void QtGradientStopsControllerPrivate::slotChangeHue(int color)
{
    QColor c = m_ui->hueColorLine->color();
    if (m_ui->hsvRadioButton->isChecked())
        c.setHsvF((qreal)color / 360.0, c.saturationF(), c.valueF(), c.alphaF());
    else
        c.setRed(color);
    slotChangeHue(c);
}

void QtGradientStopsControllerPrivate::slotChangeSaturation(const QColor &color)
{
    QtGradientStop *stop = m_model->currentStop();
    if (!stop)
        return;
    m_model->changeStop(stop, color);
    QList<QtGradientStop *> stops = m_model->selectedStops();
    QListIterator<QtGradientStop *> itStop(stops);
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (s != stop) {
            QColor c = s->color();
            if (m_ui->hsvRadioButton->isChecked()) {
                c.setHsvF(c.hueF(), color.saturationF(), c.valueF(), c.alphaF());
                int hue = c.hue();
                if (hue == 360 || hue == -1)
                    c.setHsvF(0.0, c.saturationF(), c.valueF(), c.alphaF());
            } else {
                c.setRgbF(c.redF(), color.greenF(), c.blueF(), c.alphaF());
            }
            m_model->changeStop(s, c);
        }
    }
}

void QtGradientStopsControllerPrivate::slotChangeSaturation(int color)
{
    QColor c = m_ui->saturationColorLine->color();
    if (m_ui->hsvRadioButton->isChecked())
        c.setHsvF(c.hueF(), (qreal)color / 255, c.valueF(), c.alphaF());
    else
        c.setGreen(color);
    slotChangeSaturation(c);
}

void QtGradientStopsControllerPrivate::slotChangeValue(const QColor &color)
{
    QtGradientStop *stop = m_model->currentStop();
    if (!stop)
        return;
    m_model->changeStop(stop, color);
    QList<QtGradientStop *> stops = m_model->selectedStops();
    QListIterator<QtGradientStop *> itStop(stops);
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (s != stop) {
            QColor c = s->color();
            if (m_ui->hsvRadioButton->isChecked()) {
                c.setHsvF(c.hueF(), c.saturationF(), color.valueF(), c.alphaF());
                int hue = c.hue();
                if (hue == 360 || hue == -1)
                    c.setHsvF(0.0, c.saturationF(), c.valueF(), c.alphaF());
            } else {
                c.setRgbF(c.redF(), c.greenF(), color.blueF(), c.alphaF());
            }
            m_model->changeStop(s, c);
        }
    }
}

void QtGradientStopsControllerPrivate::slotChangeValue(int color)
{
    QColor c = m_ui->valueColorLine->color();
    if (m_ui->hsvRadioButton->isChecked())
        c.setHsvF(c.hueF(), c.saturationF(), (qreal)color / 255, c.alphaF());
    else
        c.setBlue(color);
    slotChangeValue(c);
}

void QtGradientStopsControllerPrivate::slotChangeAlpha(const QColor &color)
{
    QtGradientStop *stop = m_model->currentStop();
    if (!stop)
        return;
    m_model->changeStop(stop, color);
    QList<QtGradientStop *> stops = m_model->selectedStops();
    QListIterator<QtGradientStop *> itStop(stops);
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (s != stop) {
            QColor c = s->color();
            if (m_ui->hsvRadioButton->isChecked()) {
                c.setHsvF(c.hueF(), c.saturationF(), c.valueF(), color.alphaF());
                int hue = c.hue();
                if (hue == 360 || hue == -1)
                    c.setHsvF(0.0, c.saturationF(), c.valueF(), c.alphaF());
            } else {
                c.setRgbF(c.redF(), c.greenF(), c.blueF(), color.alphaF());
            }
            m_model->changeStop(s, c);
        }
    }
}

void QtGradientStopsControllerPrivate::slotChangeAlpha(int color)
{
    QColor c = m_ui->alphaColorLine->color();
    if (m_ui->hsvRadioButton->isChecked())
        c.setHsvF(c.hueF(), c.saturationF(), c.valueF(), (qreal)color / 255);
    else
        c.setAlpha(color);
    slotChangeAlpha(c);
}

void QtGradientStopsControllerPrivate::slotChangePosition(double value)
{
    QtGradientStop *stop = m_model->currentStop();
    if (!stop)
        return;

    m_model->moveStops(value);
}

void QtGradientStopsControllerPrivate::slotChangeZoom(int value)
{
    updateZoom(value / 100.0);
}

void QtGradientStopsControllerPrivate::slotZoomIn()
{
    double newZoom = m_ui->gradientStopsWidget->zoom() * 2;
    if (newZoom > 100)
        newZoom = 100;
    updateZoom(newZoom);
}

void QtGradientStopsControllerPrivate::slotZoomOut()
{
    double newZoom = m_ui->gradientStopsWidget->zoom() / 2;
    if (newZoom < 1)
        newZoom = 1;
    updateZoom(newZoom);
}

void QtGradientStopsControllerPrivate::slotZoomAll()
{
    updateZoom(1);
}

void QtGradientStopsControllerPrivate::slotZoomChanged(double zoom)
{
    updateZoom(zoom);
}

QtGradientStopsController::QtGradientStopsController(QObject *parent)
    : QObject(parent), d_ptr(new QtGradientStopsControllerPrivate())
{
    d_ptr->q_ptr = this;

    d_ptr->m_spec = QColor::Hsv;
}

void QtGradientStopsController::setUi(Ui::QtGradientEditor *ui)
{
    d_ptr->m_ui = ui;

    d_ptr->m_ui->hueColorLine->setColorComponent(QtColorLine::Hue);
    d_ptr->m_ui->saturationColorLine->setColorComponent(QtColorLine::Saturation);
    d_ptr->m_ui->valueColorLine->setColorComponent(QtColorLine::Value);
    d_ptr->m_ui->alphaColorLine->setColorComponent(QtColorLine::Alpha);

    d_ptr->m_model = new QtGradientStopsModel(this);
    d_ptr->m_ui->gradientStopsWidget->setGradientStopsModel(d_ptr->m_model);
    connect(d_ptr->m_model, SIGNAL(currentStopChanged(QtGradientStop*)),
                this, SLOT(slotCurrentStopChanged(QtGradientStop*)));
    connect(d_ptr->m_model, SIGNAL(stopMoved(QtGradientStop*,qreal)),
                this, SLOT(slotStopMoved(QtGradientStop*,qreal)));
    connect(d_ptr->m_model, SIGNAL(stopsSwapped(QtGradientStop*,QtGradientStop*)),
                this, SLOT(slotStopsSwapped(QtGradientStop*,QtGradientStop*)));
    connect(d_ptr->m_model, SIGNAL(stopChanged(QtGradientStop*,QColor)),
                this, SLOT(slotStopChanged(QtGradientStop*,QColor)));
    connect(d_ptr->m_model, SIGNAL(stopSelected(QtGradientStop*,bool)),
                this, SLOT(slotStopSelected(QtGradientStop*,bool)));
    connect(d_ptr->m_model, SIGNAL(stopAdded(QtGradientStop*)),
                this, SLOT(slotStopAdded(QtGradientStop*)));
    connect(d_ptr->m_model, SIGNAL(stopRemoved(QtGradientStop*)),
                this, SLOT(slotStopRemoved(QtGradientStop*)));

    connect(d_ptr->m_ui->hueColorLine, SIGNAL(colorChanged(QColor)),
                this, SLOT(slotChangeHue(QColor)));
    connect(d_ptr->m_ui->saturationColorLine, SIGNAL(colorChanged(QColor)),
                this, SLOT(slotChangeSaturation(QColor)));
    connect(d_ptr->m_ui->valueColorLine, SIGNAL(colorChanged(QColor)),
                this, SLOT(slotChangeValue(QColor)));
    connect(d_ptr->m_ui->alphaColorLine, SIGNAL(colorChanged(QColor)),
                this, SLOT(slotChangeAlpha(QColor)));
    connect(d_ptr->m_ui->colorButton, SIGNAL(colorChanged(QColor)),
                this, SLOT(slotChangeColor(QColor)));

    connect(d_ptr->m_ui->hueSpinBox, SIGNAL(valueChanged(int)),
                this, SLOT(slotChangeHue(int)));
    connect(d_ptr->m_ui->saturationSpinBox, SIGNAL(valueChanged(int)),
                this, SLOT(slotChangeSaturation(int)));
    connect(d_ptr->m_ui->valueSpinBox, SIGNAL(valueChanged(int)),
                this, SLOT(slotChangeValue(int)));
    connect(d_ptr->m_ui->alphaSpinBox, SIGNAL(valueChanged(int)),
                this, SLOT(slotChangeAlpha(int)));

    connect(d_ptr->m_ui->positionSpinBox, SIGNAL(valueChanged(double)),
                this, SLOT(slotChangePosition(double)));

    connect(d_ptr->m_ui->zoomSpinBox, SIGNAL(valueChanged(int)),
                this, SLOT(slotChangeZoom(int)));
    connect(d_ptr->m_ui->zoomInButton, SIGNAL(clicked()),
                this, SLOT(slotZoomIn()));
    connect(d_ptr->m_ui->zoomOutButton, SIGNAL(clicked()),
                this, SLOT(slotZoomOut()));
    connect(d_ptr->m_ui->zoomAllButton, SIGNAL(clicked()),
                this, SLOT(slotZoomAll()));
    connect(d_ptr->m_ui->gradientStopsWidget, SIGNAL(zoomChanged(double)),
                this, SLOT(slotZoomChanged(double)));

    connect(d_ptr->m_ui->hsvRadioButton, SIGNAL(clicked()),
                this, SLOT(slotHsvClicked()));
    connect(d_ptr->m_ui->rgbRadioButton, SIGNAL(clicked()),
                this, SLOT(slotRgbClicked()));

    d_ptr->enableCurrent(false);
    d_ptr->m_ui->zoomInButton->setIcon(QIcon(QLatin1String(":/trolltech/qtgradienteditor/images/zoomin.png")));
    d_ptr->m_ui->zoomOutButton->setIcon(QIcon(QLatin1String(":/trolltech/qtgradienteditor/images/zoomout.png")));
    d_ptr->updateZoom(1);
}

QtGradientStopsController::~QtGradientStopsController()
{
}

void QtGradientStopsController::setGradientStops(const QGradientStops &stops)
{
    d_ptr->m_model->clear();
    QVectorIterator<QPair<qreal, QColor> > it(stops);
    QtGradientStop *first = 0;
    while (it.hasNext()) {
        QPair<qreal, QColor> pair = it.next();
        QtGradientStop *stop = d_ptr->m_model->addStop(pair.first, pair.second);
        if (!first)
            first = stop;
    }
    if (first)
        d_ptr->m_model->setCurrentStop(first);
}

QGradientStops QtGradientStopsController::gradientStops() const
{
    QGradientStops stops;
    QList<QtGradientStop *> stopsList = d_ptr->m_model->stops().values();
    QListIterator<QtGradientStop *> itStop(stopsList);
    while (itStop.hasNext()) {
        QtGradientStop *stop = itStop.next();
        stops << QPair<qreal, QColor>(stop->position(), stop->color());
    }
    return stops;
}

QColor::Spec QtGradientStopsController::spec() const
{
    return d_ptr->m_spec;
}

void QtGradientStopsController::setSpec(QColor::Spec spec)
{
    if (d_ptr->m_spec == spec)
        return;

    d_ptr->m_spec = spec;
    if (d_ptr->m_spec == QColor::Rgb) {
        d_ptr->m_ui->rgbRadioButton->setChecked(true);
        d_ptr->slotRgbClicked();
    } else {
        d_ptr->m_ui->hsvRadioButton->setChecked(true);
        d_ptr->slotHsvClicked();
    }
}

QT_END_NAMESPACE

#include "moc_qtgradientstopscontroller.cpp"

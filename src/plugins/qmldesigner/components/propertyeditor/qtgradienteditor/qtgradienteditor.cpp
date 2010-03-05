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

#include "qtgradienteditor.h"
#include "qtgradientstopscontroller.h"
#include "ui_qtgradienteditor.h"

QT_BEGIN_NAMESPACE

class QtGradientEditorPrivate
{
    QtGradientEditor *q_ptr;
    Q_DECLARE_PUBLIC(QtGradientEditor)
public:
    QtGradientEditorPrivate() : m_gradient(QLinearGradient()) {}

    void slotGradientStopsChanged(const QGradientStops &stops);
    void slotTypeChanged(int type);
    void slotSpreadChanged(int spread);
    void slotStartLinearXChanged(double value);
    void slotStartLinearYChanged(double value);
    void slotEndLinearXChanged(double value);
    void slotEndLinearYChanged(double value);
    void slotCentralRadialXChanged(double value);
    void slotCentralRadialYChanged(double value);
    void slotFocalRadialXChanged(double value);
    void slotFocalRadialYChanged(double value);
    void slotRadiusRadialChanged(double value);
    void slotCentralConicalXChanged(double value);
    void slotCentralConicalYChanged(double value);
    void slotAngleConicalChanged(double value);

    void slotDetailsChanged(bool details);

    void startLinearChanged(const QPointF &point);
    void endLinearChanged(const QPointF &point);
    void centralRadialChanged(const QPointF &point);
    void focalRadialChanged(const QPointF &point);
    void radiusRadialChanged(qreal radius);
    void centralConicalChanged(const QPointF &point);
    void angleConicalChanged(qreal angle);

    void setStartLinear(const QPointF &point);
    void setEndLinear(const QPointF &point);
    void setCentralRadial(const QPointF &point);
    void setFocalRadial(const QPointF &point);
    void setRadiusRadial(qreal radius);
    void setCentralConical(const QPointF &point);
    void setAngleConical(qreal angle);

    void setType(QGradient::Type type);
    void showDetails(bool details);

    void setSpinBox(QDoubleSpinBox *spinBox, const char *slot, double max = 1.0, double step = 0.01, int decimals = 3);
    void reset();
    void setLayout(bool details);
    void layoutDetails(bool details);
    bool row4Visible() const;
    bool row5Visible() const;
    int extensionWidthHint() const;

    void setCombos(bool combos);

    QGradient gradient() const;
    void updateGradient(bool emitSignal);

    Ui::QtGradientEditor m_ui;
    QtGradientStopsController *m_gradientStopsController;

    QDoubleSpinBox *startLinearXSpinBox;
    QDoubleSpinBox *startLinearYSpinBox;
    QDoubleSpinBox *endLinearXSpinBox;
    QDoubleSpinBox *endLinearYSpinBox;
    QDoubleSpinBox *centralRadialXSpinBox;
    QDoubleSpinBox *centralRadialYSpinBox;
    QDoubleSpinBox *focalRadialXSpinBox;
    QDoubleSpinBox *focalRadialYSpinBox;
    QDoubleSpinBox *radiusRadialSpinBox;
    QDoubleSpinBox *centralConicalXSpinBox;
    QDoubleSpinBox *centralConicalYSpinBox;
    QDoubleSpinBox *angleConicalSpinBox;

    QButtonGroup *m_typeGroup;
    QButtonGroup *m_spreadGroup;

    QGradient::Type m_type;

    QGridLayout *m_gridLayout;
    QWidget *m_hiddenWidget;
    QGridLayout *m_hiddenLayout;
    bool m_details;
    bool m_detailsButtonVisible;
    bool m_backgroundCheckered;

    QGradient m_gradient;

    bool m_combos;
};

QGradient QtGradientEditorPrivate::gradient() const
{
    QGradient *gradient = 0;
    switch (m_ui.gradientWidget->gradientType()) {
        case QGradient::LinearGradient:
            gradient = new QLinearGradient(m_ui.gradientWidget->startLinear(),
                        m_ui.gradientWidget->endLinear());
            break;
        case QGradient::RadialGradient:
            gradient = new QRadialGradient(m_ui.gradientWidget->centralRadial(),
                        m_ui.gradientWidget->radiusRadial(),
                        m_ui.gradientWidget->focalRadial());
            break;
        case QGradient::ConicalGradient:
            gradient = new QConicalGradient(m_ui.gradientWidget->centralConical(),
                        m_ui.gradientWidget->angleConical());
            break;
        default:
            break;
    }
    if (!gradient)
        return QGradient();
    gradient->setStops(m_ui.gradientWidget->gradientStops());
    gradient->setSpread(m_ui.gradientWidget->gradientSpread());
    gradient->setCoordinateMode(QGradient::StretchToDeviceMode);
    QGradient gr = *gradient;
    delete gradient;
    return gr;
}

void QtGradientEditorPrivate::updateGradient(bool emitSignal)
{
    QGradient grad = gradient();
    if (m_gradient == grad)
        return;

    m_gradient = grad;
    if (emitSignal)
        emit q_ptr->gradientChanged(m_gradient);
}

void QtGradientEditorPrivate::setCombos(bool combos)
{
    if (m_combos == combos)
        return;

    m_combos = combos;
    m_ui.linearButton->setVisible(!m_combos);
    m_ui.radialButton->setVisible(!m_combos);
    m_ui.conicalButton->setVisible(!m_combos);
    m_ui.padButton->setVisible(!m_combos);
    m_ui.repeatButton->setVisible(!m_combos);
    m_ui.reflectButton->setVisible(!m_combos);
    m_ui.typeComboBox->setVisible(m_combos);
    m_ui.spreadComboBox->setVisible(m_combos);
}

void QtGradientEditorPrivate::setLayout(bool details)
{
    QHBoxLayout *hboxLayout = new QHBoxLayout();
    hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
    hboxLayout->addWidget(m_ui.typeComboBox);
    hboxLayout->addWidget(m_ui.spreadComboBox);
    QHBoxLayout *typeLayout = new QHBoxLayout();
    typeLayout->setSpacing(0);
    typeLayout->addWidget(m_ui.linearButton);
    typeLayout->addWidget(m_ui.radialButton);
    typeLayout->addWidget(m_ui.conicalButton);
    hboxLayout->addLayout(typeLayout);
    QHBoxLayout *spreadLayout = new QHBoxLayout();
    spreadLayout->setSpacing(0);
    spreadLayout->addWidget(m_ui.padButton);
    spreadLayout->addWidget(m_ui.repeatButton);
    spreadLayout->addWidget(m_ui.reflectButton);
    hboxLayout->addLayout(spreadLayout);
    hboxLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    hboxLayout->addWidget(m_ui.detailsButton);
    m_gridLayout->addLayout(hboxLayout, 0, 0, 1, 2);
    int span = 1;
    if (details)
        span = 7;
    m_gridLayout->addWidget(m_ui.frame, 1, 0, span, 2);
    int row = 2;
    if (details) {
        row = 8;
        span = 4;
    }
    m_gridLayout->addWidget(m_ui.gradientStopsWidget, row, 0, span, 2);
    QHBoxLayout *hboxLayout1 = new QHBoxLayout();
    hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
    hboxLayout1->addWidget(m_ui.colorLabel);
    hboxLayout1->addWidget(m_ui.colorButton);
    hboxLayout1->addWidget(m_ui.hsvRadioButton);
    hboxLayout1->addWidget(m_ui.rgbRadioButton);
    hboxLayout1->addItem(new QSpacerItem(16, 23, QSizePolicy::Expanding, QSizePolicy::Minimum));
    int addRow = 0;
    if (details)
        addRow = 9;
    m_gridLayout->addLayout(hboxLayout1, 3 + addRow, 0, 1, 2);
    m_gridLayout->addWidget(m_ui.hLabel, 4 + addRow, 0, 1, 1);
    m_gridLayout->addWidget(m_ui.frame_2, 4 + addRow, 1, 1, 1);
    m_gridLayout->addWidget(m_ui.sLabel, 5 + addRow, 0, 1, 1);
    m_gridLayout->addWidget(m_ui.frame_5, 5 + addRow, 1, 1, 1);
    m_gridLayout->addWidget(m_ui.vLabel, 6 + addRow, 0, 1, 1);
    m_gridLayout->addWidget(m_ui.frame_3, 6 + addRow, 1, 1, 1);
    m_gridLayout->addWidget(m_ui.aLabel, 7 + addRow, 0, 1, 1);
    m_gridLayout->addWidget(m_ui.frame_4, 7 + addRow, 1, 1, 1);

    if (details) {
        layoutDetails(details);
    }
}

void QtGradientEditorPrivate::layoutDetails(bool details)
{
    QGridLayout *gridLayout = m_gridLayout;
    int col = 2;
    if (!details) {
        col = 0;
        if (!m_hiddenWidget) {
            m_hiddenWidget = new QWidget();
            m_hiddenLayout = new QGridLayout(m_hiddenWidget);
            m_hiddenLayout->setContentsMargins(0, 0, 0, 0);
            m_hiddenLayout->setSizeConstraint(QLayout::SetFixedSize);
        }
        gridLayout = m_hiddenLayout;
    }
    gridLayout->addWidget(m_ui.label1, 1, col + 0, 1, 1);
    gridLayout->addWidget(m_ui.spinBox1, 1, col + 1, 1, 1);
    gridLayout->addWidget(m_ui.label2, 2, col + 0, 1, 1);
    gridLayout->addWidget(m_ui.spinBox2, 2, col + 1, 1, 1);
    gridLayout->addWidget(m_ui.label3, 3, col + 0, 1, 1);
    gridLayout->addWidget(m_ui.spinBox3, 3, col + 1, 1, 1);
    gridLayout->addWidget(m_ui.label4, 4, col + 0, 1, 1);
    gridLayout->addWidget(m_ui.spinBox4, 4, col + 1, 1, 1);
    gridLayout->addWidget(m_ui.label5, 5, col + 0, 1, 1);
    gridLayout->addWidget(m_ui.spinBox5, 5, col + 1, 1, 1);
    gridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 6, col + 0, 1, 1);
    gridLayout->addWidget(m_ui.line1Widget, 7, col + 0, 1, 2);
    gridLayout->addWidget(m_ui.zoomLabel, 8, col + 0, 1, 1);
    gridLayout->addWidget(m_ui.zoomWidget, 8, col + 1, 1, 1);
    gridLayout->addWidget(m_ui.zoomButtonsWidget, 9, col + 0, 1, 1);
    gridLayout->addWidget(m_ui.zoomAllButton, 9, col + 1, 1, 1);
    gridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Preferred), 10, col + 0, 1, 1);
    gridLayout->addWidget(m_ui.line2Widget, 11, col + 0, 1, 2);
    gridLayout->addWidget(m_ui.positionLabel, 12, col + 0, 1, 1);
    gridLayout->addWidget(m_ui.positionWidget, 12, col + 1, 1, 1);
    gridLayout->addWidget(m_ui.hueLabel, 13, col + 0, 1, 1);
    gridLayout->addWidget(m_ui.hueWidget, 13, col + 1, 1, 1);
    gridLayout->addWidget(m_ui.saturationLabel, 14, col + 0, 1, 1);
    gridLayout->addWidget(m_ui.saturationWidget, 14, col + 1, 1, 1);
    gridLayout->addWidget(m_ui.valueLabel, 15, col + 0, 1, 1);
    gridLayout->addWidget(m_ui.valueWidget, 15, col + 1, 1, 1);
    gridLayout->addWidget(m_ui.alphaLabel, 16, col + 0, 1, 1);
    gridLayout->addWidget(m_ui.alphaWidget, 16, col + 1, 1, 1);

    if (details) {
        if (m_hiddenLayout) {
            delete m_hiddenLayout;
            m_hiddenLayout = 0;
        }
        if (m_hiddenWidget) {
            delete m_hiddenWidget;
            m_hiddenWidget = 0;
        }
    }
}

int QtGradientEditorPrivate::extensionWidthHint() const
{
    if (m_details)
        return q_ptr->size().width() - m_ui.gradientStopsWidget->size().width();

    const int space = m_ui.spinBox1->geometry().left() - m_ui.label1->geometry().right();

    return m_hiddenLayout->minimumSize().width() + space;
}

void QtGradientEditorPrivate::slotDetailsChanged(bool details)
{
    showDetails(details);
}

bool QtGradientEditorPrivate::row4Visible() const
{
    if (m_type == QGradient::ConicalGradient)
        return false;
    return true;
}

bool QtGradientEditorPrivate::row5Visible() const
{
    if (m_type == QGradient::RadialGradient)
        return true;
    return false;
}

void QtGradientEditorPrivate::showDetails(bool details)
{
    if (m_details == details)
        return;

    bool blocked = m_ui.detailsButton->signalsBlocked();
    m_ui.detailsButton->blockSignals(true);
    m_ui.detailsButton->setChecked(details);
    m_ui.detailsButton->blockSignals(blocked);

    bool updates = q_ptr->updatesEnabled();
    q_ptr->setUpdatesEnabled(false);

    if (m_gridLayout) {
        m_gridLayout->setEnabled(false);
        delete m_gridLayout;
        m_gridLayout = 0;
    }

    if (!details) {
        layoutDetails(details);
    }

    emit q_ptr->aboutToShowDetails(details, extensionWidthHint());
    m_details = details;

    m_gridLayout = new QGridLayout(q_ptr);
    m_gridLayout->setEnabled(false);
    m_gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    m_gridLayout->setContentsMargins(0, 0, 0, 0);

    m_ui.label4->setVisible(row4Visible());
    m_ui.label5->setVisible(row5Visible());
    m_ui.spinBox4->setVisible(row4Visible());
    m_ui.spinBox5->setVisible(row5Visible());

    setLayout(details);
    m_gridLayout->setEnabled(true);

    q_ptr->setUpdatesEnabled(updates);
    q_ptr->update();
}

void QtGradientEditorPrivate::setSpinBox(QDoubleSpinBox *spinBox, const char *slot, double max, double step, int decimals)
{
    bool blocked = spinBox->signalsBlocked();
    spinBox->blockSignals(true);
    spinBox->setDecimals(decimals);
    spinBox->setMaximum(max);
    spinBox->setSingleStep(step);
    spinBox->blockSignals(blocked);
    QObject::connect(spinBox, SIGNAL(valueChanged(double)), q_ptr, slot);
}

void QtGradientEditorPrivate::reset()
{
    startLinearXSpinBox = 0;
    startLinearYSpinBox = 0;
    endLinearXSpinBox = 0;
    endLinearYSpinBox = 0;
    centralRadialXSpinBox = 0;
    centralRadialYSpinBox = 0;
    focalRadialXSpinBox = 0;
    focalRadialYSpinBox = 0;
    radiusRadialSpinBox = 0;
    centralConicalXSpinBox = 0;
    centralConicalYSpinBox = 0;
    angleConicalSpinBox = 0;
}

void QtGradientEditorPrivate::setType(QGradient::Type type)
{
    if (m_type == type)
        return;

    m_type = type;
    m_ui.spinBox1->disconnect(SIGNAL(valueChanged(double)));
    m_ui.spinBox2->disconnect(SIGNAL(valueChanged(double)));
    m_ui.spinBox3->disconnect(SIGNAL(valueChanged(double)));
    m_ui.spinBox4->disconnect(SIGNAL(valueChanged(double)));
    m_ui.spinBox5->disconnect(SIGNAL(valueChanged(double)));

    reset();

    bool ena = true;

    if (m_gridLayout) {
        ena = m_gridLayout->isEnabled();
        m_gridLayout->setEnabled(false);
    }

    bool spreadEnabled = true;

    if (type == QGradient::LinearGradient) {
        startLinearXSpinBox = m_ui.spinBox1;
        setSpinBox(startLinearXSpinBox, SLOT(slotStartLinearXChanged(double)));
        m_ui.label1->setText(QApplication::translate("QtGradientEditor", "Start X", 0, QApplication::UnicodeUTF8));

        startLinearYSpinBox = m_ui.spinBox2;
        setSpinBox(startLinearYSpinBox, SLOT(slotStartLinearYChanged(double)));
        m_ui.label2->setText(QApplication::translate("QtGradientEditor", "Start Y", 0, QApplication::UnicodeUTF8));

        endLinearXSpinBox = m_ui.spinBox3;
        setSpinBox(endLinearXSpinBox, SLOT(slotEndLinearXChanged(double)));
        m_ui.label3->setText(QApplication::translate("QtGradientEditor", "Final X", 0, QApplication::UnicodeUTF8));

        endLinearYSpinBox = m_ui.spinBox4;
        setSpinBox(endLinearYSpinBox, SLOT(slotEndLinearYChanged(double)));
        m_ui.label4->setText(QApplication::translate("QtGradientEditor", "Final Y", 0, QApplication::UnicodeUTF8));

        setStartLinear(m_ui.gradientWidget->startLinear());
        setEndLinear(m_ui.gradientWidget->endLinear());
    } else if (type == QGradient::RadialGradient) {
        centralRadialXSpinBox = m_ui.spinBox1;
        setSpinBox(centralRadialXSpinBox, SLOT(slotCentralRadialXChanged(double)));
        m_ui.label1->setText(QApplication::translate("QtGradientEditor", "Central X", 0, QApplication::UnicodeUTF8));

        centralRadialYSpinBox = m_ui.spinBox2;
        setSpinBox(centralRadialYSpinBox, SLOT(slotCentralRadialYChanged(double)));
        m_ui.label2->setText(QApplication::translate("QtGradientEditor", "Central Y", 0, QApplication::UnicodeUTF8));

        focalRadialXSpinBox = m_ui.spinBox3;
        setSpinBox(focalRadialXSpinBox, SLOT(slotFocalRadialXChanged(double)));
        m_ui.label3->setText(QApplication::translate("QtGradientEditor", "Focal X", 0, QApplication::UnicodeUTF8));

        focalRadialYSpinBox = m_ui.spinBox4;
        setSpinBox(focalRadialYSpinBox, SLOT(slotFocalRadialYChanged(double)));
        m_ui.label4->setText(QApplication::translate("QtGradientEditor", "Focal Y", 0, QApplication::UnicodeUTF8));

        radiusRadialSpinBox = m_ui.spinBox5;
        setSpinBox(radiusRadialSpinBox, SLOT(slotRadiusRadialChanged(double)), 2.0);
        m_ui.label5->setText(QApplication::translate("QtGradientEditor", "Radius", 0, QApplication::UnicodeUTF8));

        setCentralRadial(m_ui.gradientWidget->centralRadial());
        setFocalRadial(m_ui.gradientWidget->focalRadial());
        setRadiusRadial(m_ui.gradientWidget->radiusRadial());
    } else if (type == QGradient::ConicalGradient) {
        centralConicalXSpinBox = m_ui.spinBox1;
        setSpinBox(centralConicalXSpinBox, SLOT(slotCentralConicalXChanged(double)));
        m_ui.label1->setText(QApplication::translate("QtGradientEditor", "Central X", 0, QApplication::UnicodeUTF8));

        centralConicalYSpinBox = m_ui.spinBox2;
        setSpinBox(centralConicalYSpinBox, SLOT(slotCentralConicalYChanged(double)));
        m_ui.label2->setText(QApplication::translate("QtGradientEditor", "Central Y", 0, QApplication::UnicodeUTF8));

        angleConicalSpinBox = m_ui.spinBox3;
        setSpinBox(angleConicalSpinBox, SLOT(slotAngleConicalChanged(double)), 360.0, 1.0, 1);
        m_ui.label3->setText(QApplication::translate("QtGradientEditor", "Angle", 0, QApplication::UnicodeUTF8));

        setCentralConical(m_ui.gradientWidget->centralConical());
        setAngleConical(m_ui.gradientWidget->angleConical());

        spreadEnabled = false;
    }
    m_ui.spreadComboBox->setEnabled(spreadEnabled);
    m_ui.padButton->setEnabled(spreadEnabled);
    m_ui.repeatButton->setEnabled(spreadEnabled);
    m_ui.reflectButton->setEnabled(spreadEnabled);

    m_ui.label4->setVisible(row4Visible());
    m_ui.spinBox4->setVisible(row4Visible());
    m_ui.label5->setVisible(row5Visible());
    m_ui.spinBox5->setVisible(row5Visible());

    if (m_gridLayout) {
        m_gridLayout->setEnabled(ena);
    }
}

void QtGradientEditorPrivate::slotGradientStopsChanged(const QGradientStops &stops)
{
    m_ui.gradientWidget->setGradientStops(stops);
    updateGradient(true);
}

void QtGradientEditorPrivate::slotTypeChanged(int idx)
{
    QGradient::Type type = QGradient::NoGradient;
    if (idx == 0)
        type = QGradient::LinearGradient;
    else if (idx == 1)
        type = QGradient::RadialGradient;
    else if (idx == 2)
        type = QGradient::ConicalGradient;
    setType(type);
    m_ui.typeComboBox->setCurrentIndex(idx);
    m_typeGroup->button(idx)->setChecked(true);
    m_ui.gradientWidget->setGradientType(type);
    updateGradient(true);
}

void QtGradientEditorPrivate::slotSpreadChanged(int spread)
{
    if (spread == 0) {
        m_ui.gradientWidget->setGradientSpread(QGradient::PadSpread);
    } else if (spread == 1) {
        m_ui.gradientWidget->setGradientSpread(QGradient::RepeatSpread);
    } else if (spread == 2) {
        m_ui.gradientWidget->setGradientSpread(QGradient::ReflectSpread);
    }
    m_ui.spreadComboBox->setCurrentIndex(spread);
    updateGradient(true);
}

void QtGradientEditorPrivate::slotStartLinearXChanged(double value)
{
    QPointF point = m_ui.gradientWidget->startLinear();
    point.setX(value);
    m_ui.gradientWidget->setStartLinear(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::slotStartLinearYChanged(double value)
{
    QPointF point = m_ui.gradientWidget->startLinear();
    point.setY(value);
    m_ui.gradientWidget->setStartLinear(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::slotEndLinearXChanged(double value)
{
    QPointF point = m_ui.gradientWidget->endLinear();
    point.setX(value);
    m_ui.gradientWidget->setEndLinear(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::slotEndLinearYChanged(double value)
{
    QPointF point = m_ui.gradientWidget->endLinear();
    point.setY(value);
    m_ui.gradientWidget->setEndLinear(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::slotCentralRadialXChanged(double value)
{
    QPointF point = m_ui.gradientWidget->centralRadial();
    point.setX(value);
    m_ui.gradientWidget->setCentralRadial(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::slotCentralRadialYChanged(double value)
{
    QPointF point = m_ui.gradientWidget->centralRadial();
    point.setY(value);
    m_ui.gradientWidget->setCentralRadial(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::slotFocalRadialXChanged(double value)
{
    QPointF point = m_ui.gradientWidget->focalRadial();
    point.setX(value);
    m_ui.gradientWidget->setFocalRadial(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::slotFocalRadialYChanged(double value)
{
    QPointF point = m_ui.gradientWidget->focalRadial();
    point.setY(value);
    m_ui.gradientWidget->setFocalRadial(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::slotRadiusRadialChanged(double value)
{
    m_ui.gradientWidget->setRadiusRadial(value);
    updateGradient(true);
}

void QtGradientEditorPrivate::slotCentralConicalXChanged(double value)
{
    QPointF point = m_ui.gradientWidget->centralConical();
    point.setX(value);
    m_ui.gradientWidget->setCentralConical(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::slotCentralConicalYChanged(double value)
{
    QPointF point = m_ui.gradientWidget->centralConical();
    point.setY(value);
    m_ui.gradientWidget->setCentralConical(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::slotAngleConicalChanged(double value)
{
    m_ui.gradientWidget->setAngleConical(value);
    updateGradient(true);
}

void QtGradientEditorPrivate::startLinearChanged(const QPointF &point)
{
    setStartLinear(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::endLinearChanged(const QPointF &point)
{
    setEndLinear(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::centralRadialChanged(const QPointF &point)
{
    setCentralRadial(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::focalRadialChanged(const QPointF &point)
{
    setFocalRadial(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::radiusRadialChanged(qreal radius)
{
    setRadiusRadial(radius);
    updateGradient(true);
}

void QtGradientEditorPrivate::centralConicalChanged(const QPointF &point)
{
    setCentralConical(point);
    updateGradient(true);
}

void QtGradientEditorPrivate::angleConicalChanged(qreal angle)
{
    setAngleConical(angle);
    updateGradient(true);
}

void QtGradientEditorPrivate::setStartLinear(const QPointF &point)
{
    if (startLinearXSpinBox)
        startLinearXSpinBox->setValue(point.x());
    if (startLinearYSpinBox)
        startLinearYSpinBox->setValue(point.y());
}

void QtGradientEditorPrivate::setEndLinear(const QPointF &point)
{
    if (endLinearXSpinBox)
        endLinearXSpinBox->setValue(point.x());
    if (endLinearYSpinBox)
        endLinearYSpinBox->setValue(point.y());
}

void QtGradientEditorPrivate::setCentralRadial(const QPointF &point)
{
    if (centralRadialXSpinBox)
        centralRadialXSpinBox->setValue(point.x());
    if (centralRadialYSpinBox)
        centralRadialYSpinBox->setValue(point.y());
}

void QtGradientEditorPrivate::setFocalRadial(const QPointF &point)
{
    if (focalRadialXSpinBox)
        focalRadialXSpinBox->setValue(point.x());
    if (focalRadialYSpinBox)
        focalRadialYSpinBox->setValue(point.y());
}

void QtGradientEditorPrivate::setRadiusRadial(qreal radius)
{
    if (radiusRadialSpinBox)
        radiusRadialSpinBox->setValue(radius);
}

void QtGradientEditorPrivate::setCentralConical(const QPointF &point)
{
    if (centralConicalXSpinBox)
        centralConicalXSpinBox->setValue(point.x());
    if (centralConicalYSpinBox)
        centralConicalYSpinBox->setValue(point.y());
}

void QtGradientEditorPrivate::setAngleConical(qreal angle)
{
    if (angleConicalSpinBox)
        angleConicalSpinBox->setValue(angle);
}

QtGradientEditor::QtGradientEditor(QWidget *parent)
    : QWidget(parent), d_ptr(new QtGradientEditorPrivate())
{
    d_ptr->q_ptr = this;
    d_ptr->m_type = QGradient::RadialGradient;
    d_ptr->m_ui.setupUi(this);
    d_ptr->m_gridLayout = 0;
    d_ptr->m_hiddenLayout = 0;
    d_ptr->m_hiddenWidget = 0;
    bool detailsDefault = false;
    d_ptr->m_details = !detailsDefault;
    d_ptr->m_detailsButtonVisible = true;
    bool checkeredDefault = true;
    d_ptr->m_backgroundCheckered = !checkeredDefault;
    d_ptr->m_gradientStopsController = new QtGradientStopsController(this);
    d_ptr->m_gradientStopsController->setUi(&d_ptr->m_ui);
    d_ptr->reset();
    d_ptr->setType(QGradient::LinearGradient);
    d_ptr->m_combos = true;
    d_ptr->setCombos(!d_ptr->m_combos);

    d_ptr->showDetails(detailsDefault);
    setBackgroundCheckered(checkeredDefault);

    d_ptr->setStartLinear(QPointF(0, 0));
    d_ptr->setEndLinear(QPointF(1, 1));
    d_ptr->setCentralRadial(QPointF(0.5, 0.5));
    d_ptr->setFocalRadial(QPointF(0.5, 0.5));
    d_ptr->setRadiusRadial(0.5);
    d_ptr->setCentralConical(QPointF(0.5, 0.5));
    d_ptr->setAngleConical(0);

    QIcon icon;
    icon.addPixmap(style()->standardPixmap(QStyle::SP_ArrowRight), QIcon::Normal, QIcon::Off);
    icon.addPixmap(style()->standardPixmap(QStyle::SP_ArrowLeft), QIcon::Normal, QIcon::On);
    d_ptr->m_ui.detailsButton->setIcon(icon);

    connect(d_ptr->m_ui.detailsButton, SIGNAL(clicked(bool)), this, SLOT(slotDetailsChanged(bool)));
    connect(d_ptr->m_gradientStopsController, SIGNAL(gradientStopsChanged(QGradientStops)),
                this, SLOT(slotGradientStopsChanged(QGradientStops)));

    QIcon iconLinear(QLatin1String(":/trolltech/qtgradienteditor/images/typelinear.png"));
    QIcon iconRadial(QLatin1String(":/trolltech/qtgradienteditor/images/typeradial.png"));
    QIcon iconConical(QLatin1String(":/trolltech/qtgradienteditor/images/typeconical.png"));

    d_ptr->m_ui.typeComboBox->addItem(iconLinear, tr("Linear"));
    d_ptr->m_ui.typeComboBox->addItem(iconRadial, tr("Radial"));
    d_ptr->m_ui.typeComboBox->addItem(iconConical, tr("Conical"));

    d_ptr->m_ui.linearButton->setIcon(iconLinear);
    d_ptr->m_ui.radialButton->setIcon(iconRadial);
    d_ptr->m_ui.conicalButton->setIcon(iconConical);

    d_ptr->m_typeGroup = new QButtonGroup(this);
    d_ptr->m_typeGroup->addButton(d_ptr->m_ui.linearButton, 0);
    d_ptr->m_typeGroup->addButton(d_ptr->m_ui.radialButton, 1);
    d_ptr->m_typeGroup->addButton(d_ptr->m_ui.conicalButton, 2);

    connect(d_ptr->m_typeGroup, SIGNAL(buttonClicked(int)),
                this, SLOT(slotTypeChanged(int)));
    connect(d_ptr->m_ui.typeComboBox, SIGNAL(activated(int)),
                this, SLOT(slotTypeChanged(int)));

    QIcon iconPad(QLatin1String(":/trolltech/qtgradienteditor/images/spreadpad.png"));
    QIcon iconRepeat(QLatin1String(":/trolltech/qtgradienteditor/images/spreadrepeat.png"));
    QIcon iconReflect(QLatin1String(":/trolltech/qtgradienteditor/images/spreadreflect.png"));

    d_ptr->m_ui.spreadComboBox->addItem(iconPad, tr("Pad"));
    d_ptr->m_ui.spreadComboBox->addItem(iconRepeat, tr("Repeat"));
    d_ptr->m_ui.spreadComboBox->addItem(iconReflect, tr("Reflect"));

    d_ptr->m_ui.padButton->setIcon(iconPad);
    d_ptr->m_ui.repeatButton->setIcon(iconRepeat);
    d_ptr->m_ui.reflectButton->setIcon(iconReflect);

    d_ptr->m_spreadGroup = new QButtonGroup(this);
    d_ptr->m_spreadGroup->addButton(d_ptr->m_ui.padButton, 0);
    d_ptr->m_spreadGroup->addButton(d_ptr->m_ui.repeatButton, 1);
    d_ptr->m_spreadGroup->addButton(d_ptr->m_ui.reflectButton, 2);
    connect(d_ptr->m_spreadGroup, SIGNAL(buttonClicked(int)),
                this, SLOT(slotSpreadChanged(int)));
    connect(d_ptr->m_ui.spreadComboBox, SIGNAL(activated(int)),
                this, SLOT(slotSpreadChanged(int)));

    connect(d_ptr->m_ui.gradientWidget, SIGNAL(startLinearChanged(QPointF)),
                this, SLOT(startLinearChanged(QPointF)));
    connect(d_ptr->m_ui.gradientWidget, SIGNAL(endLinearChanged(QPointF)),
                this, SLOT(endLinearChanged(QPointF)));
    connect(d_ptr->m_ui.gradientWidget, SIGNAL(centralRadialChanged(QPointF)),
                this, SLOT(centralRadialChanged(QPointF)));
    connect(d_ptr->m_ui.gradientWidget, SIGNAL(focalRadialChanged(QPointF)),
                this, SLOT(focalRadialChanged(QPointF)));
    connect(d_ptr->m_ui.gradientWidget, SIGNAL(radiusRadialChanged(qreal)),
                this, SLOT(radiusRadialChanged(qreal)));
    connect(d_ptr->m_ui.gradientWidget, SIGNAL(centralConicalChanged(QPointF)),
                this, SLOT(centralConicalChanged(QPointF)));
    connect(d_ptr->m_ui.gradientWidget, SIGNAL(angleConicalChanged(qreal)),
                this, SLOT(angleConicalChanged(qreal)));

    QGradientStops stops = gradient().stops();
    d_ptr->m_gradientStopsController->setGradientStops(stops);
    d_ptr->m_ui.gradientWidget->setGradientStops(stops);
}

QtGradientEditor::~QtGradientEditor()
{
    if (d_ptr->m_hiddenWidget)
        delete d_ptr->m_hiddenWidget;
}

void QtGradientEditor::setGradient(const QGradient &grad)
{
    if (grad == gradient())
        return;

    QGradient::Type type = grad.type();
    int idx = 0;
    switch (type) {
        case QGradient::LinearGradient:  idx = 0; break;
        case QGradient::RadialGradient:  idx = 1; break;
        case QGradient::ConicalGradient: idx = 2; break;
        default: return;
    }
    d_ptr->setType(type);
    d_ptr->m_ui.typeComboBox->setCurrentIndex(idx);
    d_ptr->m_ui.gradientWidget->setGradientType(type);
    d_ptr->m_typeGroup->button(idx)->setChecked(true);

    QGradient::Spread spread = grad.spread();
    switch (spread) {
        case QGradient::PadSpread:     idx = 0; break;
        case QGradient::RepeatSpread:  idx = 1; break;
        case QGradient::ReflectSpread: idx = 2; break;
        default: idx = 0; break;
    }
    d_ptr->m_ui.spreadComboBox->setCurrentIndex(idx);
    d_ptr->m_ui.gradientWidget->setGradientSpread(spread);
    d_ptr->m_spreadGroup->button(idx)->setChecked(true);

    if (type == QGradient::LinearGradient) {
        QLinearGradient *gr = (QLinearGradient *)(&grad);
        d_ptr->setStartLinear(gr->start());
        d_ptr->setEndLinear(gr->finalStop());
        d_ptr->m_ui.gradientWidget->setStartLinear(gr->start());
        d_ptr->m_ui.gradientWidget->setEndLinear(gr->finalStop());
    } else if (type == QGradient::RadialGradient) {
        QRadialGradient *gr = (QRadialGradient *)(&grad);
        d_ptr->setCentralRadial(gr->center());
        d_ptr->setFocalRadial(gr->focalPoint());
        d_ptr->setRadiusRadial(gr->radius());
        d_ptr->m_ui.gradientWidget->setCentralRadial(gr->center());
        d_ptr->m_ui.gradientWidget->setFocalRadial(gr->focalPoint());
        d_ptr->m_ui.gradientWidget->setRadiusRadial(gr->radius());
    } else if (type == QGradient::ConicalGradient) {
        QConicalGradient *gr = (QConicalGradient *)(&grad);
        d_ptr->setCentralConical(gr->center());
        d_ptr->setAngleConical(gr->angle());
        d_ptr->m_ui.gradientWidget->setCentralConical(gr->center());
        d_ptr->m_ui.gradientWidget->setAngleConical(gr->angle());
    }

    d_ptr->m_gradientStopsController->setGradientStops(grad.stops());
    d_ptr->m_ui.gradientWidget->setGradientStops(grad.stops());
    d_ptr->updateGradient(false);
}

QGradient QtGradientEditor::gradient() const
{
    return d_ptr->m_gradient;
}

bool QtGradientEditor::isBackgroundCheckered() const
{
    return d_ptr->m_backgroundCheckered;
}

void QtGradientEditor::setBackgroundCheckered(bool checkered)
{
    if (d_ptr->m_backgroundCheckered == checkered)
        return;

    d_ptr->m_backgroundCheckered = checkered;
    d_ptr->m_ui.hueColorLine->setBackgroundCheckered(checkered);
    d_ptr->m_ui.saturationColorLine->setBackgroundCheckered(checkered);
    d_ptr->m_ui.valueColorLine->setBackgroundCheckered(checkered);
    d_ptr->m_ui.alphaColorLine->setBackgroundCheckered(checkered);
    d_ptr->m_ui.gradientWidget->setBackgroundCheckered(checkered);
    d_ptr->m_ui.gradientStopsWidget->setBackgroundCheckered(checkered);
    d_ptr->m_ui.colorButton->setBackgroundCheckered(checkered);
}

bool QtGradientEditor::detailsVisible() const
{
    return d_ptr->m_details;
}

void QtGradientEditor::setDetailsVisible(bool visible)
{
    d_ptr->showDetails(visible);
}

bool QtGradientEditor::isDetailsButtonVisible() const
{
    return d_ptr->m_detailsButtonVisible;
}

void QtGradientEditor::setDetailsButtonVisible(bool visible)
{
    if (d_ptr->m_detailsButtonVisible == visible)
        return;

    d_ptr->m_detailsButtonVisible = visible;
    d_ptr->m_ui.detailsButton->setVisible(visible);
}

QColor::Spec QtGradientEditor::spec() const
{
    return d_ptr->m_gradientStopsController->spec();
}

void QtGradientEditor::setSpec(QColor::Spec spec)
{
    d_ptr->m_gradientStopsController->setSpec(spec);
}

QT_END_NAMESPACE

#include "moc_qtgradienteditor.cpp"

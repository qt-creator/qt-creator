/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "customcolordialog.h"
#include "huecontrol.h"
#include "colorbox.h"

#include <utils/hostosinfo.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGraphicsEffect>

using namespace Utils;

namespace QmlEditorWidgets {

CustomColorDialog::CustomColorDialog(QWidget *parent) : QFrame(parent )
{

    setFrameStyle(QFrame::NoFrame);
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Sunken);

    // TODO: The following code should be enabled for OSX
    // when QTBUG-23205 is fixed
    if (!HostOsInfo::isMacHost()) {
        QGraphicsDropShadowEffect *dropShadowEffect = new QGraphicsDropShadowEffect;
        dropShadowEffect->setBlurRadius(6);
        dropShadowEffect->setOffset(2, 2);
        setGraphicsEffect(dropShadowEffect);
    }
    setAutoFillBackground(true);

    m_hueControl = new HueControl(this);
    m_colorBox = new ColorBox(this);

    QWidget *colorFrameWidget = new QWidget(this);
    QVBoxLayout* vBox = new QVBoxLayout(colorFrameWidget);
    colorFrameWidget->setLayout(vBox);
    vBox->setSpacing(0);
    vBox->setMargin(0);
    vBox->setContentsMargins(0,5,0,28);

    m_beforeColorWidget = new QFrame(colorFrameWidget);
    m_beforeColorWidget->setFixedSize(30, 18);
    m_beforeColorWidget->setAutoFillBackground(true);

    m_currentColorWidget = new QFrame(colorFrameWidget);
    m_currentColorWidget->setFixedSize(30, 18);
    m_currentColorWidget->setAutoFillBackground(true);

    vBox->addWidget(m_beforeColorWidget);
    vBox->addWidget(m_currentColorWidget);


    m_rSpinBox = new QDoubleSpinBox(this);
    m_gSpinBox = new QDoubleSpinBox(this);
    m_bSpinBox = new QDoubleSpinBox(this);
    m_alphaSpinBox = new QDoubleSpinBox(this);

    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(4);
    gridLayout->setVerticalSpacing(4);
    gridLayout->setMargin(4);
    setLayout(gridLayout);

    gridLayout->addWidget(m_colorBox, 0, 0, 4, 1);
    gridLayout->addWidget(m_hueControl, 0, 1, 4, 1);

    gridLayout->addWidget(colorFrameWidget, 0, 2, 2, 1);

    gridLayout->addWidget(new QLabel(QLatin1String("R"), this), 0, 3, 1, 1);
    gridLayout->addWidget(new QLabel(QLatin1String("G"), this), 1, 3, 1, 1);
    gridLayout->addWidget(new QLabel(QLatin1String("B"), this), 2, 3, 1, 1);
    gridLayout->addWidget(new QLabel(QLatin1String("A"), this), 3, 3, 1, 1);

    gridLayout->addWidget(m_rSpinBox, 0, 4, 1, 1);
    gridLayout->addWidget(m_gSpinBox, 1, 4, 1, 1);
    gridLayout->addWidget(m_bSpinBox, 2, 4, 1, 1);
    gridLayout->addWidget(m_alphaSpinBox, 3, 4, 1, 1);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);

    QPushButton *cancelButton = buttonBox->addButton(QDialogButtonBox::Cancel);
    QPushButton *applyButton = buttonBox->addButton(QDialogButtonBox::Apply);

    gridLayout->addWidget(buttonBox, 4, 0, 1, 2);

    resize(sizeHint());

    connect(m_colorBox, SIGNAL(colorChanged()), this, SLOT(onColorBoxChanged()));
    connect(m_alphaSpinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxChanged()));
    connect(m_rSpinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxChanged()));
    connect(m_gSpinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxChanged()));
    connect(m_bSpinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxChanged()));
    connect(m_hueControl, SIGNAL(hueChanged(int)), this, SLOT(onHueChanged(int)));

    connect(applyButton, SIGNAL(pressed()), this, SLOT(onAccept()));
    connect(cancelButton, SIGNAL(pressed()), this, SIGNAL(rejected()));

    m_alphaSpinBox->setMaximum(1);
    m_rSpinBox->setMaximum(1);
    m_gSpinBox->setMaximum(1);
    m_bSpinBox->setMaximum(1);
    m_alphaSpinBox->setSingleStep(0.1);
    m_rSpinBox->setSingleStep(0.1);
    m_gSpinBox->setSingleStep(0.1);
    m_bSpinBox->setSingleStep(0.1);

    m_blockUpdate = false;
}

void CustomColorDialog::setupColor(const QColor &color)
{
    QPalette pal = m_beforeColorWidget->palette();
    pal.setColor(QPalette::Background, color);
    m_beforeColorWidget->setPalette(pal);
    setColor(color);
}

void CustomColorDialog::spinBoxChanged()
{
    if (m_blockUpdate)
        return;
    QColor newColor;
    newColor.setAlphaF(m_alphaSpinBox->value());
    newColor.setRedF(m_rSpinBox->value());
    newColor.setGreenF(m_gSpinBox->value());
    newColor.setBlueF(m_bSpinBox->value());
    setColor(newColor);
}

void CustomColorDialog::onColorBoxChanged()
{
    if (m_blockUpdate)
        return;

    setColor(m_colorBox->color());
}

void CustomColorDialog::setupWidgets()
{
    m_blockUpdate = true;
    m_hueControl->setHue(m_color.hsvHue());
    m_alphaSpinBox->setValue(m_color.alphaF());
    m_rSpinBox->setValue(m_color.redF());
    m_gSpinBox->setValue(m_color.greenF());
    m_bSpinBox->setValue(m_color.blueF());
    m_colorBox->setColor(m_color);
    QPalette pal = m_currentColorWidget->palette();
    pal.setColor(QPalette::Background, m_color);
    m_currentColorWidget->setPalette(pal);
    m_blockUpdate = false;
}

void CustomColorDialog::leaveEvent(QEvent *)
{
    if (HostOsInfo::isMacHost())
        unsetCursor();
}

void CustomColorDialog::enterEvent(QEvent *)
{
    if (HostOsInfo::isMacHost())
        setCursor(Qt::ArrowCursor);
}


} //QmlEditorWidgets

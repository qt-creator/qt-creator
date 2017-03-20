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

#include "backgroundaction.h"

#include <QComboBox>
#include <QPainter>

namespace QmlDesigner {

BackgroundAction::BackgroundAction(QObject *parent) :
    QWidgetAction(parent)
{
}

QIcon iconForColor(const QColor &color) {
    const int size = 16;
    QImage image(size, size, QImage::Format_ARGB32);
    image.fill(0);
    QPainter p(&image);

    p.fillRect(2, 2, size - 4, size - 4, Qt::black);

    if (color.alpha() == 0) {
        const int miniSize = (size - 8) / 2;
        p.fillRect(4, 4, miniSize, miniSize, Qt::white);
        p.fillRect(miniSize + 4, miniSize + 4, miniSize, miniSize, Qt::white);
    } else {
        p.fillRect(4, 4, size - 8, size - 8, color);
    }
    return QPixmap::fromImage(image);
}

QWidget *BackgroundAction::createWidget(QWidget *parent)
{
    QComboBox *comboBox = new QComboBox(parent);
    comboBox->setFixedWidth(42);

    for (int i = 0; i < colors().count(); ++i) {
        comboBox->addItem(tr(""));
        comboBox->setItemIcon(i, iconForColor((colors().at(i))));
    }

    comboBox->setCurrentIndex(0);
    connect(comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &BackgroundAction::emitBackgroundChanged);

    comboBox->setProperty("hideborder", true);
    comboBox->setToolTip(tr("Set the color of the canvas."));
    return comboBox;
}

void BackgroundAction::emitBackgroundChanged(int index)
{
    if (index < colors().count())
        emit backgroundChanged(colors().at(index));
}

QList<QColor> BackgroundAction::colors()
{
    static QColor alphaZero(Qt::transparent);
    static QList<QColor> colorList = QList<QColor>() << alphaZero
                                                  << QColor(Qt::black)
                                                  << QColor(Qt::darkGray)
                                                  << QColor(Qt::lightGray)
                                                  << QColor(Qt::white);


    return colorList;
}

} // namespace QmlDesigner

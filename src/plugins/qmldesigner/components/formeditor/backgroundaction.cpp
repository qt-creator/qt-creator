// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "backgroundaction.h"

#include <theme.h>

#include <utils/stylehelper.h>

#include <QComboBox>
#include <QPainter>

namespace QmlDesigner {

BackgroundAction::BackgroundAction(QObject *parent) :
    QWidgetAction(parent)
{
}

void BackgroundAction::setColor(const QColor &color)
{
    if (m_comboBox)
        m_comboBox->setCurrentIndex(colors().indexOf(color));

}

QIcon iconForColor(const QColor &color) {
    const int size = 16;
    QImage image(size, size, QImage::Format_ARGB32);
    image.fill(0);
    QPainter p(&image);

    if (color == BackgroundAction::ContextImage) {
        const QString unicode = Theme::getIconUnicode(Theme::Icon::textures_medium);
        const QString fontName = "qtds_propertyIconFont.ttf";
        QIcon icon = Utils::StyleHelper::getIconFromIconFont(fontName, unicode, 10, 10, Qt::white);

        return icon;
    }

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
    auto comboBox = new QComboBox(parent);
    comboBox->setFixedWidth(42);

    for (int i = 0; i < colors().size(); ++i) {
        comboBox->addItem(tr(""));
        comboBox->setItemIcon(i, iconForColor((colors().at(i))));
    }

    comboBox->setCurrentIndex(0);
    connect(comboBox, &QComboBox::currentIndexChanged,
            this, &BackgroundAction::emitBackgroundChanged);

    comboBox->setProperty(Utils::StyleHelper::C_HIDE_BORDER, true);
    comboBox->setProperty(Utils::StyleHelper::C_TOOLBAR_ACTIONWIDGET, true);
    comboBox->setToolTip(tr("Set the color of the canvas."));
    m_comboBox = comboBox;
    return comboBox;
}

void BackgroundAction::emitBackgroundChanged(int index)
{
    if (index < colors().size())
        emit backgroundChanged(colors().at(index));
}

QList<QColor> BackgroundAction::colors()
{
    static QColor alphaZero(Qt::transparent);
    static QList<QColor> colorList = {alphaZero,
                                      QColor(BackgroundAction::ContextImage),
                                      QColor(Qt::black),
                                      QColor(0x4c4e50),
                                      QColor(Qt::darkGray),
                                      QColor(Qt::lightGray),
                                      QColor(Qt::white)};

    return colorList;
}

} // namespace QmlDesigner

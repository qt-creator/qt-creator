/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "themehelper.h"
#include "theme/theme.h"
#include "stylehelper.h"

#include <QApplication>
#include <QIcon>
#include <QImage>
#include <QMetaEnum>
#include <QPainter>

namespace Utils {

static QPixmap maskToColorAndAlpha(const QPixmap &mask, const QColor &color)
{
    QImage result(mask.toImage().convertToFormat(QImage::Format_ARGB32));
    result.setDevicePixelRatio(mask.devicePixelRatio());
    QRgb *bitsStart = reinterpret_cast<QRgb*>(result.bits());
    const QRgb *bitsEnd = bitsStart + result.width() * result.height();
    const QRgb tint = color.rgb() & 0x00ffffff;
    const qreal alpha = color.alphaF();
    for (QRgb *pixel = bitsStart; pixel < bitsEnd; ++pixel) {
        QRgb pixelAlpha = ~(*pixel & 0xff) * alpha;
        *pixel = (pixelAlpha << 24) | tint;
    }
    return QPixmap::fromImage(result);
}

typedef QPair<QPixmap, QColor> MaskAndColor;
typedef QList<MaskAndColor> MasksAndColors;
static MasksAndColors masksAndColors(const QString &mask, int dpr)
{
    MasksAndColors result;
    const QStringList list = mask.split(QLatin1Char(','));
    for (QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it) {
        const QStringList items = (*it).split(QLatin1Char('|'));
        const QString fileName = items.first().trimmed();
        QColor color = creatorTheme()->color(Theme::IconsBaseColor);
        if (items.count() > 1) {
            const QString colorName = items.at(1);
            static const QMetaObject &m = Theme::staticMetaObject;
            static const QMetaEnum colorEnum = m.enumerator(m.indexOfEnumerator("Color"));
            bool keyFound = false;
            const int colorEnumKey = colorEnum.keyToValue(colorName.toLatin1().constData(), &keyFound);
            if (keyFound) {
                color = creatorTheme()->color(static_cast<Theme::Color>(colorEnumKey));
            } else {
                const QColor colorFromName(colorName);
                if (colorFromName.isValid())
                    color = colorFromName;
            }
        }
        const QString dprFileName = StyleHelper::availableImageResolutions(fileName).contains(dpr) ?
                    StyleHelper::imageFileWithResolution(fileName, dpr) : fileName;
        result.append(qMakePair(QPixmap(dprFileName), color));
    }
    return result;
}

static void smearPixmap(QPainter *painter, const QPixmap &pixmap)
{
    painter->drawPixmap(QPointF(-0.501, -0.501), pixmap);
    painter->drawPixmap(QPointF(0, -0.501), pixmap);
    painter->drawPixmap(QPointF( 0.5, -0.501), pixmap);
    painter->drawPixmap(QPointF( 0.5, 0), pixmap);
    painter->drawPixmap(QPointF( 0.5, 0.5), pixmap);
    painter->drawPixmap(QPointF( 0, 0.5), pixmap);
    painter->drawPixmap(QPointF(-0.501, 0.5), pixmap);
    painter->drawPixmap(QPointF(-0.501, 0), pixmap);
}

static QPixmap combinedMask(const MasksAndColors &masks)
{
    if (masks.count() == 1)
        return masks.first().first;

    QPixmap result(masks.first().first);
    QPainter p(&result);
    p.setCompositionMode(QPainter::CompositionMode_Darken);
    MasksAndColors::const_iterator maskImage = masks.constBegin();
    maskImage++;
    for (;maskImage != masks.constEnd(); ++maskImage) {
        p.save();
        p.setOpacity(0.4);
        p.setCompositionMode(QPainter::CompositionMode_Lighten);
        smearPixmap(&p, maskToColorAndAlpha((*maskImage).first, Qt::white));
        p.restore();
        p.drawPixmap(0, 0, (*maskImage).first);
    }
    p.end();
    return result;
}

static QPixmap masksToIcon(const MasksAndColors &masks, const QPixmap &combinedMask)
{
    QPixmap result(combinedMask.size());
    result.setDevicePixelRatio(combinedMask.devicePixelRatio());
    result.fill(Qt::transparent);
    QPainter p(&result);

    for (MasksAndColors::const_iterator maskImage = masks.constBegin();
         maskImage != masks.constEnd(); ++maskImage) {
        if (maskImage != masks.constBegin()) {
            // Punch a transparent outline around an overlay.
            p.save();
            p.setOpacity(0.4);
            p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            smearPixmap(&p, maskToColorAndAlpha((*maskImage).first, Qt::white));
            p.restore();
        }
        p.drawPixmap(0, 0, maskToColorAndAlpha((*maskImage).first, (*maskImage).second));
    }

    const QPixmap shadowMask = maskToColorAndAlpha(combinedMask, Qt::black);
    p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    p.setOpacity(0.05);
    p.drawPixmap(QPointF(0, -0.501), shadowMask);
    p.drawPixmap(QPointF(-0.501, 0), shadowMask);
    p.drawPixmap(QPointF(0.5, 0), shadowMask);
    p.drawPixmap(QPointF(0.5, 0.5), shadowMask);
    p.drawPixmap(QPointF(-0.501, 0.5), shadowMask);
    p.setOpacity(0.2);
    p.drawPixmap(0, 1, shadowMask);

    p.end();

    return result;
}

QIcon ThemeHelper::themedIcon(const QString &mask)
{
    QIcon result;
    const MasksAndColors masks = masksAndColors(mask, qRound(qApp->devicePixelRatio()));
    const QPixmap combinedMask = Utils::combinedMask(masks);
    result.addPixmap(masksToIcon(masks, combinedMask));

    QColor disabledColor = creatorTheme()->palette().mid().color();
    disabledColor.setAlphaF(0.6);
    result.addPixmap(maskToColorAndAlpha(combinedMask, disabledColor), QIcon::Disabled);
    return result;
}

QPixmap ThemeHelper::themedIconPixmap(const QString &mask)
{
    const MasksAndColors masks =
            masksAndColors(mask, qRound(qApp->devicePixelRatio()));
    const QPixmap combinedMask = Utils::combinedMask(masks);
    return masksToIcon(masks, combinedMask);
}

QPixmap ThemeHelper::recoloredPixmap(const QString &maskImage, const QColor &color)
{
    return maskToColorAndAlpha(QPixmap(Utils::StyleHelper::dpiSpecificImageFile(maskImage)), color);
}

QColor ThemeHelper::inputfieldIconColor()
{
    // See QLineEdit::paintEvent
    QColor color = QApplication::palette().text().color();
    color.setAlpha(128);
    return color;
}

} // namespace Utils

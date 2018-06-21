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

#include "screenshotcropper.h"

#include <utils/fileutils.h>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDebug>
#include <QFile>

namespace QtSupport {
namespace Internal {

class AreasOfInterest {
public:
    AreasOfInterest();
    QMap<QString, QRect> areas;
};

AreasOfInterest::AreasOfInterest()
{
#ifdef QT_CREATOR
    areas = ScreenshotCropper::loadAreasOfInterest(":/qtsupport/images_areaofinterest.xml");
#endif // QT_CREATOR
}

Q_GLOBAL_STATIC(AreasOfInterest, welcomeScreenAreas)

static inline QString fileNameForPath(const QString &path)
{
    return Utils::FileName::fromString(path).fileName();
}

static QRect cropRectForAreaOfInterest(const QSize &imageSize, const QSize &cropSize, const QRect &areaOfInterest)
{
    QRect result;
    const qreal cropSizeToAreaSizeFactor = qMin(cropSize.width() / qreal(areaOfInterest.width()),
                                                cropSize.height() / qreal(areaOfInterest.height()));
    if (cropSizeToAreaSizeFactor >= 1) {
        const QPoint areaOfInterestCenter = areaOfInterest.center();
        const int cropX = qBound(0,
                                 areaOfInterestCenter.x() - cropSize.width() / 2,
                                 imageSize.width() - cropSize.width());
        const int cropY = qBound(0,
                                 areaOfInterestCenter.y() - cropSize.height() / 2,
                                 imageSize.height() - cropSize.height());
        const int cropWidth = qMin(imageSize.width(), cropSize.width());
        const int cropHeight = qMin(imageSize.height(), cropSize.height());
        result = QRect(cropX, cropY, cropWidth, cropHeight);
    } else {
        QSize resultSize = cropSize.expandedTo(areaOfInterest.size());
        result = QRect(QPoint(), resultSize);
    }
    return result;
}

QImage ScreenshotCropper::croppedImage(const QImage &sourceImage, const QString &filePath, const QSize &cropSize)
{
    const QRect areaOfInterest = welcomeScreenAreas()->areas.value(fileNameForPath(filePath));

    if (areaOfInterest.isValid()) {
        const QRect cropRect = cropRectForAreaOfInterest(sourceImage.size(), cropSize, areaOfInterest);
        const QSize cropRectSize = cropRect.size();
        const QImage result = sourceImage.copy(cropRect);
        if (cropRectSize.width() > cropSize.width() || cropRectSize.height() > cropSize.height())
            return result.scaled(cropSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        else
            return result;
    }

    return sourceImage.scaled(cropSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

static int areaAttribute(const QXmlStreamAttributes &attributes, const QString &name)
{
    bool ok;
    const int result = attributes.value(name).toString().toInt(&ok);
    if (!ok)
        qWarning() << Q_FUNC_INFO << "Could not parse" << name << "for" << attributes.value(QLatin1String("image")).toString();
    return result;
}

static const QString xmlTagAreas = QLatin1String("areas");
static const QString xmlTagArea = QLatin1String("area");
static const QString xmlAttributeImage = QLatin1String("image");
static const QString xmlAttributeX = QLatin1String("x");
static const QString xmlAttributeY = QLatin1String("y");
static const QString xmlAttributeWidth = QLatin1String("width");
static const QString xmlAttributeHeight = QLatin1String("height");

QMap<QString, QRect> ScreenshotCropper::loadAreasOfInterest(const QString &areasXmlFile)
{
    QMap<QString, QRect> areasOfInterest;
    QFile xmlFile(areasXmlFile);
    if (!xmlFile.open(QIODevice::ReadOnly)) {
        qWarning() << Q_FUNC_INFO << "Could not open file" << areasXmlFile;
        return areasOfInterest;
     }
    QXmlStreamReader reader(&xmlFile);
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader.name() == xmlTagArea) {
                const QXmlStreamAttributes attributes = reader.attributes();
                const QString imageName = attributes.value(xmlAttributeImage).toString();
                if (imageName.isEmpty())
                    qWarning() << Q_FUNC_INFO << "Could not parse name";

                const QRect area(areaAttribute(attributes, xmlAttributeX), areaAttribute(attributes, xmlAttributeY),
                                 areaAttribute(attributes, xmlAttributeWidth), areaAttribute(attributes, xmlAttributeHeight));
                areasOfInterest.insert(imageName, area);
            }
            break;
        default: // nothing
            break;
        }
    }

    return areasOfInterest;
}

bool ScreenshotCropper::saveAreasOfInterest(const QString &areasXmlFile, QMap<QString, QRect> &areas)
{
    QFile file(areasXmlFile);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement(xmlTagAreas);
    QMapIterator<QString, QRect> i(areas);
    while (i.hasNext()) {
        i.next();
        writer.writeStartElement(xmlTagArea);
        writer.writeAttribute(xmlAttributeImage, i.key());
        writer.writeAttribute(xmlAttributeX, QString::number(i.value().x()));
        writer.writeAttribute(xmlAttributeY, QString::number(i.value().y()));
        writer.writeAttribute(xmlAttributeWidth, QString::number(i.value().width()));
        writer.writeAttribute(xmlAttributeHeight, QString::number(i.value().height()));
        writer.writeEndElement(); // xmlTagArea
    }
    writer.writeEndElement(); // xmlTagAreas
    writer.writeEndDocument();
    return true;
}

} // namespace Internal
} // namespace QtSupport

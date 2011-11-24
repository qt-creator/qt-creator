#include "screenshotcropper.h"

#include <coreplugin/icore.h>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QPainter>

namespace QtSupport {
namespace Internal {

#ifdef QT_CREATOR
Q_GLOBAL_STATIC_WITH_INITIALIZER(AreasOfInterest, areasOfInterest, {
    *x = ScreenshotCropper::loadAreasOfInterest(Core::ICore::instance()->resourcePath() + QLatin1String("/welcomescreen/images_areaofinterest.xml"));
})
#else
Q_GLOBAL_STATIC(AreasOfInterest, areasOfInterest)
#endif // QT_CREATOR

static inline QString fileNameForPath(const QString &path)
{
    return QFileInfo(path).fileName();
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
    const QRect areaOfInterest = areasOfInterest()->value(fileNameForPath(filePath));

    if (areaOfInterest.isValid()) {
        const QRect cropRect = cropRectForAreaOfInterest(sourceImage.size(), cropSize, areaOfInterest);
        const QSize cropRectSize = cropRect.size();
        const QImage result = sourceImage.copy(cropRect);
        if (cropRectSize.width() > cropSize.width() || cropRectSize.height() > cropSize.height())
            return result.scaled(cropSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        else
            return result;
    }

    return sourceImage.scaled(cropSize, Qt::KeepAspectRatio);
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

AreasOfInterest ScreenshotCropper::loadAreasOfInterest(const QString &areasXmlFile)
{
    AreasOfInterest areasOfInterest;
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

bool ScreenshotCropper::saveAreasOfInterest(const QString &areasXmlFile, AreasOfInterest &areas)
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

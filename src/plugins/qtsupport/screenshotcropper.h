#ifndef SCREENSHOTCROPPER_H
#define SCREENSHOTCROPPER_H

#include <QMap>
#include <QRect>
#include <QImage>

namespace QtSupport {
namespace Internal {

typedef QMap<QString, QRect> AreasOfInterest;

class ScreenshotCropper
{
public:
    static QImage croppedImage(const QImage &sourceImage, const QString &filePath, const QSize &cropSize);
    static AreasOfInterest loadAreasOfInterest(const QString &areasXmlFile);
    static bool saveAreasOfInterest(const QString &areasXmlFile, AreasOfInterest &areas);
};

} // namespace Internal
} // namespace QtSupport

#endif // SCREENSHOTCROPPER_H

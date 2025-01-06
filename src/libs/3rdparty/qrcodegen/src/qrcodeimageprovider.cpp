// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qrcodeimageprovider.h"
#include "qrcodegen.h"

#include <QJsonDocument>
#include <QPainter>
#include <QSvgRenderer>

QrCodeImageProvider::QrCodeImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{}

static QString qrToSvgString(
    const qrcodegen::QrCode &qr,
    int border,
    const QColor &foreground = Qt::black,
    const QColor &background = Qt::white)
{
    if (border < 0)
        throw std::domain_error("Border must be non-negative");
    if (border > INT_MAX / 2 || border * 2 > INT_MAX - qr.getSize())
        throw std::overflow_error("Border too large");

    QString svgString;
    QTextStream stream(&svgString);

    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    stream << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" "
              "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    stream << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" viewBox=\"0 0 ";
    stream << (qr.getSize() + border * 2) << " " << (qr.getSize() + border * 2)
           << "\" stroke=\"none\">\n";
    stream << "\t<rect width=\"100%\" height=\"100%\" fill=\"" << background.name() << "\"/>\n";
    stream << "\t<path d=\"";
    for (int y = 0; y < qr.getSize(); y++) {
        for (int x = 0; x < qr.getSize(); x++) {
            if (qr.getModule(x, y)) {
                if (x != 0 || y != 0)
                    stream << " ";
                stream << "M" << (x + border) << "," << (y + border) << "h1v1h-1z";
            }
        }
    }
    stream << "\" fill=\"" << foreground.name() << "\"/>\n";
    stream << "</svg>\n";

    return svgString;
}

QPixmap QrCodeImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    QString content = id;
    QColor foreground = Qt::black;
    QColor background = Qt::white;

    QJsonParseError parseError;
    QJsonDocument doc
        = QJsonDocument::fromJson(QUrl::fromPercentEncoding(id.toUtf8()).toLatin1(), &parseError);

    if (parseError.error == QJsonParseError::NoError && !doc.isNull()) { // JSON document contained
        content = doc["content"].toString();
        foreground = doc["foreground"].toString();
        background = doc["background"].toString();
    }

    int width = 1000;
    int height = 1000;

    const qrcodegen::QrCode qr
        = qrcodegen::QrCode::encodeText(content.toLatin1(), qrcodegen::QrCode::Ecc::LOW);

    QString svgString = qrToSvgString(qr, 3, foreground, background);
    QSvgRenderer svgRenderer(svgString.toLatin1());

    if (size)
        *size = QSize(width, height);

    QPixmap pixmap(
        requestedSize.width() > 0 ? requestedSize.width() : width,
        requestedSize.height() > 0 ? requestedSize.height() : height);

    QPainter painter(&pixmap);

    svgRenderer.render(&painter);

    return pixmap;
}

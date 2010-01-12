/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
****************************************************************************/

#include "qts60stylethemeio.h"

#if !defined(QT_NO_STYLE_S60)

#include "qs60style.h"
#include "qapplication.h"
#include "qwebview.h"
#include "qwebframe.h"
#include "qeventloop.h"
#include "qpicture.h"
#include "qpicture.h"
#include "qpainter.h"
#include "qfile.h"
#include "qdir.h"
#include "qfileinfo.h"
#include "qxmlstream.h"
#include "qbuffer.h"

#include "qdebug.h"

static const quint32 blobVersion = 1;
static const int pictureSize = 256;

void dumpPartPictures(const QHash<QString, QPicture> &partPictures) {
    foreach (const QString &partKey, partPictures.keys()) {
        QPicture partPicture = partPictures.value(partKey);
        qDebug() << partKey << partPicture.boundingRect();
        QImage image(partPicture.boundingRect().size(), QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter p(&image);
        partPicture.play(&p);
        image.save(partKey + QString::fromLatin1(".png"));
    }
}

void dumpColors(const QHash<QPair<QString, int>, QColor> &colors) {
    foreach (const QColor &color, colors.values()) {
        const QPair<QString, int> key = colors.key(color);
        qDebug() << key << color;
    }
}

bool setS60Theme(QHash<QString, QPicture> &partPictures,
        QHash<QPair<QString, int>, QColor> &colors,
        QS60Style *s60Style)
{
    if (!s60Style)
        s60Style = qobject_cast<QS60Style *>(QApplication::style());
    if (!s60Style || !qobject_cast<QS60Style *>(s60Style)) {
        qWarning() << __FUNCTION__ << ": No QS60Style found.";
        return false;
    }
    s60Style->setS60Theme(partPictures, colors);
    return true;
}

#ifndef QT_NO_WEBKIT
class WebKitSVGRenderer : public QWebView
{
    Q_OBJECT

public:
    WebKitSVGRenderer(QWidget *parent = 0);
    QPicture svgToQPicture(const QString &svgFileName);

private slots:
    void loadFinishedSlot(bool ok);

private:
    QEventLoop m_loop;
    QPicture m_result;
};

WebKitSVGRenderer::WebKitSVGRenderer(QWidget *parent)
    : QWebView(parent)
{
    connect(this, SIGNAL(loadFinished(bool)), SLOT(loadFinishedSlot(bool)));
    setFixedSize(pictureSize, pictureSize);
    QPalette pal = palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    setPalette(pal);
}

QPicture WebKitSVGRenderer::svgToQPicture(const QString &svgFileName)
{
    load(QUrl::fromLocalFile(svgFileName));
    m_loop.exec();
    return m_result;
}

void WebKitSVGRenderer::loadFinishedSlot(bool ok)
{
    // crude error-checking
    if (!ok)
        qDebug() << "Failed loading " << qPrintable(url().toString());

    page()->mainFrame()->evaluateJavaScript(
        "document.rootElement.preserveAspectRatio.baseVal.align = SVGPreserveAspectRatio.SVG_PRESERVEASPECTRATIO_NONE;"
        "document.rootElement.style.width = '100%';"
        "document.rootElement.style.height = '100%';"
        "document.rootElement.width.baseVal.newValueSpecifiedUnits(SVGLength.SVG_LENGTHTYPE_PERCENTAGE, 100);"
        "document.rootElement.height.baseVal.newValueSpecifiedUnits(SVGLength.SVG_LENGTHTYPE_PERCENTAGE, 100);"
    );

    m_result = QPicture(); // "Clear"
    QPainter p(&m_result);
    page()->mainFrame()->render(&p);
    p.end();
    m_result.setBoundingRect(QRect(0, 0, pictureSize, pictureSize));

    m_loop.exit();
}

bool parseTdfFile(const QString &tdfFile,
        QHash<QString, QString> &partSvgs,
        QHash<QPair<QString, int>, QColor> &colors)
{
    const QLatin1String elementKey("element");
    const QLatin1String partKey("part");
    const QLatin1String elementIdKey("id");
    const QLatin1String layerKey("layer");
    const QLatin1String layerFileNameKey("filename");
    const QLatin1String layerColourrgbKey("colourrgb");
    const QString annoyingPrefix("S60_2_6%");
    QFile file(tdfFile);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QXmlStreamReader reader(&file);
    QString partId;
    QPair<QString, int> colorId;
    // Somebody with a sense of aesthetics may implement proper XML parsing, here.
    while (!reader.atEnd()) {
        const QXmlStreamReader::TokenType token = reader.readNext();
        switch (token) {
            case QXmlStreamReader::StartElement:
                if (reader.name() == elementKey || reader.name() == partKey) {
                    QString id = reader.attributes().value(elementIdKey).toString();
                    if (QS60Style::partKeys().contains(id)) {
                        partId = id;
                    } else if (!id.isEmpty() && id.at(id.length()-1).isDigit()) {
                        QString idText = id;
                        idText.remove(QRegExp("[0-9]"));
                        if (QS60Style::colorListKeys().contains(idText)) {
                            QString idNumber = id;
                            idNumber.remove(QRegExp("[a-zA-Z]"));
                            colorId = QPair<QString, int>(idText, idNumber.toInt());
                        }
                    } else if (QS60Style::partKeys().contains(id.mid(annoyingPrefix.length()))) {
                        partId = id.mid(annoyingPrefix.length());
                    }
                } else if (reader.name() == layerKey) {
                    if (!partId.isEmpty()) {
                        const QString svgFile = reader.attributes().value(layerFileNameKey).toString();
                        partSvgs.insert(partId, svgFile);
                        partId.clear();
                    } else if (!colorId.first.isEmpty()) {
                        const QColor colorValue(reader.attributes().value(layerColourrgbKey).toString().toInt(NULL, 16));
                        colors.insert(colorId, colorValue);
                        colorId.first.clear();
                    }
                }
                break;
            case QXmlStreamReader::EndElement:
                if (reader.tokenString() == elementKey || reader.name() == partKey)
                    partId.clear();
                break;
            default:
                break;
        }
    }
    return true;
}

bool loadThemeFromTdf(const QString &tdfFile,
        QHash<QString, QPicture> &partPictures,
        QHash<QPair<QString, int>, QColor> &colors)
{
    QHash<QString, QString> parsedPartSvgs;
    QHash<QString, QPicture> parsedPartPictures;
    QHash<QPair<QString, int>, QColor> parsedColors;
    bool success = parseTdfFile(tdfFile, parsedPartSvgs, parsedColors);
    if (!success)
        return false;
    const QString tdfBasePath = QFileInfo(tdfFile).absolutePath();
    WebKitSVGRenderer renderer;
    foreach(const QString& partKey, parsedPartSvgs.keys()) {
        const QString tdfFullName =
            tdfBasePath + QDir::separator() + parsedPartSvgs.value(partKey);
        if (!QFile(tdfFullName).exists())
            qWarning() << "Could not load part " << tdfFullName;
        const QPicture partPicture = renderer.svgToQPicture(tdfFullName);
        parsedPartPictures.insert(partKey, partPicture);
    }
//    dumpPartPictures(parsedPartPictures);
//    dumpColors(colors);
    partPictures = parsedPartPictures;
    colors = parsedColors;
    return true;
}

bool QtS60StyleThemeIO::loadThemeFromTdf(const QString &themeTdf, QS60Style *s60Style)
{
    QHash<QString, QPicture> partPictures;
    QHash<QPair<QString, int>, QColor> colors;

    if (!::loadThemeFromTdf(themeTdf, partPictures, colors))
        return false;

    return ::setS60Theme(partPictures, colors, s60Style);
}

bool QtS60StyleThemeIO::convertTdfToBlob(const QString &themeTdf, const QString &themeBlob)
{
    QHash<QString, QPicture> partPictures;
    QHash<QPair<QString, int>, QColor> colors;

    if (!::loadThemeFromTdf(themeTdf, partPictures, colors))
        return false;

    QFile blob(themeBlob);
    if (!blob.open(QIODevice::WriteOnly)) {
        qWarning() << __FUNCTION__ << ": Could not create blob: " << themeBlob;
        return false;
    }

    QByteArray data;
    QBuffer dataBuffer(&data);
    dataBuffer.open(QIODevice::WriteOnly);
    QDataStream dataOut(&dataBuffer);

    const int colorsCount = colors.count();
    dataOut << colorsCount;
    const QList<QPair<QString, int> > colorKeys = colors.keys();
    for (int i = 0; i < colorsCount; ++i) {
        const QPair<QString, int> &key = colorKeys.at(i);
        dataOut << key;
        const QColor color = colors.value(key);
        dataOut << color;
    }

    const int picturesCount = partPictures.count();
    dataOut << picturesCount;
    foreach (const QString &key, partPictures.keys()) {
        const QPicture picture = partPictures.value(key);
        dataOut << key;
        dataOut << picture;
    }

    QDataStream blobOut(&blob);
    blobOut << blobVersion;
    blobOut << qCompress(data);
    return blobOut.status() == QDataStream::Ok;
}
#endif // !QT_NO_WEBKIT

bool QtS60StyleThemeIO::loadThemeFromBlob(const QString &themeBlob, QS60Style *s60Style)
{
    QHash<QString, QPicture> partPictures;
    QHash<QPair<QString, int>, QColor> colors;

    QFile blob(themeBlob);
    if (!blob.open(QIODevice::ReadOnly)) {
        qWarning() << __FUNCTION__ << ": Could not read blob: " << themeBlob;
        return false;
    }
    QDataStream blobIn(&blob);

    quint32 version;
    blobIn >> version;

    if (version != blobVersion) {
        qWarning() << __FUNCTION__ << ": Invalid blob version: " << version << " ...expected: " << blobVersion;
        return false;
    }

    QByteArray data;
    blobIn >> data;
    data = qUncompress(data);
    QBuffer dataBuffer(&data);
    dataBuffer.open(QIODevice::ReadOnly);
    QDataStream dataIn(&dataBuffer);

    int colorsCount;
    dataIn >> colorsCount;
    for (int i = 0; i < colorsCount; ++i) {
        QPair<QString, int> key;
        dataIn >> key;
        QColor value;
        dataIn >> value;
        colors.insert(key, value);
    }

    int picturesCount;
    dataIn >> picturesCount;
    for (int i = 0; i < picturesCount; ++i) {
        QString key;
        dataIn >> key;
        QPicture value;
        dataIn >> value;
        value.setBoundingRect(QRect(0, 0, pictureSize, pictureSize)); // Bug? The forced bounding rect was not deserialized.
        partPictures.insert(key, value);
    }

    if (dataIn.status() != QDataStream::Ok) {
        qWarning() << __FUNCTION__ << ": Invalid data blob: " << themeBlob;
        return false;
    }

//    dumpPartPictures(partPictures);
//    dumpColors(colors);

    return ::setS60Theme(partPictures, colors, s60Style);
}

#include "qts60stylethemeio.moc"

#endif // QT_NO_STYLE_S60

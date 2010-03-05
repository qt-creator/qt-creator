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

#include "qtgradientutils.h"
#include "qtgradientmanager.h"
#include <QtGui/QLinearGradient>
#include <QtGui/QRadialGradient>
#include <QtGui/QConicalGradient>
#include <QtXml/QDomDocument>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

static QString gradientTypeToString(QGradient::Type type)
{
    if (type == QGradient::LinearGradient)
        return QLatin1String("LinearGradient");
    if (type == QGradient::RadialGradient)
        return QLatin1String("RadialGradient");
    if (type == QGradient::ConicalGradient)
        return QLatin1String("ConicalGradient");
    return QLatin1String("NoGradient");
}

static QGradient::Type stringToGradientType(const QString &name)
{
    if (name == QLatin1String("LinearGradient"))
        return QGradient::LinearGradient;
    if (name == QLatin1String("RadialGradient"))
        return QGradient::RadialGradient;
    if (name == QLatin1String("ConicalGradient"))
        return QGradient::ConicalGradient;
    return QGradient::NoGradient;
}

static QString gradientSpreadToString(QGradient::Spread spread)
{
    if (spread == QGradient::PadSpread)
        return QLatin1String("PadSpread");
    if (spread == QGradient::RepeatSpread)
        return QLatin1String("RepeatSpread");
    if (spread == QGradient::ReflectSpread)
        return QLatin1String("ReflectSpread");
    return QLatin1String("PadSpread");
}

static QGradient::Spread stringToGradientSpread(const QString &name)
{
    if (name == QLatin1String("PadSpread"))
        return QGradient::PadSpread;
    if (name == QLatin1String("RepeatSpread"))
        return QGradient::RepeatSpread;
    if (name == QLatin1String("ReflectSpread"))
        return QGradient::ReflectSpread;
    return QGradient::PadSpread;
}

static QString gradientCoordinateModeToString(QGradient::CoordinateMode mode)
{
    if (mode == QGradient::LogicalMode)
        return QLatin1String("LogicalMode");
    if (mode == QGradient::StretchToDeviceMode)
        return QLatin1String("StretchToDeviceMode");
    if (mode == QGradient::ObjectBoundingMode)
        return QLatin1String("ObjectBoundingMode");
    return QLatin1String("StretchToDeviceMode");
}

static QGradient::CoordinateMode stringToGradientCoordinateMode(const QString &name)
{
    if (name == QLatin1String("LogicalMode"))
        return QGradient::LogicalMode;
    if (name == QLatin1String("StretchToDeviceMode"))
        return QGradient::StretchToDeviceMode;
    if (name == QLatin1String("ObjectBoundingMode"))
        return QGradient::ObjectBoundingMode;
    return QGradient::StretchToDeviceMode;
}

static QDomElement saveColor(QDomDocument &doc, const QColor &color)
{
    QDomElement colorElem = doc.createElement(QLatin1String("colorData"));

    colorElem.setAttribute(QLatin1String("r"), QString::number(color.red()));
    colorElem.setAttribute(QLatin1String("g"), QString::number(color.green()));
    colorElem.setAttribute(QLatin1String("b"), QString::number(color.blue()));
    colorElem.setAttribute(QLatin1String("a"), QString::number(color.alpha()));

    return colorElem;
}

static QDomElement saveGradientStop(QDomDocument &doc, const QGradientStop &stop)
{
    QDomElement stopElem = doc.createElement(QLatin1String("stopData"));

    stopElem.setAttribute(QLatin1String("position"), QString::number(stop.first));

    const QDomElement colorElem = saveColor(doc, stop.second);
    stopElem.appendChild(colorElem);

    return stopElem;
}

static QDomElement saveGradient(QDomDocument &doc, const QGradient &gradient)
{
    QDomElement gradElem = doc.createElement(QLatin1String("gradientData"));

    const QGradient::Type type = gradient.type();
    gradElem.setAttribute(QLatin1String("type"), gradientTypeToString(type));
    gradElem.setAttribute(QLatin1String("spread"), gradientSpreadToString(gradient.spread()));
    gradElem.setAttribute(QLatin1String("coordinateMode"), gradientCoordinateModeToString(gradient.coordinateMode()));

    QGradientStops stops = gradient.stops();
    QVectorIterator<QGradientStop > it(stops);
    while (it.hasNext())
        gradElem.appendChild(saveGradientStop(doc, it.next()));

    if (type == QGradient::LinearGradient) {
        const QLinearGradient &g = *static_cast<const QLinearGradient *>(&gradient);
        gradElem.setAttribute(QLatin1String("startX"), QString::number(g.start().x()));
        gradElem.setAttribute(QLatin1String("startY"), QString::number(g.start().y()));
        gradElem.setAttribute(QLatin1String("endX"), QString::number(g.finalStop().x()));
        gradElem.setAttribute(QLatin1String("endY"), QString::number(g.finalStop().y()));
    } else if (type == QGradient::RadialGradient) {
        const QRadialGradient &g = *static_cast<const QRadialGradient *>(&gradient);
        gradElem.setAttribute(QLatin1String("centerX"), QString::number(g.center().x()));
        gradElem.setAttribute(QLatin1String("centerY"), QString::number(g.center().y()));
        gradElem.setAttribute(QLatin1String("focalX"), QString::number(g.focalPoint().x()));
        gradElem.setAttribute(QLatin1String("focalY"), QString::number(g.focalPoint().y()));
        gradElem.setAttribute(QLatin1String("radius"), QString::number(g.radius()));
    } else if (type == QGradient::ConicalGradient) {
        const QConicalGradient &g = *static_cast<const QConicalGradient*>(&gradient);
        gradElem.setAttribute(QLatin1String("centerX"), QString::number(g.center().x()));
        gradElem.setAttribute(QLatin1String("centerY"), QString::number(g.center().y()));
        gradElem.setAttribute(QLatin1String("angle"), QString::number(g.angle()));
    }

    return gradElem;
}

static QColor loadColor(const QDomElement &elem)
{
    if (elem.tagName() != QLatin1String("colorData"))
        return QColor();

    return QColor(elem.attribute(QLatin1String("r")).toInt(),
            elem.attribute(QLatin1String("g")).toInt(),
            elem.attribute(QLatin1String("b")).toInt(),
            elem.attribute(QLatin1String("a")).toInt());
}

static QGradientStop loadGradientStop(const QDomElement &elem)
{
    if (elem.tagName() != QLatin1String("stopData"))
        return QGradientStop();

    const qreal pos = static_cast<qreal>(elem.attribute(QLatin1String("position")).toDouble());
    return qMakePair(pos, loadColor(elem.firstChild().toElement()));
}

static QGradient loadGradient(const QDomElement &elem)
{
    if (elem.tagName() != QLatin1String("gradientData"))
        return QLinearGradient();

    const QGradient::Type type = stringToGradientType(elem.attribute(QLatin1String("type")));
    const QGradient::Spread spread = stringToGradientSpread(elem.attribute(QLatin1String("spread")));
    const QGradient::CoordinateMode mode = stringToGradientCoordinateMode(elem.attribute(QLatin1String("coordinateMode")));

    QGradient gradient = QLinearGradient();

    if (type == QGradient::LinearGradient) {
        QLinearGradient g;
        g.setStart(elem.attribute(QLatin1String("startX")).toDouble(), elem.attribute(QLatin1String("startY")).toDouble());
        g.setFinalStop(elem.attribute(QLatin1String("endX")).toDouble(), elem.attribute(QLatin1String("endY")).toDouble());
        gradient = g;
    } else if (type == QGradient::RadialGradient) {
        QRadialGradient g;
        g.setCenter(elem.attribute(QLatin1String("centerX")).toDouble(), elem.attribute(QLatin1String("centerY")).toDouble());
        g.setFocalPoint(elem.attribute(QLatin1String("focalX")).toDouble(), elem.attribute(QLatin1String("focalY")).toDouble());
        g.setRadius(elem.attribute(QLatin1String("radius")).toDouble());
        gradient = g;
    } else if (type == QGradient::ConicalGradient) {
        QConicalGradient g;
        g.setCenter(elem.attribute(QLatin1String("centerX")).toDouble(), elem.attribute(QLatin1String("centerY")).toDouble());
        g.setAngle(elem.attribute(QLatin1String("angle")).toDouble());
        gradient = g;
    }

    QDomElement stopElem = elem.firstChildElement();
    while (!stopElem.isNull()) {
        QGradientStop stop = loadGradientStop(stopElem);

        gradient.setColorAt(stop.first, stop.second);

        stopElem = stopElem.nextSiblingElement();
    }

    gradient.setSpread(spread);
    gradient.setCoordinateMode(mode);

    return gradient;
}

QString QtGradientUtils::saveState(const QtGradientManager *manager)
{
    QDomDocument doc;

    QDomElement rootElem = doc.createElement(QLatin1String("gradients"));

    QMap<QString, QGradient> grads = manager->gradients();
    QMapIterator<QString, QGradient> itGrad(grads);
    while (itGrad.hasNext()) {
        itGrad.next();
        QDomElement idElem = doc.createElement(QLatin1String("gradient"));
        idElem.setAttribute(QLatin1String("name"), itGrad.key());
        QDomElement gradElem = saveGradient(doc, itGrad.value());
        idElem.appendChild(gradElem);

        rootElem.appendChild(idElem);
    }

    doc.appendChild(rootElem);

    return doc.toString();
}

void QtGradientUtils::restoreState(QtGradientManager *manager, const QString &state)
{
    manager->clear();

    QDomDocument doc;
    doc.setContent(state);

    QDomElement rootElem = doc.documentElement();

    QDomElement gradElem = rootElem.firstChildElement();
    while (!gradElem.isNull()) {
        const QString name = gradElem.attribute(QLatin1String("name"));
        const QGradient gradient = loadGradient(gradElem.firstChildElement());

        manager->addGradient(name, gradient);
        gradElem = gradElem.nextSiblingElement();
    }
}

QPixmap QtGradientUtils::gradientPixmap(const QGradient &gradient, const QSize &size, bool checkeredBackground)
{
    QImage image(size, QImage::Format_ARGB32);
    QPainter p(&image);
    p.setCompositionMode(QPainter::CompositionMode_Source);

    if (checkeredBackground) {
        int pixSize = 20;
        QPixmap pm(2 * pixSize, 2 * pixSize);

        QPainter pmp(&pm);
        pmp.fillRect(0, 0, pixSize, pixSize, Qt::lightGray);
        pmp.fillRect(pixSize, pixSize, pixSize, pixSize, Qt::lightGray);
        pmp.fillRect(0, pixSize, pixSize, pixSize, Qt::darkGray);
        pmp.fillRect(pixSize, 0, pixSize, pixSize, Qt::darkGray);

        p.setBrushOrigin((size.width() % pixSize + pixSize) / 2, (size.height() % pixSize + pixSize) / 2);
        p.fillRect(0, 0, size.width(), size.height(), pm);
        p.setBrushOrigin(0, 0);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    const qreal scaleFactor = 0.999999;
    p.scale(scaleFactor, scaleFactor);
    QGradient grad = gradient;
    grad.setCoordinateMode(QGradient::StretchToDeviceMode);
    p.fillRect(QRect(0, 0, size.width(), size.height()), grad);
    p.drawRect(QRect(0, 0, size.width() - 1, size.height() - 1));

    return QPixmap::fromImage(image);
}

static QString styleSheetFillName(const QGradient &gradient)
{
    QString result;

    switch (gradient.type()) {
        case QGradient::LinearGradient:
            result += QLatin1String("qlineargradient");
            break;
        case QGradient::RadialGradient:
            result += QLatin1String("qradialgradient");
            break;
        case QGradient::ConicalGradient:
            result += QLatin1String("qconicalgradient");
            break;
        default:
            qWarning() << "QtGradientUtils::styleSheetFillName(): gradient type" << gradient.type() << "not supported!";
            break;
    }

    return result;
}

static QStringList styleSheetParameters(const QGradient &gradient)
{
    QStringList result;

    if (gradient.type() != QGradient::ConicalGradient) {
        QString spread;
        switch (gradient.spread()) {
            case QGradient::PadSpread:
                spread = QLatin1String("pad");
                break;
            case QGradient::ReflectSpread:
                spread = QLatin1String("reflect");
                break;
            case QGradient::RepeatSpread:
                spread = QLatin1String("repeat");
                break;
            default:
                qWarning() << "QtGradientUtils::styleSheetParameters(): gradient spread" << gradient.spread() << "not supported!";
                break;
        }
        result << QLatin1String("spread:") + spread;
    }

    switch (gradient.type()) {
        case QGradient::LinearGradient: {
            const QLinearGradient *linearGradient = static_cast<const QLinearGradient*>(&gradient);
            result << QLatin1String("x1:") + QString::number(linearGradient->start().x())
                << QLatin1String("y1:")    + QString::number(linearGradient->start().y())
                << QLatin1String("x2:")    + QString::number(linearGradient->finalStop().x())
                << QLatin1String("y2:")    + QString::number(linearGradient->finalStop().y());
            break;
        }
        case QGradient::RadialGradient: {
            const QRadialGradient *radialGradient = static_cast<const QRadialGradient*>(&gradient);
            result << QLatin1String("cx:")  + QString::number(radialGradient->center().x())
                << QLatin1String("cy:")     + QString::number(radialGradient->center().y())
                << QLatin1String("radius:") + QString::number(radialGradient->radius())
                << QLatin1String("fx:")     + QString::number(radialGradient->focalPoint().x())
                << QLatin1String("fy:")     + QString::number(radialGradient->focalPoint().y());
            break;
        }
        case QGradient::ConicalGradient: {
            const QConicalGradient *conicalGradient = static_cast<const QConicalGradient*>(&gradient);
            result << QLatin1String("cx:") + QString::number(conicalGradient->center().x())
                << QLatin1String("cy:")    + QString::number(conicalGradient->center().y())
                << QLatin1String("angle:") + QString::number(conicalGradient->angle());
            break;
        }
        default:
            qWarning() << "QtGradientUtils::styleSheetParameters(): gradient type" << gradient.type() << "not supported!";
            break;
    }

    return result;
}

static QStringList styleSheetStops(const QGradient &gradient)
{
    QStringList result;
    foreach (QGradientStop stop, gradient.stops()) {
        const QColor color = stop.second;

        const QString stopDescription = QLatin1String("stop:") + QString::number(stop.first) + QLatin1String(" rgba(")
                + QString::number(color.red()) + QLatin1String(", ")
                + QString::number(color.green()) + QLatin1String(", ")
                + QString::number(color.blue()) + QLatin1String(", ")
                + QString::number(color.alpha()) + QLatin1Char(')');
        result << stopDescription;
    }

    return result;
}

QString QtGradientUtils::styleSheetCode(const QGradient &gradient)
{
    QStringList gradientParameters;
    gradientParameters << styleSheetParameters(gradient) << styleSheetStops(gradient);

    return styleSheetFillName(gradient) + QLatin1Char('(') + gradientParameters.join(QLatin1String(", ")) + QLatin1Char(')');
}

QT_END_NAMESPACE

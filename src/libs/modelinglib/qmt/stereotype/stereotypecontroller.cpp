/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "stereotypecontroller.h"

#include "customrelation.h"
#include "stereotypeicon.h"
#include "shapepaintvisitor.h"
#include "toolbar.h"

#include "qmt/infrastructure/qmtassert.h"
#include "qmt/style/style.h"
#include <utils/algorithm.h>

#include <QHash>
#include <QPainter>
#include <QIcon>
#include <QPair>

#include <algorithm>

namespace qmt {

namespace {

struct IconKey {
    IconKey(StereotypeIcon::Element element, const QList<QString> &stereotypes, const QString &defaultIconPath,
            const Uid &styleUid, const QSize &size, const QMarginsF &margins, qreal lineWidth)
        : m_element(element),
          m_stereotypes(stereotypes),
          m_defaultIconPath(defaultIconPath),
          m_styleUid(styleUid),
          m_size(size),
          m_margins(margins),
          m_lineWidth(lineWidth)
    {
    }

    const StereotypeIcon::Element m_element;
    const QList<QString> m_stereotypes;
    const QString m_defaultIconPath;
    const Uid m_styleUid;
    const QSize m_size;
    const QMarginsF m_margins;
    const qreal m_lineWidth;
};

bool operator==(const IconKey &lhs, const IconKey &rhs) {
    return lhs.m_element == rhs.m_element
            && lhs.m_stereotypes == rhs.m_stereotypes
            && lhs.m_defaultIconPath == rhs.m_defaultIconPath
            && lhs.m_styleUid == rhs.m_styleUid
            && lhs.m_size == rhs.m_size
            && lhs.m_margins == rhs.m_margins
            && lhs.m_lineWidth == rhs.m_lineWidth;
}

uint qHash(const IconKey &key) {
    return ::qHash(key.m_element) + qHash(key.m_stereotypes) + qHash(key.m_defaultIconPath)
            + qHash(key.m_styleUid) + ::qHash(key.m_size.width()) + ::qHash(key.m_size.height());
}

}

class StereotypeController::StereotypeControllerPrivate
{
public:
    QHash<QPair<StereotypeIcon::Element, QString>, QString> m_stereotypeToIconIdMap;
    QHash<QString, StereotypeIcon> m_iconIdToStereotypeIconsMap;
    QHash<QString, CustomRelation> m_relationIdToCustomRelationMap;
    QList<Toolbar> m_toolbars;
    QList<Toolbar> m_elementToolbars;
    QHash<IconKey, QIcon> m_iconMap;
};

StereotypeController::StereotypeController(QObject *parent) :
    QObject(parent),
    d(new StereotypeControllerPrivate)
{
}

StereotypeController::~StereotypeController()
{
    delete d;
}

QList<StereotypeIcon> StereotypeController::stereotypeIcons() const
{
    return d->m_iconIdToStereotypeIconsMap.values();
}

QList<Toolbar> StereotypeController::toolbars() const
{
    return d->m_toolbars;
}

QList<Toolbar> StereotypeController::findToolbars(const QString &elementType) const
{
    return Utils::filtered(d->m_elementToolbars, [&elementType](const Toolbar &tb) {
        return tb.elementTypes().contains(elementType);
    });
}

QList<QString> StereotypeController::knownStereotypes(StereotypeIcon::Element stereotypeElement) const
{
    QSet<QString> stereotypes;
    foreach (const StereotypeIcon &icon, d->m_iconIdToStereotypeIconsMap) {
        if (icon.elements().isEmpty() || icon.elements().contains(stereotypeElement))
            stereotypes += icon.stereotypes();
    }
    QList<QString> list = stereotypes.toList();
    std::sort(list.begin(), list.end());
    return list;
}

QString StereotypeController::findStereotypeIconId(StereotypeIcon::Element element,
                                                   const QList<QString> &stereotypes) const
{
    foreach (const QString &stereotype, stereotypes) {
        if (d->m_stereotypeToIconIdMap.contains(qMakePair(element, stereotype)))
            return d->m_stereotypeToIconIdMap.value(qMakePair(element, stereotype));
        else if (d->m_stereotypeToIconIdMap.contains(qMakePair(StereotypeIcon::ElementAny, stereotype)))
            return d->m_stereotypeToIconIdMap.value(qMakePair(StereotypeIcon::ElementAny, stereotype));
    }
    return QString();
}

QList<QString> StereotypeController::filterStereotypesByIconId(const QString &stereotypeIconId,
                                                               const QList<QString> &stereotypes) const
{
    if (!d->m_iconIdToStereotypeIconsMap.contains(stereotypeIconId))
        return stereotypes;
    QList<QString> filteredStereotypes = stereotypes;
    foreach (const QString &stereotype, d->m_iconIdToStereotypeIconsMap.value(stereotypeIconId).stereotypes())
        filteredStereotypes.removeAll(stereotype);
    return filteredStereotypes;
}

StereotypeIcon StereotypeController::findStereotypeIcon(const QString &stereotypeIconId) const
{
    QMT_CHECK(d->m_iconIdToStereotypeIconsMap.contains(stereotypeIconId));
    return d->m_iconIdToStereotypeIconsMap.value(stereotypeIconId);
}

CustomRelation StereotypeController::findCustomRelation(const QString &customRelationId) const
{
    return d->m_relationIdToCustomRelationMap.value(customRelationId);
}

QIcon StereotypeController::createIcon(StereotypeIcon::Element element, const QList<QString> &stereotypes,
                                       const QString &defaultIconPath, const Style *style, const QSize &size,
                                       const QMarginsF &margins, qreal lineWidth)
{
    IconKey key(element, stereotypes, defaultIconPath, style->uid(), size, margins, lineWidth);
    QIcon icon = d->m_iconMap.value(key);
    if (!icon.isNull())
        return icon;
    QString stereotypeIconId = findStereotypeIconId(element, stereotypes);
    if (!stereotypeIconId.isEmpty()) {
        StereotypeIcon stereotypeIcon = findStereotypeIcon(stereotypeIconId);

        // calculate bounding rectangle relativ to original icon size
        ShapeSizeVisitor sizeVisitor(QPointF(0.0, 0.0),
                                     QSizeF(stereotypeIcon.width(), stereotypeIcon.height()),
                                     QSizeF(stereotypeIcon.width(), stereotypeIcon.height()),
                                     QSizeF(stereotypeIcon.width(), stereotypeIcon.height()));
        stereotypeIcon.iconShape().visitShapes(&sizeVisitor);
        QRectF iconBoundingRect = sizeVisitor.boundingRect();

        // calc painting space within margins
        qreal innerWidth = size.width() - margins.left() - margins.right();
        qreal innerHeight = size.height() - margins.top() - margins.bottom();

        // calculate width/height ratio from icon size
        qreal widthRatio = 1.0;
        qreal heightRatio = 1.0;
        qreal ratio = stereotypeIcon.width() / stereotypeIcon.height();
        if (ratio > 1.0)
            heightRatio /= ratio;
        else
            widthRatio *= ratio;

        // calculate inner painting area
        qreal paintWidth = stereotypeIcon.width() * innerWidth / iconBoundingRect.width() * widthRatio;
        qreal paintHeight = stereotypeIcon.height() * innerHeight / iconBoundingRect.height() * heightRatio;
        // icons which renders smaller than their size should not be zoomed
        if (paintWidth > innerWidth) {
            paintHeight *= innerWidth / paintHeight;
            paintWidth = innerWidth;
        }
        if (paintHeight > innerHeight) {
            paintWidth *= innerHeight / paintHeight;
            paintHeight = innerHeight;
        }

        // calculate offset of top/left edge
        qreal paintLeft = iconBoundingRect.left() * paintWidth / stereotypeIcon.width();
        qreal paintTop = iconBoundingRect.top() * paintHeight / stereotypeIcon.height();

        // calculate total painting size
        qreal totalPaintWidth = iconBoundingRect.width() * paintWidth / stereotypeIcon.width();
        qreal totalPaintHeight = iconBoundingRect.height() * paintHeight / stereotypeIcon.height();

        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        painter.setBrush(Qt::NoBrush);
        // set painting origin taking margin, offset and centering into account
        painter.translate(QPointF(margins.left(), margins.top()) - QPointF(paintLeft, paintTop)
                          + QPointF((innerWidth - totalPaintWidth) / 2, (innerHeight - totalPaintHeight) / 2));
        QPen linePen = style->linePen();
        linePen.setWidthF(lineWidth);
        painter.setPen(linePen);
        painter.setBrush(style->fillBrush());

        ShapePaintVisitor visitor(&painter, QPointF(0.0, 0.0),
                                  QSizeF(stereotypeIcon.width(), stereotypeIcon.height()),
                                  QSizeF(paintWidth, paintHeight), QSizeF(paintWidth, paintHeight));
        stereotypeIcon.iconShape().visitShapes(&visitor);

        icon = QIcon(pixmap);
    }
    if (icon.isNull() && !defaultIconPath.isEmpty())
        icon = QIcon(defaultIconPath);
    d->m_iconMap.insert(key, icon);
    return icon;

}

void StereotypeController::addStereotypeIcon(const StereotypeIcon &stereotypeIcon)
{
    if (stereotypeIcon.elements().isEmpty()) {
        foreach (const QString &stereotype, stereotypeIcon.stereotypes())
            d->m_stereotypeToIconIdMap.insert(qMakePair(StereotypeIcon::ElementAny, stereotype), stereotypeIcon.id());
    } else {
        foreach (StereotypeIcon::Element element, stereotypeIcon.elements()) {
            foreach (const QString &stereotype, stereotypeIcon.stereotypes())
                d->m_stereotypeToIconIdMap.insert(qMakePair(element, stereotype), stereotypeIcon.id());
        }
    }
    d->m_iconIdToStereotypeIconsMap.insert(stereotypeIcon.id(), stereotypeIcon);
}

void StereotypeController::addCustomRelation(const CustomRelation &customRelation)
{
    d->m_relationIdToCustomRelationMap.insert(customRelation.id(), customRelation);
}

void StereotypeController::addToolbar(const Toolbar &toolbar)
{
    if (toolbar.elementTypes().isEmpty())
        d->m_toolbars.append(toolbar);
    else
        d->m_elementToolbars.append(toolbar);
}

} // namespace qmt

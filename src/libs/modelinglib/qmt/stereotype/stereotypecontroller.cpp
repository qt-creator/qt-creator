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

#include "stereotypeicon.h"
#include "shapepaintvisitor.h"
#include "toolbar.h"

#include "qmt/infrastructure/qmtassert.h"
#include "qmt/style/style.h"

#include <QHash>
#include <QPainter>
#include <QIcon>
#include <QPair>

#include <algorithm>

namespace qmt {

class StereotypeController::StereotypeControllerPrivate
{
public:
    QHash<QPair<StereotypeIcon::Element, QString>, QString> m_stereotypeToIconIdMap;
    QHash<QString, StereotypeIcon> m_iconIdToStereotypeIconsMap;
    QList<Toolbar> m_toolbars;
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

QList<QString> StereotypeController::knownStereotypes(StereotypeIcon::Element stereotypeElement) const
{
    QSet<QString> stereotypes;
    foreach (const StereotypeIcon &icon, d->m_iconIdToStereotypeIconsMap.values()) {
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

StereotypeIcon StereotypeController::findStereotypeIcon(const QString &stereotypeIconId)
{
    QMT_CHECK(d->m_iconIdToStereotypeIconsMap.contains(stereotypeIconId));
    return d->m_iconIdToStereotypeIconsMap.value(stereotypeIconId);
}

QIcon StereotypeController::createIcon(StereotypeIcon::Element element, const QList<QString> &stereotypes,
                                       const QString &defaultIconPath, const Style *style, const QSize &size,
                                       const QMarginsF &margins)
{
    // TODO implement cache with key build from element, stereotypes, defaultIconPath, style, size and margins
    // TODO implement unique id for style which can be used as key
    // TODO fix rendering of icon which negativ extension of bounding box (e.g. stereotype "component")
    QIcon icon;
    QString stereotypeIconId = findStereotypeIconId(element, stereotypes);
    if (!stereotypeIconId.isEmpty()) {
        StereotypeIcon stereotypeIcon = findStereotypeIcon(stereotypeIconId);

        qreal width = size.width() - margins.left() - margins.right();
        qreal height = size.height() - margins.top() - margins.bottom();
        qreal ratioWidth = height * stereotypeIcon.width() / stereotypeIcon.height();
        qreal ratioHeight = width * stereotypeIcon.height() / stereotypeIcon.width();
        if (ratioWidth > width)
            height = ratioHeight;
        else if (ratioHeight > height)
            width = ratioWidth;
        QSizeF shapeSize(width, height);

        ShapeSizeVisitor sizeVisitor(QPointF(0.0, 0.0),
                                           QSizeF(stereotypeIcon.width(), stereotypeIcon.height()),
                                           shapeSize, shapeSize);
        stereotypeIcon.iconShape().visitShapes(&sizeVisitor);
        QRectF iconBoundingRect = sizeVisitor.boundingRect();
        QPixmap pixmap(iconBoundingRect.width() + margins.left() + margins.right(),
                       iconBoundingRect.height() + margins.top() + margins.bottom());
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setBrush(Qt::NoBrush);
        painter.translate(-iconBoundingRect.topLeft() + QPointF(margins.left(), margins.top()));
        QPen linePen = style->linePen();
        linePen.setWidthF(2.0);
        painter.setPen(linePen);
        painter.setBrush(style->fillBrush());
        ShapePaintVisitor visitor(&painter, QPointF(0.0, 0.0),
                                       QSizeF(stereotypeIcon.width(), stereotypeIcon.height()),
                                       shapeSize, shapeSize);
        stereotypeIcon.iconShape().visitShapes(&visitor);

        QPixmap iconPixmap(size);
        iconPixmap.fill(Qt::transparent);
        QPainter iconPainter(&iconPixmap);
        iconPainter.drawPixmap((iconPixmap.width() - pixmap.width()) / 2,
                               (iconPixmap.width() - pixmap.height()) / 2, pixmap);
        icon = QIcon(iconPixmap);
    }
    if (icon.isNull() && !defaultIconPath.isEmpty())
        icon = QIcon(defaultIconPath);
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

void StereotypeController::addToolbar(const Toolbar &toolbar)
{
    d->m_toolbars.append(toolbar);
}

} // namespace qmt

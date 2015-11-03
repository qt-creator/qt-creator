/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

struct StereotypeController::StereotypeControllerPrivate
{
    StereotypeControllerPrivate()
    {
    }

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

QList<StereotypeIcon> StereotypeController::getStereotypeIcons() const
{
    return d->m_iconIdToStereotypeIconsMap.values();
}

QList<Toolbar> StereotypeController::getToolbars() const
{
    return d->m_toolbars;
}

QList<QString> StereotypeController::getKnownStereotypes(StereotypeIcon::Element stereotype_element) const
{
    QSet<QString> stereotypes;
    foreach (const StereotypeIcon &icon, d->m_iconIdToStereotypeIconsMap.values()) {
        if (icon.getElements().isEmpty() || icon.getElements().contains(stereotype_element)) {
            stereotypes += icon.getStereotypes();
        }
    }
    QList<QString> list = stereotypes.toList();
    std::sort(list.begin(), list.end());
    return list;
}

QString StereotypeController::findStereotypeIconId(StereotypeIcon::Element element, const QList<QString> &stereotypes) const
{
    foreach (const QString &stereotype, stereotypes) {
        if (d->m_stereotypeToIconIdMap.contains(qMakePair(element, stereotype))) {
            return d->m_stereotypeToIconIdMap.value(qMakePair(element, stereotype));
        } else if (d->m_stereotypeToIconIdMap.contains(qMakePair(StereotypeIcon::ELEMENT_ANY, stereotype))) {
            return d->m_stereotypeToIconIdMap.value(qMakePair(StereotypeIcon::ELEMENT_ANY, stereotype));
        }
    }
    return QString();
}

QList<QString> StereotypeController::filterStereotypesByIconId(const QString &stereotype_icon_id, const QList<QString> &stereotypes) const
{
    if (!d->m_iconIdToStereotypeIconsMap.contains(stereotype_icon_id)) {
        return stereotypes;
    }
    QList<QString> filtered_stereotypes = stereotypes;
    foreach (const QString &stereotype, d->m_iconIdToStereotypeIconsMap.value(stereotype_icon_id).getStereotypes()) {
        filtered_stereotypes.removeAll(stereotype);
    }
    return filtered_stereotypes;
}

StereotypeIcon StereotypeController::findStereotypeIcon(const QString &stereotype_icon_id)
{
    QMT_CHECK(d->m_iconIdToStereotypeIconsMap.contains(stereotype_icon_id));
    return d->m_iconIdToStereotypeIconsMap.value(stereotype_icon_id);
}

QIcon StereotypeController::createIcon(StereotypeIcon::Element element, const QList<QString> &stereotypes, const QString &default_icon_path,
                                       const Style *style, const QSize &size, const QMarginsF &margins)
{
    // TODO implement cache with key build from element, stereotypes, default_icon_path, style, size and margins
    // TODO implement unique id for style which can be used as key
    // TODO fix rendering of icon which negativ extension of bounding box (e.g. stereotype "component")
    QIcon icon;
    QString stereotype_icon_id = findStereotypeIconId(element, stereotypes);
    if (!stereotype_icon_id.isEmpty()) {
        StereotypeIcon stereotype_icon = findStereotypeIcon(stereotype_icon_id);

        qreal width = size.width() - margins.left() - margins.right();
        qreal height = size.height() - margins.top() - margins.bottom();
        qreal ratio_width = height * stereotype_icon.getWidth() / stereotype_icon.getHeight();
        qreal ratio_height = width * stereotype_icon.getHeight() / stereotype_icon.getWidth();
        if (ratio_width > width) {
            height = ratio_height;
        } else if (ratio_height > height) {
            width = ratio_width;
        }
        QSizeF shape_size(width, height);

        ShapeSizeVisitor size_visitor(QPointF(0.0, 0.0),
                                           QSizeF(stereotype_icon.getWidth(), stereotype_icon.getHeight()),
                                           shape_size, shape_size);
        stereotype_icon.getIconShape().visitShapes(&size_visitor);
        QRectF icon_bounding_rect = size_visitor.getBoundingRect();
        QPixmap pixmap(icon_bounding_rect.width() + margins.left() + margins.right(), icon_bounding_rect.height() + margins.top() + margins.bottom());
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setBrush(Qt::NoBrush);
        painter.translate(-icon_bounding_rect.topLeft() + QPointF(margins.left(), margins.top()));
        QPen line_pen = style->getLinePen();
        line_pen.setWidthF(2.0);
        painter.setPen(line_pen);
        painter.setBrush(style->getFillBrush());
        ShapePaintVisitor visitor(&painter, QPointF(0.0, 0.0),
                                       QSizeF(stereotype_icon.getWidth(), stereotype_icon.getHeight()),
                                       shape_size, shape_size);
        stereotype_icon.getIconShape().visitShapes(&visitor);

        QPixmap icon_pixmap(size);
        icon_pixmap.fill(Qt::transparent);
        QPainter icon_painter(&icon_pixmap);
        icon_painter.drawPixmap((icon_pixmap.width() - pixmap.width()) / 2, (icon_pixmap.width() - pixmap.height()) / 2, pixmap);
        icon = QIcon(icon_pixmap);
    }
    if (icon.isNull() && !default_icon_path.isEmpty()) {
        icon = QIcon(default_icon_path);
    }
    return icon;

}

void StereotypeController::addStereotypeIcon(const StereotypeIcon &stereotype_icon)
{
    if (stereotype_icon.getElements().isEmpty()) {
        foreach (const QString &stereotype, stereotype_icon.getStereotypes()) {
            d->m_stereotypeToIconIdMap.insert(qMakePair(StereotypeIcon::ELEMENT_ANY, stereotype), stereotype_icon.getId());
        }
    } else {
        foreach (StereotypeIcon::Element element, stereotype_icon.getElements()) {
            foreach (const QString &stereotype, stereotype_icon.getStereotypes()) {
                d->m_stereotypeToIconIdMap.insert(qMakePair(element, stereotype), stereotype_icon.getId());
            }
        }
    }
    d->m_iconIdToStereotypeIconsMap.insert(stereotype_icon.getId(), stereotype_icon);
}

void StereotypeController::addToolbar(const Toolbar &toolbar)
{
    d->m_toolbars.append(toolbar);
}

} // namespace qmt

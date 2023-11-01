// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "stereotypeicon.h"

#include <QMarginsF>
#include <QObject>

namespace qmt {

class CustomRelation;
class Toolbar;
class Style;

class QMT_EXPORT StereotypeController : public QObject
{
    class StereotypeControllerPrivate;

public:
    explicit StereotypeController(QObject *parent = nullptr);
    ~StereotypeController() override;

    QList<StereotypeIcon> stereotypeIcons() const;
    QList<Toolbar> toolbars() const;
    QList<Toolbar> findToolbars(const QString &elementType) const;
    QList<QString> knownStereotypes(StereotypeIcon::Element stereotypeElement) const;

    QString findStereotypeIconId(StereotypeIcon::Element element,
                                 const QList<QString> &stereotypes) const;
    QList<QString> filterStereotypesByIconId(const QString &stereotypeIconId,
                                             const QList<QString> &stereotypes) const;
    StereotypeIcon findStereotypeIcon(const QString &stereotypeIconId) const;
    CustomRelation findCustomRelation(const QString &customRelationId) const;
    QIcon createIcon(StereotypeIcon::Element element, const QList<QString> &stereotypes,
                     const QString &defaultIconPath, const Style *style,
                     const QSize &size, const QMarginsF &margins, qreal lineWidth);

    void addStereotypeIcon(const StereotypeIcon &stereotypeIcon);
    void addCustomRelation(const CustomRelation &customRelation);
    void addToolbar(const Toolbar &toolbar);

private:
    StereotypeControllerPrivate *d;
};

} // namespace qmt

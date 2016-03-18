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

#pragma once

#include <QObject>

#include "stereotypeicon.h"

#include <QMarginsF>

namespace qmt {

class Toolbar;
class Style;

class QMT_EXPORT StereotypeController : public QObject
{
    Q_OBJECT
    class StereotypeControllerPrivate;

public:
    explicit StereotypeController(QObject *parent = 0);
    ~StereotypeController() override;

    QList<StereotypeIcon> stereotypeIcons() const;
    QList<Toolbar> toolbars() const;
    QList<QString> knownStereotypes(StereotypeIcon::Element stereotypeElement) const;

    QString findStereotypeIconId(StereotypeIcon::Element element,
                                 const QList<QString> &stereotypes) const;
    QList<QString> filterStereotypesByIconId(const QString &stereotypeIconId,
                                             const QList<QString> &stereotypes) const;
    StereotypeIcon findStereotypeIcon(const QString &stereotypeIconId);
    QIcon createIcon(StereotypeIcon::Element element, const QList<QString> &stereotypes,
                     const QString &defaultIconPath, const Style *style,
                     const QSize &size, const QMarginsF &margins);

    void addStereotypeIcon(const StereotypeIcon &stereotypeIcon);
    void addToolbar(const Toolbar &toolbar);

private:
    StereotypeControllerPrivate *d;
};

} // namespace qmt

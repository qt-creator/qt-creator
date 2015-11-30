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

#ifndef QMT_STEREOTYPECONTROLLER_H
#define QMT_STEREOTYPECONTROLLER_H

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

#endif // QMT_STEREOTYPECONTROLLER_H

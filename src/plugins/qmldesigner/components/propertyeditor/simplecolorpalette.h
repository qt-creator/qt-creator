/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
#include <QtQml/qqml.h>
#include <QColor>

namespace QmlDesigner {

class PaletteColor
{
    Q_GADGET

    Q_PROPERTY(QColor color READ color FINAL)
    Q_PROPERTY(QString colorCode READ colorCode FINAL)
    Q_PROPERTY(bool isFavorite READ isFavorite FINAL)
public:
    PaletteColor();
    PaletteColor(const QString &colorCode);
    PaletteColor(const QColor &value);
    ~PaletteColor() = default;

    enum Property {
        objectNameRole = 0,
        colorRole = 1,
        colorCodeRole = 2,
        isFavoriteRole = 3
    };

    QVariant getProperty(Property id) const;

    QColor color() const;
    void setColor(const QColor &value);

    QString colorCode() const;

    bool isFavorite() const;
    void setFavorite(bool favorite);
    bool toggleFavorite();

    bool operator==(const PaletteColor &other) const;

private:
    QColor m_color;
    QString m_colorCode;
    bool m_isFavorite;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PaletteColor)

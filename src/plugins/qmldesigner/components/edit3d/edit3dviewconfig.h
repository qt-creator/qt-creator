/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <view3dactioncommand.h>

#include <nodeinstanceview.h>
#include <qmldesignerplugin.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

class Edit3DViewConfig
{
public:
    static QList<QColor> load(const char key[])
    {
        QVariant var = QmlDesignerPlugin::settings().value(key);

        if (!var.isValid())
            return {};

        auto colorNameList = var.value<QStringList>();

        return Utils::transform(colorNameList, [](const QString &colorName) {
            return QColor{colorName};
        });
    }

    static void set(View3DActionCommand::Type type, const QList<QColor> &colorConfig)
    {
        if (colorConfig.size() == 1)
            set(type, colorConfig.at(0));
        else
            setVariant(type, QVariant::fromValue(colorConfig));
    }

    static void set(View3DActionCommand::Type type, const QColor &color)
    {
        setVariant(type, QVariant::fromValue(color));
    }

    static void save(const QByteArray &key, const QList<QColor> &colorConfig)
    {
        QStringList colorNames = Utils::transform(colorConfig, [](const QColor &color) {
            return color.name();
        });

        saveVariant(key, QVariant::fromValue(colorNames));
    }

    static void save(const QByteArray &key, const QColor &color)
    {
        saveVariant(key, QVariant::fromValue(color.name()));
    }

    static bool isValid(const QList<QColor> &colorConfig) { return !colorConfig.isEmpty(); }

private:
    static void setVariant(View3DActionCommand::Type type, const QVariant &colorConfig)
    {
        auto view = QmlDesignerPlugin::instance()->viewManager().nodeInstanceView();
        View3DActionCommand cmd(type, colorConfig);
        view->view3DAction(cmd);
    }

    static void saveVariant(const QByteArray &key, const QVariant &colorConfig)
    {
        QmlDesignerPlugin::settings().insert(key, colorConfig);
    }
};

} // namespace QmlDesigner

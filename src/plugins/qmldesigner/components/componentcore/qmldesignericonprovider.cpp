/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmldesignericonprovider.h"

#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>

#include <QDebug>

namespace QmlDesigner {

QmlDesignerIconProvider::QmlDesignerIconProvider()
    : QQuickImageProvider(Pixmap)
{

}

static QString iconPath()
{
    return Core::ICore::resourcePath() + QLatin1String("/qmldesigner/propertyEditorQmlSources/HelperWidgets/images/");
}

QPixmap QmlDesignerIconProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize)

    static Utils::Icon UP_ARROW({
            { iconPath() + QLatin1String("up-arrow.png"), Utils::Theme::IconsBaseColor}});

    static Utils::Icon DOWN_ARROW({
            { iconPath() + QLatin1String("down-arrow.png"), Utils::Theme::IconsBaseColor}});

    static Utils::Icon PLACEHOLDER({
            { iconPath() + QLatin1String("placeholder.png"), Utils::Theme::IconsBaseColor}});

    static Utils::Icon EXPRESSION({
            { iconPath() + QLatin1String("expression.png"), Utils::Theme::IconsBaseColor}});

    static Utils::Icon SUBMENU({
            { iconPath() + QLatin1String("submenu.png"), Utils::Theme::IconsBaseColor}});

    QPixmap result;

    if (id == "close")
        result = Core::Icons::CLOSE_TOOLBAR.pixmap();

    else if (id == "plus")
        result = Core::Icons::PLUS.pixmap();
    else if (id == "expression")
        result = EXPRESSION.pixmap();
    else if (id == "placeholder")
        result = PLACEHOLDER.pixmap();
    else if (id == "expression")
        result = EXPRESSION.pixmap();
    else if (id == "submenu")
        result = SUBMENU.pixmap();
    else if (id == "up-arrow")
        result = UP_ARROW.pixmap();
    else if (id == "down-arrow")
        result = DOWN_ARROW.pixmap();
    else
        qWarning() << Q_FUNC_INFO << "Image not found.";

    if (size)
        *size = result.size();
    return result;
}

} // namespace QmlDesigner

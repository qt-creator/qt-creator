// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldesignericonprovider.h"
#include <qmldesignericons.h>


#include <utils/icon.h>
#include <utils/utilsicons.h>

#include <coreplugin/icore.h>

#include <QDebug>

namespace QmlDesigner {

QmlDesignerIconProvider::QmlDesignerIconProvider()
    : QQuickImageProvider(Pixmap)
{

}

static Utils::FilePath iconPath()
{
    return Core::ICore::resourcePath(
        "qmldesigner/propertyEditorQmlSources/imports/HelperWidgets/images/");
}

QPixmap QmlDesignerIconProvider::requestPixmap(const QString &id,
                                               QSize *size,
                                               [[maybe_unused]] const QSize &requestedSize)
{
    QPixmap result = getPixmap(id);

    if (size)
        *size = result.size();
    return result;
}

QPixmap QmlDesignerIconProvider::getPixmap(const QString &id)
{
    using namespace Utils;

    QPixmap result;

    if (id == "close")
        result = Utils::Icons::CLOSE_TOOLBAR.pixmap();
    else if (id == "plus")
        result = Utils::Icons::PLUS_TOOLBAR.pixmap();
    else if (id == "expression")
        result = Icon({{iconPath() / "expression.png", Theme::QmlDesigner_HighlightColor}}).pixmap();
    else if (id == "placeholder")
        result = Icon(iconPath() / "placeholder.png").pixmap();
    else if (id == "submenu")
        result = Icon(iconPath() / "submenu.png").pixmap();
    else if (id == "up-arrow")
        result = Icon({{iconPath() / "up-arrow.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "down-arrow")
        result = Icon({{iconPath() / "down-arrow.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "checkbox-indicator")
        result = Icon({
                { ":/qmldesigner/images/checkbox_indicator.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "tr")
        result = Icon({
                { ":/qmldesigner/images/tr.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "ok")
        result = Icon({
                { ":/utils/images/ok.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "error")
        result = Icon({
                { ":/utils/images/broken.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "anchor-top")
        result = Icon({
                { ":/qmldesigner/images/anchor_top.png", Theme::IconsBaseColor},
                { ":/qmldesigner/images/anchoreditem.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "anchor-right")
        result = Icon({
                { ":/qmldesigner/images/anchor_right.png", Theme::IconsBaseColor},
                { ":/qmldesigner/images/anchoreditem.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "anchor-bottom")
        result = Icon({
                { ":/qmldesigner/images/anchor_bottom.png", Theme::IconsBaseColor},
                { ":/qmldesigner/images/anchoreditem.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "anchor-left")
        result = Icon({
                { ":/qmldesigner/images/anchor_left.png", Theme::IconsBaseColor},
                { ":/qmldesigner/images/anchoreditem.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "anchor-horizontal")
        result = Icon({
                { ":/qmldesigner/images/anchor_horizontal.png", Theme::IconsBaseColor},
                { ":/qmldesigner/images/anchoreditem.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "anchor-vertical")
        result = Icon({
                { ":/qmldesigner/images/anchor_vertical.png", Theme::IconsBaseColor},
                { ":/qmldesigner/images/anchoreditem.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "anchor-fill")
        result = Icon({
                { ":/qmldesigner/images/anchor_fill.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "alignment-left")
        result = Icon({
                { ":/qmldesigner/images/alignment_left.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "alignment-left-h")
        result = Icon({
                { ":/qmldesigner/images/alignment_left.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).pixmap();
    else if (id == "alignment-center")
        result = Icon({
                { ":/qmldesigner/images/alignment_center.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "alignment-center-h")
        result = Icon({
                { ":/qmldesigner/images/alignment_center.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).pixmap();
    else if (id == "alignment-right")
        result = Icon({
                { ":/qmldesigner/images/alignment_right.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "alignment-right-h")
        result = Icon({
                { ":/qmldesigner/images/alignment_right.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).pixmap();
    else if (id == "alignment-top")
        result = Icon({
                { ":/qmldesigner/images/alignment_top.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "alignment-top-h")
        result = Icon({
                { ":/qmldesigner/images/alignment_top.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).pixmap();
    else if (id == "alignment-middle")
        result = Icon({
                { ":/qmldesigner/images/alignment_middle.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "alignment-middle-h")
        result = Icon({
                { ":/qmldesigner/images/alignment_middle.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).pixmap();
    else if (id == "alignment-bottom")
        result = Icon({
                { ":/qmldesigner/images/alignment_bottom.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "alignment-bottom-h")
        result = Icon({
                { ":/qmldesigner/images/alignment_bottom.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).pixmap();
    else if (id == "style-bold")
        result = Icon({
                { ":/qmldesigner/images/style_bold.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "style-bold-h")
        result = Icon({
                { ":/qmldesigner/images/style_bold.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).pixmap();
    else if (id == "style-italic")
        result = Icon({
                { ":/qmldesigner/images/style_italic.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "style-italic-h")
        result = Icon({
                { ":/qmldesigner/images/style_italic.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).pixmap();
    else if (id == "style-underline")
        result = Icon({
                { ":/qmldesigner/images/style_underline.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "style-underline-h")
        result = Icon({
                { ":/qmldesigner/images/style_underline.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).pixmap();
    else if (id == "style-strikeout")
        result = Icon({
                { ":/qmldesigner/images/style_strikeout.png", Theme::IconsBaseColor}}, Icon::Tint).pixmap();
    else if (id == "style-strikeout-h")
        result = Icon({
                { ":/qmldesigner/images/style_strikeout.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).pixmap();
    else if (id == "alias-export-checked")
        result = Icons::EXPORT_CHECKED.pixmap();
    else if (id == "alias-export-unchecked")
        result = Icons::EXPORT_UNCHECKED.pixmap();
    else
        qWarning() << Q_FUNC_INFO << "Image not found:" << id;

    return result;
}

} // namespace QmlDesigner

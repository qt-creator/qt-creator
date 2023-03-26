// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textnodedumper.h"
#include "assetexportpluginconstants.h"

#include <QColor>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QHash>
#include <QtMath>

#include <private/qquicktext_p.h>

#include <algorithm>

namespace  {
const QHash<QString, QString> AlignMapping{
    {"AlignRight", "RIGHT"},
    {"AlignHCenter", "CENTER"},
    {"AlignJustify", "JUSTIFIED"},
    {"AlignLeft", "LEFT"},
    {"AlignTop", "TOP"},
    {"AlignVCenter", "CENTER"},
    {"AlignBottom", "BOTTOM"}
};

QString toJsonAlignEnum(QString value) {
    if (value.isEmpty() || !AlignMapping.contains(value))
        return {};
    return AlignMapping[value];
}
}


namespace QmlDesigner {
using namespace Constants;
TextNodeDumper::TextNodeDumper(const QByteArrayList &lineage, const ModelNode &node) :
    ItemNodeDumper(lineage, node)
{

}

bool TextNodeDumper::isExportable() const
{
    const QByteArrayList &baseClasses = lineage();
    return std::any_of(baseClasses.cbegin(), baseClasses.cend(), [](const QByteArray &type) {
        return type == "QtQuick.Text" || type == "QtQuick.Controls.Label";
    });
}

QJsonObject TextNodeDumper::json([[maybe_unused]] Component &component) const
{
    QJsonObject jsonObject = ItemNodeDumper::json(component);

    QJsonObject textDetails;
    textDetails.insert(TextContentTag, propertyValue("text").toString());

    QFont font = propertyValue("font").value<QFont>();
    QFontInfo fontInfo(font);
    textDetails.insert(FontFamilyTag, fontInfo.family());
    textDetails.insert(FontStyleTag, fontInfo.styleName());
    textDetails.insert(FontSizeTag, fontInfo.pixelSize());
    textDetails.insert(LetterSpacingTag, font.letterSpacing());


    QColor fontColor(propertyValue("font.color").toString());
    textDetails.insert(TextColorTag, fontColor.name(QColor::HexArgb));

    textDetails.insert(HAlignTag, toJsonAlignEnum(propertyValue("horizontalAlignment").toString()));
    textDetails.insert(VAlignTag, toJsonAlignEnum(propertyValue("verticalAlignment").toString()));

    textDetails.insert(IsMultilineTag, propertyValue("wrapMode").toString().compare("NoWrap") != 0);

    // Calculate line height in pixels
    QFontMetricsF fm(font);
    auto lineHeightMode = propertyValue("lineHeightMode").value<QQuickText::LineHeightMode>();
    double lineHeight = propertyValue("lineHeight").toDouble();
    qreal lineHeightPx = (lineHeightMode == QQuickText::FixedHeight) ?
                lineHeight : qCeil(fm.height()) * lineHeight;
    textDetails.insert(LineHeightTag, lineHeightPx);

    QJsonObject metadata = jsonObject.value(MetadataTag).toObject();
    metadata.insert(TextDetailsTag, textDetails);
    jsonObject.insert(MetadataTag, metadata);
    return  jsonObject;
}
}

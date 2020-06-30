/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "textnodeparser.h"
#include "assetexportpluginconstants.h"

#include <QColor>
#include <QHash>

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
TextNodeParser::TextNodeParser(const QByteArrayList &lineage, const ModelNode &node) :
    ItemNodeParser(lineage, node)
{

}

bool TextNodeParser::isExportable() const
{
    return lineage().contains("QtQuick.Text");
}

QJsonObject TextNodeParser::json(Component &component) const
{
    Q_UNUSED(component);
    QJsonObject jsonObject = ItemNodeParser::json(component);

    QJsonObject textDetails;
    textDetails.insert(TextContentTag, propertyValue("text").toString());
    textDetails.insert(FontFamilyTag, propertyValue("font.family").toString());
    textDetails.insert(FontStyleTag, propertyValue("font.styleName").toString());
    textDetails.insert(FontSizeTag, propertyValue("font.pixelSize").toInt());
    textDetails.insert(LetterSpacingTag, propertyValue("font.letterSpacing").toFloat());

    QColor fontColor(propertyValue("font.color").toString());
    textDetails.insert(TextColorTag, fontColor.name(QColor::HexArgb));

    textDetails.insert(HAlignTag, toJsonAlignEnum(propertyValue("horizontalAlignment").toString()));
    textDetails.insert(VAlignTag, toJsonAlignEnum(propertyValue("verticalAlignment").toString()));

    textDetails.insert(IsMultilineTag, propertyValue("wrapMode").toString().compare("NoWrap") != 0);

    jsonObject.insert(TextDetailsTag, textDetails);
    return  jsonObject;
}
}

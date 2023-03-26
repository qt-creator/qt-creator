// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <qmlitemnode.h>

#include <QDomDocument>

namespace QmlDesigner {

struct CSSProperty
{
    QString directive;
    QString value;
};

using CSSRule = std::vector<CSSProperty>;
using CSSRules = QHash<QString, CSSRule>;
using PropertyMap = QHash<QByteArray, QVariant>;

class SVGPasteAction
{
public:
    SVGPasteAction();

    bool containsSVG(const QString &str);

    QmlObjectNode createQmlObjectNode(QmlDesigner::ModelNode &targetNode);

private:
    QDomDocument m_domDocument;
};

} // namespace QmlDesigner

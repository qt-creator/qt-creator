// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

#include <texteditor/tabsettings.h>

#include "abstractproperty.h"
#include "modelnode.h"

namespace QmlDesigner {
namespace Internal {

class QmlTextGenerator
{
public:
    explicit QmlTextGenerator(const PropertyNameList &propertyOrder,
                              const TextEditor::TabSettings &tabSettings,
                              const int startIndentDepth = 0);

    QString operator()(const AbstractProperty &property) const
    { return toQml(property, m_startIndentDepth); }

    QString operator()(const ModelNode &modelNode) const
    { return toQml(modelNode, m_startIndentDepth); }

private:
    QString toQml(const AbstractProperty &property, int indentDepth) const;
    QString toQml(const ModelNode &modelNode, int indentDepth) const;
    QString propertiesToQml(const ModelNode &node, int indentDepth) const;
    QString propertyToQml(const AbstractProperty &property, int indentDepth) const;

private:
    PropertyNameList m_propertyOrder;
    TextEditor::TabSettings m_tabSettings;
    const int m_startIndentDepth;
};

} // namespace Internal
} // namespace QmlDesigner

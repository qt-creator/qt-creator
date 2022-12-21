// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>

namespace QmlDesigner {

class CORESHARED_EXPORT ObjectPropertyBinding
{
public:
    ObjectPropertyBinding();
    ObjectPropertyBinding(const ModelNode &node);


    ModelNode modelNode() const;
    bool isValid() const;

private:
    ModelNode m_node;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ObjectPropertyBinding);

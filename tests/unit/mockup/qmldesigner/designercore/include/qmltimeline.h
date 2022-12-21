// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlmodelnodefacade.h"
#include <qmldesignercorelib_global.h>

namespace QmlDesigner {

class QmlTimeline : public QmlModelNodeFacade
{
public:
    QmlTimeline() {}
    QmlTimeline(const ModelNode &) {}

    bool isValid() const override { return {}; }

    void toogleRecording(bool) const {}

    void resetGroupRecording() const {}
};

} // namespace QmlDesigner

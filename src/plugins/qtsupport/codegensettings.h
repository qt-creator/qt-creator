// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

#include <utils/aspects.h>

namespace QtSupport {

class QTSUPPORT_EXPORT CodeGenSettings : public Utils::AspectContainer
{
public:
    CodeGenSettings();

    // How to embed the Ui::Form class.
    enum UiClassEmbedding
    {
        PointerAggregatedUiClass, // "Ui::Form *m_ui";
        AggregatedUiClass,        // "Ui::Form m_ui";
        InheritedUiClass          // "...private Ui::Form..."
    };

    Utils::SelectionAspect embedding{this};
    Utils::BoolAspect retranslationSupport{this}; // Add handling for language change events
    Utils::BoolAspect includeQtModule{this}; // Include "<QtGui/[Class]>" or just "<[Class]>"
    Utils::BoolAspect addQtVersionCheck{this}; // Include #ifdef when using "#include <QtGui/..."
};

QTSUPPORT_EXPORT CodeGenSettings &codeGenSettings();

} // namespace QtSupport

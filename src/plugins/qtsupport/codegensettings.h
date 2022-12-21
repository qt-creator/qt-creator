// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

QT_FORWARD_DECLARE_CLASS(QSettings)

namespace QtSupport {

class QTSUPPORT_EXPORT CodeGenSettings
{
public:
    // How to embed the Ui::Form class.
    enum UiClassEmbedding
    {
        PointerAggregatedUiClass, // "Ui::Form *m_ui";
        AggregatedUiClass,        // "Ui::Form m_ui";
        InheritedUiClass          // "...private Ui::Form..."
    };

    CodeGenSettings();
    bool equals(const CodeGenSettings &rhs) const;

    void fromSettings(const QSettings *settings);
    void toSettings(QSettings *settings) const;

    friend bool operator==(const CodeGenSettings &p1, const CodeGenSettings &p2) { return p1.equals(p2); }
    friend bool operator!=(const CodeGenSettings &p1, const CodeGenSettings &p2) { return !p1.equals(p2); }

    UiClassEmbedding embedding;
    bool retranslationSupport; // Add handling for language change events
    bool includeQtModule; // Include "<QtGui/[Class]>" or just "<[Class]>"
    bool addQtVersionCheck; // Include #ifdef when using "#include <QtGui/..."
};

} // namespace QtSupport

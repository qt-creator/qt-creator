/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CODEGENSETTINGS_H
#define CODEGENSETTINGS_H

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

    UiClassEmbedding embedding;
    bool retranslationSupport; // Add handling for language change events
    bool includeQtModule; // Include "<QtGui/[Class]>" or just "<[Class]>"
    bool addQtVersionCheck; // Include #ifdef when using "#include <QtGui/..."
};

inline bool operator==(const CodeGenSettings &p1, const CodeGenSettings &p2) { return p1.equals(p2); }
inline bool operator!=(const CodeGenSettings &p1, const CodeGenSettings &p2) { return !p1.equals(p2); }

} // namespace QtSupport

#endif // CODEGENSETTINGS_H

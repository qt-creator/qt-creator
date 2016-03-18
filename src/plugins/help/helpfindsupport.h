/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "centralwidget.h"

#include <coreplugin/find/ifindsupport.h>

namespace Help {
namespace Internal {

class HelpViewer;

class HelpViewerFindSupport : public Core::IFindSupport
{
    Q_OBJECT
public:
    HelpViewerFindSupport(HelpViewer *viewer);

    bool supportsReplace() const { return false; }
    Core::FindFlags supportedFindFlags() const;
    void resetIncrementalSearch() {}
    void clearHighlights() {}
    QString currentFindString() const;
    QString completedFindString() const { return QString(); }

    Result findIncremental(const QString &txt, Core::FindFlags findFlags);
    Result findStep(const QString &txt, Core::FindFlags findFlags);

private:
    bool find(const QString &ttf, Core::FindFlags findFlags, bool incremental);
    HelpViewer *m_viewer;
};

} // namespace Internal
} // namespace Help

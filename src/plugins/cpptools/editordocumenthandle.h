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

#include "cpptools_global.h"
#include "senddocumenttracker.h"

#include <QString>

namespace CppTools {
class BaseEditorDocumentProcessor;

class CPPTOOLS_EXPORT CppEditorDocumentHandle
{
public:
    virtual ~CppEditorDocumentHandle();

    enum RefreshReason {
        None,
        ProjectUpdate,
        Other,
    };
    RefreshReason refreshReason() const;
    void setRefreshReason(const RefreshReason &refreshReason);

    // For the Working Copy
    virtual QString filePath() const = 0;
    virtual QByteArray contents() const = 0;
    virtual unsigned revision() const = 0;

    // For updating if new project info is set
    virtual BaseEditorDocumentProcessor *processor() const = 0;

    virtual void resetProcessor() = 0;

    SendDocumentTracker &sendTracker();

private:
    SendDocumentTracker m_sendTracker;
    RefreshReason m_refreshReason = None;
};

} // namespace CppTools

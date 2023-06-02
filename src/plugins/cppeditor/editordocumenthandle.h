// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

namespace Utils { class FilePath; }

namespace CppEditor {

class BaseEditorDocumentProcessor;

class CPPEDITOR_EXPORT CppEditorDocumentHandle
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
    virtual Utils::FilePath filePath() const = 0;
    virtual QByteArray contents() const = 0;
    virtual unsigned revision() const = 0;

    // For updating if new project info is set
    virtual BaseEditorDocumentProcessor *processor() const = 0;

    virtual void resetProcessor() = 0;

private:
    RefreshReason m_refreshReason = None;
};

} // CppEditor

// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbasesubmiteditor.h>

namespace ClearCase::Internal {

class ClearCaseSubmitEditorWidget;

class ClearCaseSubmitEditor : public VcsBase::VcsBaseSubmitEditor
{
    Q_OBJECT

public:
    ClearCaseSubmitEditor();

    static QString fileFromStatusLine(const QString &statusLine);

    void setStatusList(const QStringList &statusOutput);
    ClearCaseSubmitEditorWidget *submitEditorWidget();

    void setIsUcm(bool isUcm);

protected:
    QByteArray fileContents() const override;
};

} // ClearCase::Internal

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/submiteditorwidget.h>

namespace Perforce::Internal {

class SubmitPanel;

/* Submit editor widget with additional information pane
 * at the top. */
class PerforceSubmitEditorWidget : public VcsBase::SubmitEditorWidget
{
public:
    PerforceSubmitEditorWidget();

    void setData(const QString &change, const QString &client, const QString &userName);

private:
    SubmitPanel *m_submitPanel;
};

} // Perforce::Internal

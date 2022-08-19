// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "ui_submitpanel.h"
#include <vcsbase/submiteditorwidget.h>

namespace Perforce {
namespace Internal {

/* Submit editor widget with additional information pane
 * at the top. */
class PerforceSubmitEditorWidget : public VcsBase::SubmitEditorWidget
{
public:
    PerforceSubmitEditorWidget();

    void setData(const QString &change, const QString &client, const QString &userName);

private:
    QGroupBox *m_submitPanel;
    Ui::SubmitPanel m_submitPanelUi;
};

} // namespace Internal
} // namespace Perforce

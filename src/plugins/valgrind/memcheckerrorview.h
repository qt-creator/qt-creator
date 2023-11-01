// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/analyzer/detailederrorview.h>

#include <utils/filepath.h>

#include <QListView>

namespace Valgrind::Internal {

class ValgrindSettings;

class MemcheckErrorView : public Debugger::DetailedErrorView
{
public:
    MemcheckErrorView(QWidget *parent = nullptr);
    ~MemcheckErrorView() override;

    void setDefaultSuppressionFile(const Utils::FilePath &suppFile);
    Utils::FilePath defaultSuppressionFile() const;
    ValgrindSettings *settings() const { return m_settings; }
    void settingsChanged(ValgrindSettings *settings);

private:
    void suppressError();
    QList<QAction *> customActions() const override;

    QAction *m_suppressAction;
    Utils::FilePath m_defaultSuppFile;
    ValgrindSettings *m_settings = nullptr;
};

} // namespace Valgrind::Internal

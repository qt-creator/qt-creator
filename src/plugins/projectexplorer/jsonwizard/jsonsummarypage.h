// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectwizardpage.h"
#include "jsonwizard.h"

#include <QWizardPage>

namespace ProjectExplorer {

class FolderNode;
class Node;

// Documentation inside.
class JsonSummaryPage : public Internal::ProjectWizardPage
{
    Q_OBJECT

public:
    JsonSummaryPage(QWidget *parent = nullptr);
    void setHideProjectUiValue(const QVariant &hideProjectUiValue);

    void initializePage() override;
    bool validatePage() override;
    void cleanupPage() override;

    void triggerCommit(const JsonWizard::GeneratorFiles &files);
    void addToProject(const JsonWizard::GeneratorFiles &files);
    void summarySettingsHaveChanged();

private:
    Node *findWizardContextNode(Node *contextNode) const;
    void updateFileList();
    void updateProjectData(FolderNode *node);

    JsonWizard *m_wizard;
    JsonWizard::GeneratorFiles m_fileList;
    QVariant m_hideProjectUiValue;
};

} // namespace ProjectExplorer

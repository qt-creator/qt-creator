// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

#include <QDialog>

namespace ProjectExplorer { class Kit; }
namespace Utils { class FilePath; }

namespace Debugger::Internal {

class AttachCoreDialog : public QDialog
{
public:
    explicit AttachCoreDialog(QWidget *parent);
    ~AttachCoreDialog() override;

    int exec() override;

    Utils::FilePath symbolFile() const;
    Utils::FilePath coreFile() const;
    Utils::FilePath overrideStartScript() const;
    Utils::FilePath sysRoot() const;

    // For persistance.
    ProjectExplorer::Kit *kit() const;
    void setSymbolFile(const Utils::FilePath &symbolFilePath);
    void setCoreFile(const Utils::FilePath &coreFilePath);
    void setOverrideStartScript(const Utils::FilePath &scriptName);
    void setSysRoot(const Utils::FilePath &sysRoot);
    void setKitId(Utils::Id id);

    Utils::FilePath coreFileCopy() const;
    Utils::FilePath symbolFileCopy() const;

    void accepted();

private:
    void changed();
    void coreFileChanged(const Utils::FilePath &core);

    class AttachCoreDialogPrivate *d;
};

} // Debugger::Internal

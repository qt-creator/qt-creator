// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <diffeditor/diffeditorcontroller.h>

namespace Utils {
class Environment;
class Process;
} // Utils

namespace VcsBase {

class VcsBaseDiffEditorControllerPrivate;

class VCSBASE_EXPORT VcsBaseDiffEditorController : public DiffEditor::DiffEditorController
{
public:
    explicit VcsBaseDiffEditorController(Core::IDocument *document);
    ~VcsBaseDiffEditorController() override;

    void setProcessEnvironment(const Utils::Environment &value);
    void setVcsBinary(const Utils::FilePath &path);

protected:
    Tasking::GroupItem postProcessTask(const Tasking::TreeStorage<QString> &inputStorage);

    void setupCommand(Utils::Process &process, const QStringList &args) const;

private:
    friend class VcsBaseDiffEditorControllerPrivate;
    VcsBaseDiffEditorControllerPrivate *d;
};

} //namespace VcsBase

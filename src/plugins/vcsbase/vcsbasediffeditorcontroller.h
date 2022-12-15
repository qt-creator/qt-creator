// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"
#include "vcsenums.h"

#include <diffeditor/diffeditorcontroller.h>

QT_BEGIN_NAMESPACE
class QTextCodec;
QT_END_NAMESPACE

namespace Core { class IDocument; }

namespace Utils {
template <typename R> class AsyncTask;
class Environment;
class FilePath;
class QtcProcess;
} // Utils

namespace VcsBase {

class VcsBaseDiffEditorControllerPrivate;

class VCSBASE_EXPORT VcsBaseDiffEditorController : public DiffEditor::DiffEditorController
{
    Q_OBJECT

public:
    explicit VcsBaseDiffEditorController(Core::IDocument *document);
    ~VcsBaseDiffEditorController() override;

    void setProcessEnvironment(const Utils::Environment &value);
    void setVcsBinary(const Utils::FilePath &path);
    void setVcsTimeoutS(int value);

protected:
    Utils::Tasking::TreeStorage<QString> inputStorage() const;
    Utils::Tasking::TaskItem postProcessTask();

    void setupCommand(Utils::QtcProcess &process, const QStringList &args) const;
    void runCommand(const QList<QStringList> &args, RunFlags flags, QTextCodec *codec = nullptr);
    virtual void processCommandOutput(const QString &output);

private:
    friend class VcsBaseDiffEditorControllerPrivate;
    VcsBaseDiffEditorControllerPrivate *d;
};

} //namespace VcsBase

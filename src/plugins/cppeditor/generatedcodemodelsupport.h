// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "abstracteditorsupport.h"

#include <projectexplorer/projectnodes.h>
#include <projectexplorer/extracompiler.h>

#include <QDateTime>
#include <QHash>
#include <QSet>

namespace Core { class IEditor; }
namespace ProjectExplorer { class Project; }

namespace CppEditor {

class CPPEDITOR_EXPORT GeneratedCodeModelSupport : public AbstractEditorSupport
{
    Q_OBJECT

public:
    GeneratedCodeModelSupport(CppModelManager *modelmanager,
                              ProjectExplorer::ExtraCompiler *generator,
                              const Utils::FilePath &generatedFile);
    ~GeneratedCodeModelSupport() override;

    /// \returns the contents encoded in UTF-8.
    QByteArray contents() const override;
    QString fileName() const override; // The generated file
    QString sourceFileName() const override;

    static void update(const QList<ProjectExplorer::ExtraCompiler *> &generators);

private:
    void onContentsChanged(const Utils::FilePath &file);
    Utils::FilePath m_generatedFileName;
    ProjectExplorer::ExtraCompiler *m_generator;
};

} // CppEditor

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uicgenerator.h"

#include "baseqtversion.h"
#include "qtkitaspect.h"
#include "qtsupportutils.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/extracompiler.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>

#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport::Internal {

class UicGenerator final : public ProcessExtraCompiler
{
public:
    UicGenerator(const Project *project, const FilePath &source,
                 const FilePaths &targets, QObject *parent)
        : ProcessExtraCompiler(project, source, targets, parent)
    {
        QTC_ASSERT(targets.count() == 1, return);
    }

private:
    Parameters parameters() const override
    {
        QTC_ASSERT(targets().size() == 1, return {});

        Parameters params;

        Kit *kit = project()->activeKit();
        if (!kit)
            kit = KitManager::defaultKit();
        if (QtVersion *version = QtKitAspect::qtVersion(kit))
            params.command = {version->uicFilePath(), {"-p"}};
        params.postRunner = [target = targets().first()](Process *process) {
            return FileNameToContentsHash{std::make_pair(target, uiHeaderFromUic(process))};
        };

        return params;
    }
};

// UicGeneratorFactory

class UicGeneratorFactory final : public ExtraCompilerFactory
{
public:
    explicit UicGeneratorFactory(QObject *guard) : m_guard(guard) {}

private:
    FileType sourceType() const final { return FileType::Form; }

    QString sourceTag() const final { return QLatin1String("ui"); }

    ExtraCompiler *create(const Project *project,
                          const FilePath &source,
                          const FilePaths &targets) final
    {
        return new UicGenerator(project, source, targets, m_guard);
    }

    QObject *m_guard;
};

void setupUicGenerator(QObject *guard)
{
    static UicGeneratorFactory theUicGeneratorFactory(guard);
}

} // QtSupport::Internal

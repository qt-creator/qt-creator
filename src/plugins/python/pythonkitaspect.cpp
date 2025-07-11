// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonkitaspect.h"

#include "pythonconstants.h"
#include "pythonsettings.h"
#include "pythontr.h"
#include "pythonutils.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspect.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/guard.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcprocess.h>

#include <QComboBox>
#include <QSortFilterProxyModel>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python {
namespace Internal {

class PythonAspectModel : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    void reset()
    {
        if (QAbstractItemModel * const model = sourceModel()) {
            setSourceModel(nullptr);
            model->deleteLater();
        }
        ListModel<Interpreter> * const model = createInterpreterModel(this);
        model->setAllData(model->allData() << Interpreter("none", {}, {}));
        setSourceModel(model);
    }
};

class PythonKitAspectImpl final : public KitAspect
{
public:
    PythonKitAspectImpl(Kit *kit, const KitAspectFactory *kitInfo)
        : KitAspect(kit, kitInfo)
    {
        setManagingPage(Constants::C_PYTHONOPTIONS_PAGE_ID);

        const auto model = new PythonAspectModel(this);
        auto getter = [](const Kit &k) -> QVariant {
            if (const auto interpreter = PythonKitAspect::python(&k))
                return interpreter->id;
            return {};
        };
        auto setter = [](Kit &k, const QVariant &v) {
            PythonKitAspect::setPython(&k, v.toString());
        };
        auto resetModel = [model] { model->reset(); };
        addListAspectSpec({model, std::move(getter), std::move(setter), std::move(resetModel)});

        connect(PythonSettings::instance(),
                &PythonSettings::interpretersChanged,
                this,
                &PythonKitAspectImpl::refresh);
    }

    void refresh() override
    {
        KitAspect::refresh();
        emit changed(); // we need to emit changed here to update changes in the macro expander
    }
};

class PythonKitAspectFactory : public KitAspectFactory
{
public:
    PythonKitAspectFactory()
    {
        setId(PythonKitAspect::id());
        setDisplayName(Tr::tr("Python"));
        setDescription(Tr::tr("The interpreter used for Python based projects."));
        setPriority(10000);
    }

    Tasks validate(const Kit *k) const override
    {
        Tasks result;
        const std::optional<Interpreter> python = PythonKitAspect::python(k);
        if (!python)
            return result;
        const FilePath path = python->command;
        if (!path.isLocal())
            return result;
        if (path.isEmpty()) {
            result << BuildSystemTask(Task::Error, Tr::tr("No Python set up."));
        } else if (!path.exists()) {
            result << BuildSystemTask(Task::Error,
                                      Tr::tr("Python \"%1\" not found.").arg(path.toUserOutput()));
        } else if (!path.isExecutableFile()) {
            result << BuildSystemTask(Task::Error,
                                      Tr::tr("Python \"%1\" is not executable.")
                                          .arg(path.toUserOutput()));
        } else {
            if (!pipIsUsable(path)) {
                result << BuildSystemTask(
                    Task::Warning,
                    Tr::tr("Python \"%1\" does not contain a usable pip. pip is needed to install "
                           "Python "
                           "packages from the Python Package Index, like PySide and the Python "
                           "language server. To use any of that functionality "
                           "ensure that pip is installed for that Python.")
                        .arg(path.toUserOutput()));
            }
            if (!venvIsUsable(path)) {
                result << BuildSystemTask(
                    Task::Warning,
                    Tr::tr(
                        "Python \"%1\" does not contain a usable venv. venv is the recommended way "
                        "to isolate a development environment for a project from the globally "
                        "installed Python.")
                        .arg(path.toUserOutput()));
            }
        }
        return result;
    }

    ItemList toUserOutput(const Kit *k) const override
    {
        if (const std::optional<Interpreter> python = PythonKitAspect::python(k)) {
            return {{displayName(),
                     QString("%1 (%2)").arg(python->name).arg(python->command.toUserOutput())}};
        }
        return {};
    }

    KitAspect *createKitAspect(Kit *k) const override { return new PythonKitAspectImpl(k, this); }

    QSet<Id> availableFeatures(const Kit *k) const override
    {
        if (PythonKitAspect::python(k))
            return {PythonKitAspect::id()};
        return {};
    }

    void addToMacroExpander(Kit *kit, MacroExpander *expander) const override
    {
        QTC_ASSERT(kit, return);
        expander->registerVariable("Python:Name",
                                   Tr::tr("Name of Python Interpreter"),
                                   [kit]() -> QString {
                                       if (auto python = PythonKitAspect::python(kit))
                                           return python->name;
                                       return {};
                                   });
        expander->registerVariable("Python:Path",
                                   Tr::tr("Path to Python Interpreter"),
                                   [kit]() -> QString {
                                       if (auto python = PythonKitAspect::python(kit))
                                           return python->command.toUserOutput();
                                       return {};
                                   });
    }

    std::optional<Tasking::ExecutableItem> autoDetect(
        Kit *kit,
        const Utils::FilePaths &searchPaths,
        const QString &detectionSource,
        const LogCallback &logCallback) const override
    {
        Q_UNUSED(kit);

        using namespace Tasking;

        const auto setupSearch = [searchPaths, detectionSource](Async<Interpreter> &task) {
            const QList<Interpreter> alreadyConfigured = PythonSettings::interpreters();

            task.setConcurrentCallData(
                [](QPromise<Interpreter> &promise,
                   const FilePaths &searchPaths,
                   const QList<Interpreter> &alreadyConfigured,
                   const QString &detectionSource) {
                    for (const FilePath &path : searchPaths) {
                        const FilePath python = path.pathAppended("python3").withExecutableSuffix();
                        if (!python.isExecutableFile())
                            continue;
                        if (Utils::contains(
                                alreadyConfigured, Utils::equal(&Interpreter::command, python)))
                            continue;

                        Interpreter interpreter = PythonSettings::createInterpreter(
                            python, {}, "(" + python.toUserOutput() + ")", detectionSource);

                        promise.addResult(interpreter);
                    }
                },
                searchPaths,
                alreadyConfigured,
                detectionSource);
        };

        const auto searchDone = [detectionSource, logCallback](const Async<Interpreter> &task) {
            for (const auto &interpreter : task.results()) {
                PythonSettings::addInterpreter(interpreter, false);
                logCallback(
                    Tr::tr("Found \"%1\" (%2)")
                        .arg(interpreter.name, interpreter.command.toUserOutput()));
            }
        };

        return AsyncTask<Interpreter>(setupSearch, searchDone);
    }

    std::optional<Tasking::ExecutableItem> removeAutoDetected(
        const QString &detectionSource, const LogCallback &logCallback) const override
    {
        return Tasking::Sync([detectionSource, logCallback]() {
            PythonSettings::removeDetectedPython(detectionSource, logCallback);
        });
    }

    void listAutoDetected(
        const QString &detectionSource, const LogCallback &logCallback) const override
    {
        PythonSettings::listDetectedPython(detectionSource, logCallback);
    }
};

} // Internal

using namespace Internal;

std::optional<Interpreter> PythonKitAspect::python(const Kit *kit)
{
    QTC_ASSERT(kit, return {});
    const QString interpreterId = kit->value(id()).toString();
    if (interpreterId.isEmpty())
        return {};
    if (const Interpreter interpreter = PythonSettings::interpreter(interpreterId); interpreter.id == interpreterId)
        return interpreter;
    return {};
}

void PythonKitAspect::setPython(Kit *k, const QString &interpreterId)
{
    QTC_ASSERT(k, return);
    k->setValue(PythonKitAspect::id(), interpreterId);
}

Id PythonKitAspect::id()
{
    return Constants::PYTHON_TOOLKIT_ASPECT_ID;
}

const PythonKitAspectFactory thePythonToolKitAspectFactory;

} // namespace Python

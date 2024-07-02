// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonkitaspect.h"

#include "pythonconstants.h"
#include "pythonsettings.h"
#include "pythontr.h"
#include "pythonutils.h"

#include <projectexplorer/kitmanager.h>

#include <utils/guard.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcprocess.h>

#include <QComboBox>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python {

using namespace Internal;

class PythonKitAspectImpl final : public KitAspect
{
public:
    PythonKitAspectImpl(Kit *kit, const KitAspectFactory *kitInfo)
        : KitAspect(kit, kitInfo)
    {
        setManagingPage(Constants::C_PYTHONOPTIONS_PAGE_ID);

        m_comboBox = createSubWidget<QComboBox>();
        m_comboBox->setSizePolicy(QSizePolicy::Ignored, m_comboBox->sizePolicy().verticalPolicy());

        refresh();
        m_comboBox->setToolTip(kitInfo->description());
        connect(m_comboBox, &QComboBox::currentIndexChanged, this, [this] {
            if (m_ignoreChanges.isLocked())
                return;

            PythonKitAspect::setPython(m_kit, m_comboBox->currentData().toString());
        });
        connect(PythonSettings::instance(),
                &PythonSettings::interpretersChanged,
                this,
                &PythonKitAspectImpl::refresh);
    }

    void makeReadOnly() override
    {
        m_comboBox->setEnabled(false);
    }

    void refresh() override
    {
        const GuardLocker locker(m_ignoreChanges);
        m_comboBox->clear();
        m_comboBox->addItem(Tr::tr("None"), QString());

        for (const Interpreter &interpreter : PythonSettings::interpreters())
            m_comboBox->addItem(interpreter.name, interpreter.id);

        updateComboBox(PythonKitAspect::python(m_kit));
        emit changed(); // we need to emit changed here to update changes in the macro expander
    }

    void updateComboBox(const std::optional<Interpreter> &python)
    {
        const int index = python ? std::max(m_comboBox->findData(python->id), 0) : 0;
        m_comboBox->setCurrentIndex(index);
    }

protected:
    void addToInnerLayout(Layouting::Layout &parent) override
    {
        addMutableAction(m_comboBox);
        parent.addItem(m_comboBox);
    }

private:
    Guard m_ignoreChanges;
    QComboBox *m_comboBox = nullptr;
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
        if (path.needsDevice())
            return result;
        if (path.isEmpty()) {
            result << BuildSystemTask(Task::Error, Tr::tr("No Python setup."));
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
};

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

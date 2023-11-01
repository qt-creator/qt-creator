// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonkitaspect.h"

#include "pythonconstants.h"
#include "pythonsettings.h"
#include "pythontr.h"

#include <projectexplorer/kitmanager.h>

#include <utils/guard.h>
#include <utils/layoutbuilder.h>

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
    }

    void updateComboBox(const std::optional<Interpreter> &python)
    {
        const int index = python ? std::max(m_comboBox->findData(python->id), 0) : 0;
        m_comboBox->setCurrentIndex(index);
    }

protected:
    void addToLayoutImpl(Layouting::LayoutItem &parent) override
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
        const std::optional<Interpreter> python = PythonKitAspect::python(k);
        if (!python)
            return {};
        const FilePath path = python->command;
        if (path.needsDevice())
            return {};
        if (path.isEmpty())
            return {BuildSystemTask(Task::Error, Tr::tr("No Python setup."))};
        if (!path.exists()) {
            return {BuildSystemTask(Task::Error,
                                    Tr::tr("Python %1 not found.").arg(path.toUserOutput()))};
        }
        if (!path.isExecutableFile()) {
            return {BuildSystemTask{Task::Error,
                                    Tr::tr("Python %1 not executable.").arg(path.toUserOutput())}};
        }
        return {};
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
        if (k->isAspectRelevant(PythonKitAspect::id()) && PythonKitAspect::python(k))
            return {PythonKitAspect::id()};
        return {};
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

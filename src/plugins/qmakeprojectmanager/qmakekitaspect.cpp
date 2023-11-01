// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmakekitaspect.h"

#include "qmakeprojectmanagerconstants.h"
#include "qmakeprojectmanagertr.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/guard.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QLineEdit>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmakeProjectManager::Internal {

class QmakeKitAspectImpl final : public KitAspect
{
public:
    QmakeKitAspectImpl(Kit *k, const KitAspectFactory *ki)
        : KitAspect(k, ki), m_lineEdit(createSubWidget<QLineEdit>())
    {
        refresh(); // set up everything according to kit
        m_lineEdit->setToolTip(ki->description());
        connect(m_lineEdit, &QLineEdit::textEdited, this, &QmakeKitAspectImpl::mkspecWasChanged);
    }

    ~QmakeKitAspectImpl() override { delete m_lineEdit; }

private:
    void addToLayoutImpl(Layouting::LayoutItem &parent) override
    {
        addMutableAction(m_lineEdit);
        parent.addItem(m_lineEdit);
    }

    void makeReadOnly() override { m_lineEdit->setEnabled(false); }

    void refresh() override
    {
        if (!m_ignoreChanges.isLocked())
            m_lineEdit->setText(QDir::toNativeSeparators(QmakeKitAspect::mkspec(m_kit)));
    }

    void mkspecWasChanged(const QString &text)
    {
        const GuardLocker locker(m_ignoreChanges);
        QmakeKitAspect::setMkspec(m_kit, text, QmakeKitAspect::MkspecSource::User);
    }

    QLineEdit *m_lineEdit = nullptr;
    Guard m_ignoreChanges;
};

Id QmakeKitAspect::id()
{
    return Constants::KIT_INFORMATION_ID;
}

QString QmakeKitAspect::mkspec(const Kit *k)
{
    if (!k)
        return {};
    return k->value(QmakeKitAspect::id()).toString();
}

QString QmakeKitAspect::effectiveMkspec(const Kit *k)
{
    if (!k)
        return {};
    const QString spec = mkspec(k);
    if (spec.isEmpty())
        return defaultMkspec(k);
    return spec;
}

void QmakeKitAspect::setMkspec(Kit *k, const QString &mkspec, MkspecSource source)
{
    QTC_ASSERT(k, return);
    k->setValue(QmakeKitAspect::id(), source == MkspecSource::Code && mkspec == defaultMkspec(k)
                ? QString() : mkspec);
}

QString QmakeKitAspect::defaultMkspec(const Kit *k)
{
    QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
    if (!version) // No version, so no qmake
        return {};

    return version->mkspecFor(ToolChainKitAspect::cxxToolChain(k));
}

// QmakeKitAspectFactory

class QmakeKitAspectFactory : public KitAspectFactory
{
public:
    QmakeKitAspectFactory()
    {
        setId(QmakeKitAspect::id());
        setDisplayName(Tr::tr("Qt mkspec"));
        setDescription(Tr::tr("The mkspec to use when building the project with qmake.<br>"
                              "This setting is ignored when using other build systems."));
        setPriority(24000);
    }

    Tasks validate(const Kit *k) const override
    {
        Tasks result;
        QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(k);

        const QString mkspec = QmakeKitAspect::mkspec(k);
        if (!version && !mkspec.isEmpty())
            result << BuildSystemTask(Task::Warning, Tr::tr("No Qt version set, so mkspec is ignored."));
        if (version && !version->hasMkspec(mkspec))
            result << BuildSystemTask(Task::Error, Tr::tr("Mkspec not found for Qt version."));

        return result;
    }

    KitAspect *createKitAspect(Kit *k) const override
    {
        return new QmakeKitAspectImpl(k, this);
    }

    ItemList toUserOutput(const Kit *k) const override
    {
        return {{Tr::tr("mkspec"), QDir::toNativeSeparators(QmakeKitAspect::mkspec(k))}};
    }

    void addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const override
    {
        expander->registerVariable("Qmake:mkspec", Tr::tr("Mkspec configured for qmake by the kit."),
                                   [kit]() -> QString {
                                       return QDir::toNativeSeparators(QmakeKitAspect::mkspec(kit));
                                   });
    }
};

const QmakeKitAspectFactory theQmakeKitAspectFactory;

} // QmakeProjectManager::Internal

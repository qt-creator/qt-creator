/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qtkitinformation.h"

#include "qtsupportconstants.h"
#include "qtversionmanager.h"
#include "qtparser.h"
#include "qttestparser.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <utils/algorithm.h>
#include <utils/buildablehelperlibrary.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport {
namespace Internal {

class QtKitAspectWidget : public KitAspectWidget
{
    Q_DECLARE_TR_FUNCTIONS(QtSupport::QtKitAspectWidget)
public:
    QtKitAspectWidget(Kit *k, const KitAspect *ki) : KitAspectWidget(k, ki)
    {
        m_combo = new QComboBox;
        m_combo->setSizePolicy(QSizePolicy::Ignored, m_combo->sizePolicy().verticalPolicy());
        m_combo->addItem(tr("None"), -1);

        QList<int> versionIds = Utils::transform(QtVersionManager::versions(), &BaseQtVersion::uniqueId);
        versionsChanged(versionIds, QList<int>(), QList<int>());

        m_manageButton = new QPushButton(KitAspectWidget::msgManage());

        refresh();
        m_combo->setToolTip(ki->description());

        connect(m_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &QtKitAspectWidget::currentWasChanged);

        connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
                this, &QtKitAspectWidget::versionsChanged);

        connect(m_manageButton, &QAbstractButton::clicked, this, &QtKitAspectWidget::manageQtVersions);
    }

    ~QtKitAspectWidget() override
    {
        delete m_combo;
        delete m_manageButton;
    }

private:
    void makeReadOnly() override { m_combo->setEnabled(false); }
    QWidget *mainWidget() const override { return m_combo; }
    QWidget *buttonWidget() const override { return m_manageButton; }

    void refresh() override
    {
        m_combo->setCurrentIndex(findQtVersion(QtKitAspect::qtVersionId(m_kit)));
    }

private:
    static QString itemNameFor(const BaseQtVersion *v)
    {
        QTC_ASSERT(v, return QString());
        QString name = v->displayName();
        if (!v->isValid())
            name = QCoreApplication::translate("QtSupport::Internal::QtKitConfigWidget", "%1 (invalid)").arg(v->displayName());
        return name;
    }

    void versionsChanged(const QList<int> &added, const QList<int> &removed, const QList<int> &changed)
    {
        foreach (const int id, added) {
            BaseQtVersion *v = QtVersionManager::version(id);
            QTC_CHECK(v);
            QTC_CHECK(findQtVersion(id) < 0);
            m_combo->addItem(itemNameFor(v), id);
        }
        foreach (const int id, removed) {
            int pos = findQtVersion(id);
            if (pos >= 0) // We do not include invalid Qt versions, so do not try to remove those.
                m_combo->removeItem(pos);
        }
        foreach (const int id, changed) {
            BaseQtVersion *v = QtVersionManager::version(id);
            int pos = findQtVersion(id);
            QTC_CHECK(pos >= 0);
            m_combo->setItemText(pos, itemNameFor(v));
        }
    }

    void manageQtVersions()
    {
        Core::ICore::showOptionsDialog(Constants::QTVERSION_SETTINGS_PAGE_ID, buttonWidget());
    }

    void currentWasChanged(int idx)
    {
        QtKitAspect::setQtVersionId(m_kit, m_combo->itemData(idx).toInt());
    }

    int findQtVersion(const int id) const
    {
        for (int i = 0; i < m_combo->count(); ++i) {
            if (id == m_combo->itemData(i).toInt())
                return i;
        }
        return -1;
    }

    QComboBox *m_combo;
    QPushButton *m_manageButton;
};
} // namespace Internal

QtKitAspect::QtKitAspect()
{
    setObjectName(QLatin1String("QtKitAspect"));
    setId(QtKitAspect::id());
    setDisplayName(tr("Qt version"));
    setDescription(tr("The Qt library to use for all projects using this kit.<br>"
                      "A Qt version is required for qmake-based projects "
                      "and optional when using other build systems."));
    setPriority(26000);

    connect(KitManager::instance(), &KitManager::kitsLoaded,
            this, &QtKitAspect::kitsWereLoaded);
}

void QtKitAspect::setup(ProjectExplorer::Kit *k)
{
    if (!k || k->hasValue(id()))
        return;
    const Abi tcAbi = ToolChainKitAspect::targetAbi(k);
    const Utils::Id deviceType = DeviceTypeKitAspect::deviceTypeId(k);

    const QList<BaseQtVersion *> matches
            = QtVersionManager::versions([&tcAbi, &deviceType](const BaseQtVersion *qt) {
        return qt->targetDeviceTypes().contains(deviceType)
                && Utils::contains(qt->qtAbis(), [&tcAbi](const Abi &qtAbi) {
            return qtAbi.isCompatibleWith(tcAbi); });
    });
    if (matches.empty())
        return;

    // An MSVC 2015 toolchain is compatible with an MSVC 2017 Qt, but we prefer an
    // MSVC 2015 Qt if we find one.
    const QList<BaseQtVersion *> exactMatches = Utils::filtered(matches,
                                                                [&tcAbi](const BaseQtVersion *qt) {
        return qt->qtAbis().contains(tcAbi);
    });
    const QList<BaseQtVersion *> &candidates = !exactMatches.empty() ? exactMatches : matches;

    BaseQtVersion * const qtFromPath = QtVersionManager::version(
                equal(&BaseQtVersion::autodetectionSource, QString::fromLatin1("PATH")));
    if (qtFromPath && candidates.contains(qtFromPath))
        k->setValue(id(), qtFromPath->uniqueId());
    else
        k->setValue(id(), candidates.first()->uniqueId());
}

Tasks QtKitAspect::validate(const ProjectExplorer::Kit *k) const
{
    QTC_ASSERT(QtVersionManager::isLoaded(), return { });
    BaseQtVersion *version = qtVersion(k);
    if (!version)
    return { };

    return version->validateKit(k);
}

void QtKitAspect::fix(ProjectExplorer::Kit *k)
{
    QTC_ASSERT(QtVersionManager::isLoaded(), return);
    BaseQtVersion *version = qtVersion(k);
    if (!version) {
        if (qtVersionId(k) >= 0) {
            qWarning("Qt version is no longer known, removing from kit \"%s\".",
                     qPrintable(k->displayName()));
            setQtVersionId(k, -1);
        }
        return;
    }

    // Set a matching toolchain if we don't have one.
    if (ToolChainKitAspect::cxxToolChain(k))
        return;

    const QString spec = version->mkspec();
    QList<ToolChain *> possibleTcs = ToolChainManager::toolChains(
                [version](const ToolChain *t) {
        return t->isValid()
                && t->language() == Utils::Id(ProjectExplorer::Constants::CXX_LANGUAGE_ID)
                && contains(version->qtAbis(), [t](const Abi &qtAbi) {
                       return qtAbi.isFullyCompatibleWith(t->targetAbi());
                   });
    });
    if (!possibleTcs.isEmpty()) {
        // Prefer exact matches.
        // TODO: We should probably prefer the compiler with the highest version number instead,
        //       but this information is currently not exposed by the ToolChain class.
        sort(possibleTcs, [version](const ToolChain *tc1, const ToolChain *tc2) {
            const QVector<Abi> &qtAbis = version->qtAbis();
            const bool tc1ExactMatch = qtAbis.contains(tc1->targetAbi());
            const bool tc2ExactMatch = qtAbis.contains(tc2->targetAbi());
            return tc1ExactMatch && !tc2ExactMatch;
        });

        const QList<ToolChain *> goodTcs = Utils::filtered(possibleTcs,
                                                           [&spec](const ToolChain *t) {
            return t->suggestedMkspecList().contains(spec);
        });
        // Hack to prefer a tool chain from PATH (e.g. autodetected) over other matches.
        // This improves the situation a bit if a cross-compilation tool chain has the
        // same ABI as the host.
        const Environment systemEnvironment = Environment::systemEnvironment();
        ToolChain *bestTc = Utils::findOrDefault(goodTcs,
                                                 [&systemEnvironment](const ToolChain *t) {
            return systemEnvironment.path().contains(t->compilerCommand().parentDir());
        });
        if (!bestTc) {
            bestTc = goodTcs.isEmpty() ? possibleTcs.last() : goodTcs.last();
        }
        if (bestTc)
            ToolChainKitAspect::setAllToolChainsToMatch(k, bestTc);
    }
}

ProjectExplorer::KitAspectWidget *QtKitAspect::createConfigWidget(ProjectExplorer::Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new Internal::QtKitAspectWidget(k, this);
}

QString QtKitAspect::displayNamePostfix(const ProjectExplorer::Kit *k) const
{
    BaseQtVersion *version = qtVersion(k);
    return version ? version->displayName() : QString();
}

ProjectExplorer::KitAspect::ItemList
QtKitAspect::toUserOutput(const ProjectExplorer::Kit *k) const
{
    BaseQtVersion *version = qtVersion(k);
    return {{tr("Qt version"), version ? version->displayName() : tr("None")}};
}

void QtKitAspect::addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const
{
    BaseQtVersion *version = qtVersion(k);
    if (version)
        version->addToEnvironment(k, env);
}

QList<Utils::OutputLineParser *> QtKitAspect::createOutputParsers(const Kit *k) const
{
    if (qtVersion(k))
        return {new Internal::QtTestParser, new QtParser};
    return {};
}

class QtMacroSubProvider
{
public:
    QtMacroSubProvider(Kit *kit)
        : expander(BaseQtVersion::createMacroExpander(
              [kit] { return QtKitAspect::qtVersion(kit); }))
    {}

    MacroExpander *operator()() const
    {
        return expander.get();
    }

    std::shared_ptr<MacroExpander> expander;
};

void QtKitAspect::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    QTC_ASSERT(kit, return);
    expander->registerSubProvider(QtMacroSubProvider(kit));

    expander->registerVariable("Qt:Name", tr("Name of Qt Version"),
                [kit]() -> QString {
                   BaseQtVersion *version = qtVersion(kit);
                   return version ? version->displayName() : tr("unknown");
                });
    expander->registerVariable("Qt:qmakeExecutable", tr("Path to the qmake executable"),
                [kit]() -> QString {
                    BaseQtVersion *version = qtVersion(kit);
                    return version ? version->qmakeCommand().toString() : QString();
                });
}

Utils::Id QtKitAspect::id()
{
    return "QtSupport.QtInformation";
}

int QtKitAspect::qtVersionId(const ProjectExplorer::Kit *k)
{
    if (!k)
        return -1;

    int id = -1;
    QVariant data = k->value(QtKitAspect::id(), -1);
    if (data.type() == QVariant::Int) {
        bool ok;
        id = data.toInt(&ok);
        if (!ok)
            id = -1;
    } else {
        QString source = data.toString();
        BaseQtVersion *v = QtVersionManager::version([source](const BaseQtVersion *v) { return v->autodetectionSource() == source; });
        if (v)
            id = v->uniqueId();
    }
    return id;
}

void QtKitAspect::setQtVersionId(ProjectExplorer::Kit *k, const int id)
{
    QTC_ASSERT(k, return);
    k->setValue(QtKitAspect::id(), id);
}

BaseQtVersion *QtKitAspect::qtVersion(const ProjectExplorer::Kit *k)
{
    return QtVersionManager::version(qtVersionId(k));
}

void QtKitAspect::setQtVersion(ProjectExplorer::Kit *k, const BaseQtVersion *v)
{
    if (!v)
        setQtVersionId(k, -1);
    else
        setQtVersionId(k, v->uniqueId());
}

void QtKitAspect::qtVersionsChanged(const QList<int> &addedIds,
                                         const QList<int> &removedIds,
                                         const QList<int> &changedIds)
{
    Q_UNUSED(addedIds)
    Q_UNUSED(removedIds)
    foreach (ProjectExplorer::Kit *k, ProjectExplorer::KitManager::kits()) {
        if (changedIds.contains(qtVersionId(k))) {
            k->validate(); // Qt version may have become (in)valid
            notifyAboutUpdate(k);
        }
    }
}

void QtKitAspect::kitsWereLoaded()
{
    foreach (ProjectExplorer::Kit *k, ProjectExplorer::KitManager::kits())
        fix(k);

    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &QtKitAspect::qtVersionsChanged);
}

Kit::Predicate QtKitAspect::platformPredicate(Utils::Id platform)
{
    return [platform](const Kit *kit) -> bool {
        BaseQtVersion *version = QtKitAspect::qtVersion(kit);
        return version && version->targetDeviceTypes().contains(platform);
    };
}

Kit::Predicate QtKitAspect::qtVersionPredicate(const QSet<Utils::Id> &required,
                                                    const QtVersionNumber &min,
                                                    const QtVersionNumber &max)
{
    return [required, min, max](const Kit *kit) -> bool {
        BaseQtVersion *version = QtKitAspect::qtVersion(kit);
        if (!version)
            return false;
        QtVersionNumber current = version->qtVersion();
        if (min.majorVersion > -1 && current < min)
            return false;
        if (max.majorVersion > -1 && current > max)
            return false;
        return version->features().contains(required);
    };
}

QSet<Utils::Id> QtKitAspect::supportedPlatforms(const Kit *k) const
{
    BaseQtVersion *version = QtKitAspect::qtVersion(k);
    return version ? version->targetDeviceTypes() : QSet<Utils::Id>();
}

QSet<Utils::Id> QtKitAspect::availableFeatures(const Kit *k) const
{
    BaseQtVersion *version = QtKitAspect::qtVersion(k);
    return version ? version->features() : QSet<Utils::Id>();
}

int QtKitAspect::weight(const Kit *k) const
{
    const BaseQtVersion * const qt = qtVersion(k);
    if (!qt)
        return 0;
    if (!qt->targetDeviceTypes().contains(DeviceTypeKitAspect::deviceTypeId(k)))
        return 0;
    const Abi tcAbi = ToolChainKitAspect::targetAbi(k);
    if (qt->qtAbis().contains(tcAbi))
        return 2;
    return Utils::contains(qt->qtAbis(), [&tcAbi](const Abi &qtAbi) {
        return qtAbi.isCompatibleWith(tcAbi); }) ? 1 : 0;
}

} // namespace QtSupport

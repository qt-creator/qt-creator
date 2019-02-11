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

#include "qmakekitinformation.h"

#include "qmakeprojectmanagerconstants.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QLineEdit>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

class QmakeKitAspectWidget : public KitAspectWidget
{
    Q_DECLARE_TR_FUNCTIONS(QmakeProjectManager::Internal::QmakeKitAspect)

public:
    QmakeKitAspectWidget(Kit *k, const KitAspect *ki)
        : KitAspectWidget(k, ki), m_lineEdit(new QLineEdit)
    {
        refresh(); // set up everything according to kit
        m_lineEdit->setToolTip(ki->description());
        connect(m_lineEdit, &QLineEdit::textEdited, this, &QmakeKitAspectWidget::mkspecWasChanged);
    }

    ~QmakeKitAspectWidget() override { delete m_lineEdit; }

private:
    QWidget *mainWidget() const override { return m_lineEdit; }
    void makeReadOnly() override { m_lineEdit->setEnabled(false); }

    void refresh() override
    {
        if (!m_ignoreChange)
            m_lineEdit->setText(QmakeKitAspect::mkspec(m_kit).toUserOutput());
    }

    void mkspecWasChanged(const QString &text)
    {
        m_ignoreChange = true;
        QmakeKitAspect::setMkspec(m_kit, Utils::FileName::fromString(text));
        m_ignoreChange = false;
    }

    QLineEdit *m_lineEdit = nullptr;
    bool m_ignoreChange = false;
};


QmakeKitAspect::QmakeKitAspect()
{
    setObjectName(QLatin1String("QmakeKitAspect"));
    setId(QmakeKitAspect::id());
    setDisplayName(tr("Qt mkspec"));
    setDescription(tr("The mkspec to use when building the project with qmake.<br>"
                      "This setting is ignored when using other build systems."));
    setPriority(24000);
}

QVariant QmakeKitAspect::defaultValue(const Kit *k) const
{
    Q_UNUSED(k);
    return QString();
}

QList<Task> QmakeKitAspect::validate(const Kit *k) const
{
    QList<Task> result;
    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(k);

    FileName mkspec = QmakeKitAspect::mkspec(k);
    if (!version && !mkspec.isEmpty())
        result << Task(Task::Warning, tr("No Qt version set, so mkspec is ignored."),
                       FileName(), -1, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
    if (version && !version->hasMkspec(mkspec))
        result << Task(Task::Error, tr("Mkspec not found for Qt version."),
                       FileName(), -1, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
    return result;
}

void QmakeKitAspect::setup(Kit *k)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
    if (!version)
        return;

    // HACK: Ignore Boot2Qt kits!
    if (version->type() == "Boot2Qt.QtVersionType" || version->type() == "Qdb.EmbeddedLinuxQt")
        return;

    FileName spec = QmakeKitAspect::mkspec(k);
    if (spec.isEmpty())
        spec = version->mkspec();

    ToolChain *tc = ToolChainKitAspect::toolChain(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    if (!tc || (!tc->suggestedMkspecList().empty() && !tc->suggestedMkspecList().contains(spec))) {
        const QList<ToolChain *> possibleTcs = ToolChainManager::toolChains(
                    [version](const ToolChain *t) {
            return t->isValid()
                && t->language() == Core::Id(ProjectExplorer::Constants::CXX_LANGUAGE_ID)
                && version->qtAbis().contains(t->targetAbi());
        });
        if (!possibleTcs.isEmpty()) {
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
}

KitAspectWidget *QmakeKitAspect::createConfigWidget(Kit *k) const
{
    return new Internal::QmakeKitAspectWidget(k, this);
}

KitAspect::ItemList QmakeKitAspect::toUserOutput(const Kit *k) const
{
    return ItemList() << qMakePair(tr("mkspec"), mkspec(k).toUserOutput());
}

void QmakeKitAspect::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    expander->registerVariable("Qmake:mkspec", tr("Mkspec configured for qmake by the Kit."),
                [kit]() -> QString {
                    return QmakeKitAspect::mkspec(kit).toUserOutput();
                });
}

Core::Id QmakeKitAspect::id()
{
    return Constants::KIT_INFORMATION_ID;
}

FileName QmakeKitAspect::mkspec(const Kit *k)
{
    if (!k)
        return FileName();
    return FileName::fromString(k->value(QmakeKitAspect::id()).toString());
}

FileName QmakeKitAspect::effectiveMkspec(const Kit *k)
{
    if (!k)
        return FileName();
    FileName spec = mkspec(k);
    if (spec.isEmpty())
        return defaultMkspec(k);
    return spec;
}

void QmakeKitAspect::setMkspec(Kit *k, const FileName &fn)
{
    QTC_ASSERT(k, return);
    k->setValue(QmakeKitAspect::id(), fn == defaultMkspec(k) ? QString() : fn.toString());
}

FileName QmakeKitAspect::defaultMkspec(const Kit *k)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
    if (!version) // No version, so no qmake
        return FileName();

    return version->mkspecFor(ToolChainKitAspect::toolChain(k,
                        ProjectExplorer::Constants::CXX_LANGUAGE_ID));
}

} // namespace Internal
} // namespace QmakeProjectManager

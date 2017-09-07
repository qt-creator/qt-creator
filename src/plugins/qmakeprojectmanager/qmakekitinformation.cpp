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

#include "qmakekitconfigwidget.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmakeProjectManager {

QmakeKitInformation::QmakeKitInformation()
{
    setObjectName(QLatin1String("QmakeKitInformation"));
    setId(QmakeKitInformation::id());
    setPriority(24000);
}

QVariant QmakeKitInformation::defaultValue(const Kit *k) const
{
    Q_UNUSED(k);
    return QString();
}

QList<Task> QmakeKitInformation::validate(const Kit *k) const
{
    QList<Task> result;
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);

    FileName mkspec = QmakeKitInformation::mkspec(k);
    if (!version && !mkspec.isEmpty())
        result << Task(Task::Warning, tr("No Qt version set, so mkspec is ignored."),
                       FileName(), -1, Constants::TASK_CATEGORY_BUILDSYSTEM);
    if (version && !version->hasMkspec(mkspec))
        result << Task(Task::Error, tr("Mkspec not found for Qt version."),
                       FileName(), -1, Constants::TASK_CATEGORY_BUILDSYSTEM);
    return result;
}

void QmakeKitInformation::setup(Kit *k)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
    if (!version)
        return;

    // HACK: Ignore Boot2Qt kits!
    if (version->type() == "Boot2Qt.QtVersionType" || version->type() == "Qdb.EmbeddedLinuxQt")
        return;

    FileName spec = QmakeKitInformation::mkspec(k);
    if (spec.isEmpty())
        spec = version->mkspec();

    ToolChain *tc = ToolChainKitInformation::toolChain(k, Constants::CXX_LANGUAGE_ID);

    if (!tc || (!tc->suggestedMkspecList().empty() && !tc->suggestedMkspecList().contains(spec))) {
        const QList<ToolChain *> possibleTcs = ToolChainManager::toolChains(
                    [version](const ToolChain *t) {
            return t->isValid()
                && t->language() == Core::Id(Constants::CXX_LANGUAGE_ID)
                && version->qtAbis().contains(t->targetAbi());
        });
        if (!possibleTcs.isEmpty()) {
            ToolChain *possibleTc
                    = Utils::findOr(possibleTcs, possibleTcs.last(),
                                    [&spec](const ToolChain *t) { return t->suggestedMkspecList().contains(spec); });
            if (possibleTc)
                ToolChainKitInformation::setAllToolChainsToMatch(k, possibleTc);
        }
    }
}

KitConfigWidget *QmakeKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::QmakeKitConfigWidget(k, this);
}

KitInformation::ItemList QmakeKitInformation::toUserOutput(const Kit *k) const
{
    return ItemList() << qMakePair(tr("mkspec"), mkspec(k).toUserOutput());
}

void QmakeKitInformation::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    expander->registerVariable("Qmake:mkspec", tr("Mkspec configured for qmake by the Kit."),
                [kit]() -> QString {
                    return QmakeKitInformation::mkspec(kit).toUserOutput();
                });
}

Core::Id QmakeKitInformation::id()
{
    return "QtPM4.mkSpecInformation";
}

FileName QmakeKitInformation::mkspec(const Kit *k)
{
    if (!k)
        return FileName();
    return FileName::fromString(k->value(QmakeKitInformation::id()).toString());
}

FileName QmakeKitInformation::effectiveMkspec(const Kit *k)
{
    if (!k)
        return FileName();
    FileName spec = mkspec(k);
    if (spec.isEmpty())
        return defaultMkspec(k);
    return spec;
}

void QmakeKitInformation::setMkspec(Kit *k, const FileName &fn)
{
    k->setValue(QmakeKitInformation::id(), fn == defaultMkspec(k) ? QString() : fn.toString());
}

FileName QmakeKitInformation::defaultMkspec(const Kit *k)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
    if (!version) // No version, so no qmake
        return FileName();

    return version->mkspecFor(ToolChainKitInformation::toolChain(k, Constants::CXX_LANGUAGE_ID));
}

} // namespace QmakeProjectManager

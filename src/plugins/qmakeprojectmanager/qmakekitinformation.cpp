/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmakekitinformation.h"

#include "qmakekitconfigwidget.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtkitinformation.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmakeProjectManager {

QmakeKitInformation::QmakeKitInformation()
{
    setObjectName(QLatin1String("QmakeKitInformation"));
    setId(QmakeKitInformation::id());
    setPriority(24000);
}

QVariant QmakeKitInformation::defaultValue(Kit *k) const
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

    FileName spec = QmakeKitInformation::mkspec(k);
    if (spec.isEmpty())
        spec = version->mkspec();

    ToolChain *tc = ToolChainKitInformation::toolChain(k);

    if (!tc || (!tc->suggestedMkspecList().empty() && !tc->suggestedMkspecList().contains(spec))) {
        ToolChain *possibleTc = 0;
        foreach (ToolChain *current, ToolChainManager::toolChains()) {
            if (version->qtAbis().contains(current->targetAbi())) {
                possibleTc = current;
                if (current->suggestedMkspecList().contains(spec))
                    break;
            }
        }
        ToolChainKitInformation::setToolChain(k, possibleTc);
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

    return version->mkspecFor(ToolChainKitInformation::toolChain(k));
}

} // namespace QmakeProjectManager

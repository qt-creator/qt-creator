/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#pragma once

#include "mcusupport_global.h"

#include <QHash>
#include <QPair>

#include <memory>

namespace Utils {
class FilePath;
} // namespace Utils

namespace McuSupport::Internal {

namespace Sdk {
struct McuTargetDescription;
} //namespace Sdk

class McuAbstractTargetFactory
{
public:
    using Ptr = std::unique_ptr<McuAbstractTargetFactory>;
    virtual ~McuAbstractTargetFactory() = default;

    virtual QPair<Targets, Packages> createTargets(const Sdk::McuTargetDescription &,
                                                   const Utils::FilePath &qtForMcuPath)
        = 0;
    using AdditionalPackages
        = QPair<QHash<QString, McuToolChainPackagePtr>, QHash<QString, McuPackagePtr>>;
    virtual AdditionalPackages getAdditionalPackages() const { return {}; }
}; // struct McuAbstractTargetFactory
} // namespace McuSupport::Internal

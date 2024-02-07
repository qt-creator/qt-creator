// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

namespace VcsBase {

class VcsBaseSubmitEditorParameters;
class VersionControlBase;

VCSBASE_EXPORT void setupVcsSubmitEditor(
    VersionControlBase *versionControl,
    const VcsBaseSubmitEditorParameters &parameters);

} // namespace VcsBase

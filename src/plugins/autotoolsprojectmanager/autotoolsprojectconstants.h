// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace AutotoolsProjectManager {
/**
 * Collects project constants, that are shared between several classes.
 */
namespace Constants {

const char MAKEFILE_MIMETYPE[] = "text/x-makefile";

// Steps
const char AUTOGEN_STEP_ID[] = "AutotoolsProjectManager.AutogenStep";
const char AUTORECONF_STEP_ID[] = "AutotoolsProjectManager.AutoreconfStep";
const char CONFIGURE_STEP_ID[] = "AutotoolsProjectManager.ConfigureStep";
const char MAKE_STEP_ID[] = "AutotoolsProjectManager.MakeStep";

//Project
const char AUTOTOOLS_PROJECT_ID[] = "AutotoolsProjectManager.AutotoolsProject";

} // namespace Constants
} // namespace AutotoolsProjectManager

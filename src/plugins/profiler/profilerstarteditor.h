// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core { class IEditor; }

namespace Profiler::Internal {

// Opens the page that starts a profiling run, or raises it when it is already
// open. It is a document like the traces are, so it sits next to them in the
// editor area rather than replacing whatever is open there.
Core::IEditor *openProfilerStartPage();

// Opens or raises it. Registered in the Analyze menu, and shown in the Profile
// mode's toolbar, which is the only way back to the page from inside the mode.
QAction *profilerStartPageAction();

// Whether that page is open, and whether any trace is.
bool isProfilerStartPageOpen();
bool hasOpenTrace();

void setupProfilerStartEditor();
void destroyProfilerStartEditor();

} // namespace Profiler::Internal

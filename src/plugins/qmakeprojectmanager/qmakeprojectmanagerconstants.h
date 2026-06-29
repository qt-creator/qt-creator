// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace QmakeProjectManager::Constants {

// Menus
inline constexpr char M_CONTEXT[] = "ProFileEditor.ContextMenu";

// Kinds
inline constexpr char PROFILE_EDITOR_ID[] = "Qt4.proFileEditor";
// Actions
inline constexpr char RUNQMAKE[] = "Qt4Builder.RunQMake";
inline constexpr char RUNQMAKECONTEXTMENU[] = "Qt4Builder.RunQMakeContextMenu";
inline constexpr char BUILDSUBDIR[] = "Qt4Builder.BuildSubDir";
inline constexpr char REBUILDSUBDIR[] = "Qt4Builder.RebuildSubDir";
inline constexpr char CLEANSUBDIR[] = "Qt4Builder.CleanSubDir";
inline constexpr char ADDLIBRARY[] = "Qt4.AddLibrary";

// Tasks
inline constexpr char PROFILE_EVALUATE[] = "Qt4ProjectManager.ProFileEvaluate";

// Project
inline constexpr char QMAKEPROJECT_ID[] = "Qt4ProjectManager.Qt4Project";

inline constexpr char QMAKE_BC_ID[] = "Qt4ProjectManager.Qt4BuildConfiguration";
inline constexpr char MAKESTEP_BS_ID[] = "Qt4ProjectManager.MakeStep";
inline constexpr char QMAKE_BS_ID[] = "QtProjectManager.QMakeBuildStep";

// Kit
inline constexpr char KIT_INFORMATION_ID[] = "QtPM4.mkSpecInformation";

} // namespace QmakeProjectManager::Constants

// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QmlDesigner {

inline constexpr QStringView priorityImports[] = {u"QtQuick.Controls",
                                                  u"QtQuick.Layouts",
                                                  u"QtQuick.Studio.Effects",
                                                  u"QtQuick.Studio.Components",
                                                  u"QtQuick.Studio.MultiText",
                                                  u"QtQuick.Studio.LogicHelper",
                                                  u"Qt.SafeRenderer",
                                                  u"QtQuick3D",
                                                  u"FlowView"}; // have to be sorted

} // namespace QmlDesigner

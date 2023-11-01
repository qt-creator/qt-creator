// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imode.h"

QT_BEGIN_NAMESPACE
class QSplitter;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Core {

class EditorManager;

namespace Internal {

class EditMode : public IMode
{
    Q_OBJECT

public:
    EditMode();
    ~EditMode() override;

private:
    void grabEditorManager(Utils::Id mode);

    QSplitter *m_splitter;
    QVBoxLayout *m_rightSplitWidgetLayout;
};

} // namespace Internal
} // namespace Core

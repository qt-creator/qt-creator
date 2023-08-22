// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

#include <QDialog>

namespace Utils { class BaseTreeModel; }

namespace ProjectExplorer {
class Kit;
namespace Internal {

class FilterKitAspectsDialog : public QDialog
{
public:
    FilterKitAspectsDialog(const Kit *kit, QWidget *parent);
    QSet<Utils::Id> irrelevantAspects() const;

private:
    Utils::BaseTreeModel * const m_model;
};

} // namespace Internal
} // namespace ProjectExplorer

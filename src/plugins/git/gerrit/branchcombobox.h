// Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QComboBox>

namespace Git {
namespace Internal { class GitClient; }
}

namespace Gerrit {
namespace Internal {

class BranchComboBox : public QComboBox
{
public:
    explicit BranchComboBox(QWidget *parent = nullptr);
    void init(const Utils::FilePath &repository);

private:
    Utils::FilePath m_repository;
    bool m_detached = false;
};

} // namespace Internal
} // namespace Gerrit

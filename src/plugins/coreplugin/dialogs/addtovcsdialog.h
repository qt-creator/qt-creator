// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QDialog>

namespace Core {
namespace Internal {

class AddToVcsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddToVcsDialog(QWidget *parent, const QString &title,
                            const Utils::FilePaths &files, const QString &vcsDisplayName);
    ~AddToVcsDialog() override;
};


} // namespace Internal
} // namespace Core

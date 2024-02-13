// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

namespace Fossil {
namespace Internal {

struct RepositorySettings;
class ConfigureDialogPrivate;

class ConfigureDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigureDialog(QWidget *parent = nullptr);
    ~ConfigureDialog() final;

    const RepositorySettings settings() const;
    void setSettings(const RepositorySettings &settings);

private:
    ConfigureDialogPrivate *d = nullptr;
};

} // namespace Internal
} // namespace Fossil

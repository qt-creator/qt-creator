// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/pathchooser.h>
#include <vcsbase/vcsbaseplugin.h>
#include <QDialog>

namespace Mercurial {
namespace Internal {

namespace Ui { class SrcDestDialog; }

class SrcDestDialog : public QDialog
{
    Q_OBJECT

public:
    enum Direction { outgoing, incoming };
    explicit SrcDestDialog(const VcsBase::VcsBasePluginState &state, Direction dir, QWidget *parent = nullptr);
    ~SrcDestDialog() override;

    void setPathChooserKind(Utils::PathChooser::Kind kind);
    QString getRepositoryString() const;
    Utils::FilePath workingDir() const;

private:
    QUrl getRepoUrl() const;

private:
    Ui::SrcDestDialog *m_ui;
    Direction m_direction;
    mutable QString m_workingdir;
    VcsBase::VcsBasePluginState m_state;
};

} // namespace Internal
} // namespace Mercurial

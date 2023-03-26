// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/pathchooser.h>

#include <vcsbase/vcsbaseplugin.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QRadioButton;
QT_END_NAMESPACE

namespace Mercurial::Internal {

class SrcDestDialog : public QDialog
{
public:
    enum Direction { outgoing, incoming };

    explicit SrcDestDialog(const VcsBase::VcsBasePluginState &state, Direction dir, QWidget *parent = nullptr);
    ~SrcDestDialog() override;

    void setPathChooserKind(Utils::PathChooser::Kind kind);
    QString getRepositoryString() const;
    Utils::FilePath workingDir() const;

private:
    QUrl getRepoUrl() const;

    Direction m_direction;
    mutable QString m_workingdir;
    VcsBase::VcsBasePluginState m_state;

    QRadioButton *m_defaultButton;
    QRadioButton *m_localButton;
    Utils::PathChooser *m_localPathChooser;
    QLineEdit *m_urlLineEdit;
    QCheckBox *m_promptForCredentials;
};

} // Mercurial::Internal

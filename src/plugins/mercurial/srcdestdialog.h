/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <utils/pathchooser.h>
#include <QDialog>

namespace Mercurial {
namespace Internal {

namespace Ui { class SrcDestDialog; }

class SrcDestDialog : public QDialog
{
    Q_OBJECT

public:
    enum Direction { outgoing, incoming };
    explicit SrcDestDialog(Direction dir, QWidget *parent = 0);
    ~SrcDestDialog() override;

    void setPathChooserKind(Utils::PathChooser::Kind kind);
    QString getRepositoryString() const;
    QString workingDir() const;

private:
    QUrl getRepoUrl() const;

private:
    Ui::SrcDestDialog *m_ui;
    Direction m_direction;
    mutable QString m_workingdir;
};

} // namespace Internal
} // namespace Mercurial

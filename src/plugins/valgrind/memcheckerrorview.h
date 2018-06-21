/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#include <debugger/analyzer/detailederrorview.h>

#include <QListView>

namespace Valgrind {
namespace Internal {

class ValgrindBaseSettings;

class MemcheckErrorView : public Debugger::DetailedErrorView
{
    Q_OBJECT

public:
    MemcheckErrorView(QWidget *parent = 0);
    ~MemcheckErrorView();

    void setDefaultSuppressionFile(const QString &suppFile);
    QString defaultSuppressionFile() const;
    ValgrindBaseSettings *settings() const { return m_settings; }
    void settingsChanged(ValgrindBaseSettings *settings);

private:
    void suppressError();
    QList<QAction *> customActions() const override;

    QAction *m_suppressAction;
    QString m_defaultSuppFile;
    ValgrindBaseSettings *m_settings;
};

} // namespace Internal
} // namespace Valgrind

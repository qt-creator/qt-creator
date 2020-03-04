/****************************************************************************
**
** Copyright (C) 2020 Miklos Marton <martonmiklosqdev@gmail.com>
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

#include <utils/outputformatter.h>

#include <QRegularExpression>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace VcsBase {

class VcsOutputFormatter : public Utils::OutputFormatter
{
    Q_OBJECT
public:
    VcsOutputFormatter();
    ~VcsOutputFormatter() override = default;
    void appendMessage(const QString &text, Utils::OutputFormat format) override;
    void handleLink(const QString &href) override;
    void fillLinkContextMenu(QMenu *menu, const QString &workingDirectory, const QString &href);

signals:
    void referenceClicked(const QString &reference);

private:
    const QRegularExpression m_regexp;
};

}

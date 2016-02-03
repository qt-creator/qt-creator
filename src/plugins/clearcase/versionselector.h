/****************************************************************************
**
** Copyright (C) 2016 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

#include <QDialog>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace ClearCase {
namespace Internal {

namespace Ui { class VersionSelector; }

class VersionSelector : public QDialog
{
    Q_OBJECT

public:
    explicit VersionSelector(const QString &fileName, const QString &message,
                             QWidget *parent = nullptr);
    ~VersionSelector() override;
    bool isUpdate() const;

private:
    bool readValues();

    Ui::VersionSelector *ui;
    QTextStream *m_stream;
    QString m_versionID, m_createdBy, m_createdOn, m_message;
};

} // namespace Internal
} // namespace ClearCase

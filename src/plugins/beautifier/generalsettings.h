/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
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

#include <utils/mimetypes/mimetype.h>

#include <QList>

namespace Beautifier {
namespace Internal {

class GeneralSettings
{
public:
    explicit GeneralSettings();
    void read();
    void save();

    bool autoFormatOnSave() const;
    void setAutoFormatOnSave(bool autoFormatOnSave);

    QString autoFormatTool() const;
    void setAutoFormatTool(const QString &autoFormatTool);

    QList<Utils::MimeType> autoFormatMime() const;
    QString autoFormatMimeAsString() const;
    void setAutoFormatMime(const QList<Utils::MimeType> &autoFormatMime);
    void setAutoFormatMime(const QString &mimeList);

    bool autoFormatOnlyCurrentProject() const;
    void setAutoFormatOnlyCurrentProject(bool autoFormatOnlyCurrentProject);

private:
    bool m_autoFormatOnSave = false;
    bool m_autoFormatOnlyCurrentProject = true;
    QString m_autoFormatTool;
    QList<Utils::MimeType> m_autoFormatMime;
};

} // namespace Internal
} // namespace Beautifier

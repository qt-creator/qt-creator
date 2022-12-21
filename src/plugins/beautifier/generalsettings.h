// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/mimeutils.h>

#include <QList>

namespace Beautifier {
namespace Internal {

class GeneralSettings
{
public:
    explicit GeneralSettings();
    static GeneralSettings *instance();

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

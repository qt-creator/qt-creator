// Copyright (C) 2016 Lorenz Haas
// Copyright (C) 2022 Xavier BESSON
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/mimeutils.h>

#include <QList>
#include <QVersionNumber>

namespace Core { class IDocument; }

namespace CMakeProjectManager {
namespace Internal {

class VersionUpdater;

class CMakeFormatterSettings : public QObject
{
    Q_OBJECT
public:
    explicit CMakeFormatterSettings(QObject* parent = nullptr);
    static CMakeFormatterSettings *instance();

    void read();
    void save();

    Utils::FilePath command() const;
    void setCommand(const QString &cmd);

    bool autoFormatOnSave() const;
    void setAutoFormatOnSave(bool autoFormatOnSave);

    QList<Utils::MimeType> autoFormatMime() const;
    QString autoFormatMimeAsString() const;
    void setAutoFormatMime(const QList<Utils::MimeType> &autoFormatMime);
    void setAutoFormatMime(const QString &mimeList);

    bool autoFormatOnlyCurrentProject() const;
    void setAutoFormatOnlyCurrentProject(bool autoFormatOnlyCurrentProject);

    bool isApplicable(const Core::IDocument *document) const;

signals:
    void supportedMimeTypesChanged();

private:
    QString m_command;

    bool m_autoFormatOnSave = false;
    bool m_autoFormatOnlyCurrentProject = true;
    QString m_autoFormatTool;
    QList<Utils::MimeType> m_autoFormatMime;
};

} // namespace Internal
} // namespace CMakeProjectManager

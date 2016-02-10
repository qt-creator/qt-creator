/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <coreplugin/dialogs/ioptionspage.h>
#include "ui_fileshareprotocolsettingswidget.h"

#include <QSharedPointer>
#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CodePaster {

class FileShareProtocolSettings {
public:
    FileShareProtocolSettings();
    void toSettings(QSettings *) const;
    void fromSettings(const QSettings *);
    bool equals(const FileShareProtocolSettings &rhs) const;

    QString path;
    int displayCount;
};

inline bool operator==(const FileShareProtocolSettings &s1, const FileShareProtocolSettings &s2)
{ return s1.equals(s2); }
inline bool operator!=(const FileShareProtocolSettings &s1, const FileShareProtocolSettings &s2)
{ return !s1.equals(s2); }

class FileShareProtocolSettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit FileShareProtocolSettingsWidget(QWidget *parent = 0);

    void setSettings(const FileShareProtocolSettings &);
    FileShareProtocolSettings settings() const;

private:
    Internal::Ui::FileShareProtocolSettingsWidget m_ui;
};

class FileShareProtocolSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit FileShareProtocolSettingsPage(const QSharedPointer<FileShareProtocolSettings> &s,
                                           QObject *parent = 0);

    QWidget *widget() override;
    void apply() override;
    void finish() override { }

private:
    const QSharedPointer<FileShareProtocolSettings> m_settings;
    QPointer<FileShareProtocolSettingsWidget> m_widget;
};
} // namespace CodePaster

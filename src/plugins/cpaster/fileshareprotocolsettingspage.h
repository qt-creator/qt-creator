/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FILESHAREPROTOCOLSETTINGSPAGE_H
#define FILESHAREPROTOCOLSETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>
#include "ui_fileshareprotocolsettingswidget.h"

#include <QSharedPointer>
#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CodePaster {

struct FileShareProtocolSettings {
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

    QWidget *widget();
    void apply();
    void finish() { }

private:
    const QSharedPointer<FileShareProtocolSettings> m_settings;
    QPointer<FileShareProtocolSettingsWidget> m_widget;
};
} // namespace CodePaster

#endif // FILESHAREPROTOCOLSETTINGSPAGE_H

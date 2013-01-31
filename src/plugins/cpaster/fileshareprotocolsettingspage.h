/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }

private:
    const QSharedPointer<FileShareProtocolSettings> m_settings;
    QPointer<FileShareProtocolSettingsWidget> m_widget;
};
} // namespace CodePaster

#endif // FILESHAREPROTOCOLSETTINGSPAGE_H

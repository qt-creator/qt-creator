/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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

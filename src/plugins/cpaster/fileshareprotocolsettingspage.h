/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef FILESHAREPROTOCOLSETTINGSPAGE_H
#define FILESHAREPROTOCOLSETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>
#include "ui_fileshareprotocolsettingswidget.h"

#include <QtCore/QSharedPointer>
#include <QtCore/QPointer>
#include <QtGui/QWidget>

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
    Ui::FileShareProtocolSettingsWidget m_ui;
};
class FileShareProtocolSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    explicit FileShareProtocolSettingsPage(const QSharedPointer<FileShareProtocolSettings> &s,
                                           QObject *parent = 0);

    QString id() const;
    QString displayName() const;
    QString category() const;
    QString displayCategory() const;
    QIcon categoryIcon() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }

private:
    const QSharedPointer<FileShareProtocolSettings> m_settings;
    QPointer<FileShareProtocolSettingsWidget> m_widget;
};
} // namespace CodePaster

#endif // FILESHAREPROTOCOLSETTINGSPAGE_H

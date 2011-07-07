/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef TABPREFERENCES_H
#define TABPREFERENCES_H

#include "ifallbackpreferences.h"
#include "tabsettings.h"

namespace TextEditor {

class TEXTEDITOR_EXPORT TabPreferences : public IFallbackPreferences
{
    Q_OBJECT
public:
    explicit TabPreferences(
        const QList<IFallbackPreferences *> &fallbacks,
        QObject *parentObject = 0);

    explicit TabPreferences(
        const QList<TabPreferences *> &fallbacks,
        QObject *parentObject = 0);

    virtual QVariant value() const;
    virtual void setValue(const QVariant &);

    TabSettings settings() const;

    // tracks parent fallbacks until null and extracts settings from it
    TabSettings currentSettings() const;

    virtual void toMap(const QString &prefix, QVariantMap *map) const;
    virtual void fromMap(const QString &prefix, const QVariantMap &map);

public slots:
    void setSettings(const TextEditor::TabSettings &tabSettings);

signals:
    void settingsChanged(const TextEditor::TabSettings &);
    void currentSettingsChanged(const TextEditor::TabSettings &);

protected:
    virtual QString settingsSuffix() const;

private slots:
    void slotCurrentValueChanged(const QVariant &);

private:
    TabSettings m_data;
};

} // namespace TextEditor

#endif // TABPREFERENCES_H

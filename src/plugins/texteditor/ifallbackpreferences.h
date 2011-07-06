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

#ifndef IFALLBACKPREFERENCES_H
#define IFALLBACKPREFERENCES_H

#include "texteditor_global.h"

#include <QtCore/QObject>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

namespace Internal {
class IFallbackPreferencesPrivate;
}

class TabSettings;

class TEXTEDITOR_EXPORT IFallbackPreferences : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
public:
    explicit IFallbackPreferences(const QList<IFallbackPreferences *> &fallbacks, QObject *parentObject = 0);
    virtual ~IFallbackPreferences();

    QString id() const;
    void setId(const QString &name);

    QString displayName() const;
    void setDisplayName(const QString &name);

    bool isReadOnly() const;
    void setReadOnly(bool on);

    bool isFallbackEnabled(IFallbackPreferences *fallback) const;
    void setFallbackEnabled(IFallbackPreferences *fallback, bool on);

    virtual IFallbackPreferences *clone() const;

    virtual QVariant value() const = 0;
    virtual void setValue(const QVariant &) = 0;

    QVariant currentValue() const; // may be from grandparent

    IFallbackPreferences *currentPreferences() const; // may be grandparent

    QList<IFallbackPreferences *> fallbacks() const;
    IFallbackPreferences *currentFallback() const; // null or one of the above list
    void setCurrentFallback(IFallbackPreferences *fallback);

    QString currentFallbackId() const;
    void setCurrentFallback(const QString &id);

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    // make below 2 protected?
    virtual void toMap(const QString &prefix, QVariantMap *map) const = 0;
    virtual void fromMap(const QString &prefix, const QVariantMap &map) = 0;

signals:
    void valueChanged(const QVariant &);
    void currentValueChanged(const QVariant &);
    void currentFallbackChanged(TextEditor::IFallbackPreferences *currentFallback);
    void currentPreferencesChanged(TextEditor::IFallbackPreferences *currentPreferences);

protected:
    virtual QString settingsSuffix() const = 0;

private:
    Internal::IFallbackPreferencesPrivate *d;
};

} // namespace TextEditor

#endif // IFALLBACKPREFERENCES_H

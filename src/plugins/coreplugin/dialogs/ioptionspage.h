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

#ifndef IOPTIONSPAGE_H
#define IOPTIONSPAGE_H

#include <coreplugin/id.h>

#include <QIcon>
#include <QObject>

namespace Core {

class CORE_EXPORT IOptionsPage : public QObject
{
    Q_OBJECT

public:
    IOptionsPage(QObject *parent = 0) : QObject(parent) {}

    Id id() const { return m_id; }
    QString displayName() const { return m_displayName; }
    Id category() const { return m_category; }
    QString displayCategory() const { return m_displayCategory; }
    QIcon categoryIcon() const { return QIcon(m_categoryIcon); }

    virtual bool matches(const QString & /* searchKeyWord*/) const { return false; }
    virtual QWidget *createPage(QWidget *parent) = 0;
    virtual void apply() = 0;
    virtual void finish() = 0;

protected:
    void setId(Id id) { m_id = id; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setCategory(Id category) { m_category = category; }
    void setDisplayCategory(const QString &displayCategory) { m_displayCategory = displayCategory; }
    void setCategoryIcon(const QString &categoryIcon) { m_categoryIcon = categoryIcon; }

    Id m_id;
    Id m_category;
    QString m_displayName;
    QString m_displayCategory;
    QString m_categoryIcon;
};

/*
    Alternative way for providing option pages instead of adding IOptionsPage
    objects into the plugin manager pool. Should only be used if creation of the
    actual option pages is not possible or too expensive at Qt Creator startup.
    (Like the designer integration, which needs to initialize designer plugins
    before the options pages get available.)
*/

class CORE_EXPORT IOptionsPageProvider : public QObject
{
    Q_OBJECT

public:
    IOptionsPageProvider(QObject *parent = 0) : QObject(parent) {}

    Core::Id category() const { return m_category; }
    QString displayCategory() const { return m_displayCategory; }
    QIcon categoryIcon() const { return QIcon(m_categoryIcon); }

    virtual QList<IOptionsPage *> pages() const = 0;

protected:
    void setCategory(Core::Id category) { m_category = category; }
    void setDisplayCategory(const QString &displayCategory) { m_displayCategory = displayCategory; }
    void setCategoryIcon(const QString &categoryIcon) { m_categoryIcon = categoryIcon; }

    Core::Id m_category;
    QString m_displayCategory;
    QString m_categoryIcon;
};

} // namespace Core

#endif // IOPTIONSPAGE_H

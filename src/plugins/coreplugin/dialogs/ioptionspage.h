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

#include <coreplugin/id.h>

#include <utils/icon.h>

#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QIcon;
class QWidget;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT IOptionsPage : public QObject
{
    Q_OBJECT

public:
    IOptionsPage(QObject *parent = 0);
    virtual ~IOptionsPage();

    Id id() const { return m_id; }
    QString displayName() const { return m_displayName; }
    Id category() const { return m_category; }
    QString displayCategory() const { return m_displayCategory; }
    QIcon categoryIcon() const;

    virtual bool matches(const QString &searchKeyWord) const;
    virtual QWidget *widget() = 0;
    virtual void apply() = 0;
    virtual void finish() = 0;

protected:
    void setId(Id id) { m_id = id; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setCategory(Id category) { m_category = category; }
    void setDisplayCategory(const QString &displayCategory) { m_displayCategory = displayCategory; }
    void setCategoryIcon(const Utils::Icon &categoryIcon) { m_categoryIcon = categoryIcon; }

    Id m_id;
    Id m_category;
    QString m_displayName;
    QString m_displayCategory;
    Utils::Icon m_categoryIcon;

    mutable bool m_keywordsInitialized;
    mutable QStringList m_keywords;
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

    Id category() const { return m_category; }
    QString displayCategory() const { return m_displayCategory; }
    QIcon categoryIcon() const;

    virtual QList<IOptionsPage *> pages() const = 0;
    virtual bool matches(const QString & /* searchKeyWord*/) const = 0;

protected:
    void setCategory(Id category) { m_category = category; }
    void setDisplayCategory(const QString &displayCategory) { m_displayCategory = displayCategory; }
    void setCategoryIcon(const QString &categoryIcon) { m_categoryIcon = categoryIcon; }

    Id m_category;
    QString m_displayCategory;
    QString m_categoryIcon;
};

} // namespace Core

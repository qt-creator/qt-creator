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

#include "utils_global.h"

#include <QWizard>

namespace Utils {

class Wizard;
class WizardProgress;
class WizardPrivate;

const char SHORT_TITLE_PROPERTY[] = "shortTitle";

class QTCREATOR_UTILS_EXPORT Wizard : public QWizard
{
    Q_OBJECT
    Q_PROPERTY(bool automaticProgressCreationEnabled READ isAutomaticProgressCreationEnabled WRITE setAutomaticProgressCreationEnabled)

public:
    explicit Wizard(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~Wizard() override;

    bool isAutomaticProgressCreationEnabled() const;
    void setAutomaticProgressCreationEnabled(bool enabled);

    void setStartId(int pageId);

    WizardProgress *wizardProgress() const;

    template<class T> T *find() const
    {
        foreach (int id, pageIds()) {
            if (T *result = qobject_cast<T *>(page(id)))
                return result;
        }
        return 0;
    }

    // will return true for all fields registered via Utils::WizardPage::registerFieldWithName(...)
    bool hasField(const QString &name) const;
    void registerFieldName(const QString &name);
    QSet<QString> fieldNames() const;

    virtual QHash<QString, QVariant> variables() const;

    void showVariables();

protected:
    virtual QString stringify(const QVariant &v) const;
    virtual QString evaluate(const QVariant &v) const;
    bool event(QEvent *event) override;

private:
    void _q_currentPageChanged(int pageId);
    void _q_pageAdded(int pageId);
    void _q_pageRemoved(int pageId);

    Q_DECLARE_PRIVATE(Wizard)
    class WizardPrivate *d_ptr;
};

class WizardProgressItem;
class WizardProgressPrivate;

class QTCREATOR_UTILS_EXPORT WizardProgress : public QObject
{
    Q_OBJECT

public:
    WizardProgress(QObject *parent = 0);
    ~WizardProgress();

    WizardProgressItem *addItem(const QString &title);
    void removeItem(WizardProgressItem *item);

    void removePage(int pageId);

    QList<int> pages(WizardProgressItem *item) const;
    WizardProgressItem *item(int pageId) const;

    WizardProgressItem *currentItem() const;

    QList<WizardProgressItem *> items() const;

    WizardProgressItem *startItem() const;

    QList<WizardProgressItem *> visitedItems() const;
    QList<WizardProgressItem *> directlyReachableItems() const;
    bool isFinalItemDirectlyReachable() const; // return  availableItems().last()->isFinalItem();

Q_SIGNALS:
    void currentItemChanged(WizardProgressItem *item);

    void itemChanged(WizardProgressItem *item); // contents of the item: title or icon
    void itemAdded(WizardProgressItem *item);
    void itemRemoved(WizardProgressItem *item);
    void nextItemsChanged(WizardProgressItem *item, const QList<WizardProgressItem *> &items);
    void nextShownItemChanged(WizardProgressItem *item, WizardProgressItem *nextShownItem);
    void startItemChanged(WizardProgressItem *item);

private:
    void setCurrentPage(int pageId);
    void setStartPage(int pageId);

private:
    friend class Wizard;
    friend class WizardProgressItem;

    Q_DECLARE_PRIVATE(WizardProgress)

    class WizardProgressPrivate *d_ptr;
};

class WizardProgressItemPrivate;

class QTCREATOR_UTILS_EXPORT WizardProgressItem // managed by WizardProgress
{

public:
    void addPage(int pageId);
    QList<int> pages() const;
    void setNextItems(const QList<WizardProgressItem *> &items);
    QList<WizardProgressItem *> nextItems() const;
    void setNextShownItem(WizardProgressItem *item);
    WizardProgressItem *nextShownItem() const;
    bool isFinalItem() const; // return nextItems().isEmpty();

    void setTitle(const QString &title);
    QString title() const;
    void setTitleWordWrap(bool wrap);
    bool titleWordWrap() const;

protected:
    WizardProgressItem(WizardProgress *progress, const QString &title);
    virtual ~WizardProgressItem();

private:
    friend class WizardProgress;

    Q_DECLARE_PRIVATE(WizardProgressItem)

    class WizardProgressItemPrivate *d_ptr;
};

} // namespace Utils

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    explicit Wizard(QWidget *parent = nullptr, Qt::WindowFlags flags = {});
    ~Wizard() override;

    bool isAutomaticProgressCreationEnabled() const;
    void setAutomaticProgressCreationEnabled(bool enabled);

    void setStartId(int pageId);

    WizardProgress *wizardProgress() const;

    template<class T> T *find() const
    {
        const QList<int> pages = pageIds();
        for (int id : pages) {
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
    WizardProgress(QObject *parent = nullptr);
    ~WizardProgress() override;

    WizardProgressItem *addItem(const QString &title);
    void removeItem(WizardProgressItem *item);

    void removePage(int pageId);

    static QList<int> pages(WizardProgressItem *item);
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

    friend QTCREATOR_UTILS_EXPORT QDebug &operator<<(QDebug &debug, const WizardProgress &progress);

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
    friend QTCREATOR_UTILS_EXPORT QDebug &operator<<(QDebug &d, const WizardProgressItem &item);

    Q_DECLARE_PRIVATE(WizardProgressItem)

    class WizardProgressItemPrivate *d_ptr;
};

QTCREATOR_UTILS_EXPORT QDebug &operator<<(QDebug &debug, const WizardProgress &progress);

QTCREATOR_UTILS_EXPORT QDebug &operator<<(QDebug &debug, const WizardProgressItem &item);

} // namespace Utils

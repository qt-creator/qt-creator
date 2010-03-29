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

#ifndef WIZARD_H
#define WIZARD_H

#include <QWizard>

#include "utils_global.h"

#include <QtGui/QWizard>

namespace Utils {

class WizardProgress;
class WizardPrivate;

class QTCREATOR_UTILS_EXPORT Wizard : public QWizard
{
    Q_OBJECT
    Q_PROPERTY(bool automaticProgressCreationEnabled READ isAutomaticProgressCreationEnabled WRITE setAutomaticProgressCreationEnabled)

public:
    explicit Wizard(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    virtual ~Wizard();

    bool isAutomaticProgressCreationEnabled() const;
    void setAutomaticProgressCreationEnabled(bool enabled);

    void setStartId(int pageId);

    WizardProgress *wizardProgress() const;

private slots:
    void _q_currentPageChanged(int pageId);
    void _q_pageAdded(int pageId);
    void _q_pageRemoved(int pageId);

private:

    Q_DISABLE_COPY(Wizard)
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
    void startItemChanged(WizardProgressItem *item);

private:
    void setCurrentPage(int pageId);
    void setStartPage(int pageId);

private:
    friend class Wizard;
    friend class WizardProgressItem;

    Q_DISABLE_COPY(WizardProgress)
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

    Q_DISABLE_COPY(WizardProgressItem)
    Q_DECLARE_PRIVATE(WizardProgressItem)

    class WizardProgressItemPrivate *d_ptr;
};

} // namespace Utils

#endif // WIZARD_H

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

#ifndef INFOBAR_H
#define INFOBAR_H

#include "core_global.h"
#include <coreplugin/id.h>

#include <QObject>
#include <QSet>

QT_BEGIN_NAMESPACE
class QBoxLayout;
QT_END_NAMESPACE

namespace Core {

class InfoBar;
class InfoBarDisplay;

class CORE_EXPORT InfoBarEntry
{
public:
    enum GlobalSuppressionMode
    {
        GlobalSuppressionDisabled,
        GlobalSuppressionEnabled
    };

    InfoBarEntry(Id _id, const QString &_infoText, GlobalSuppressionMode _globalSuppression = GlobalSuppressionDisabled);
    InfoBarEntry(const InfoBarEntry &other) { *this = other; }
    void setCustomButtonInfo(const QString &_buttonText, QObject *_object, const char *_member);
    void setCancelButtonInfo(QObject *_object, const char *_member);
    void setCancelButtonInfo(const QString &_cancelButtonText, QObject *_object, const char *_member);

private:
    Id id;
    QString infoText;
    QString buttonText;
    QObject *object;
    const char *buttonPressMember;
    QString cancelButtonText;
    QObject *cancelObject;
    const char *cancelButtonPressMember;
    GlobalSuppressionMode globalSuppression;
    friend class InfoBar;
    friend class InfoBarDisplay;
};

class CORE_EXPORT InfoBar : public QObject
{
    Q_OBJECT

public:
    void addInfo(const InfoBarEntry &info);
    void removeInfo(Id id);
    bool containsInfo(Id id) const;
    void suppressInfo(Id id);
    bool canInfoBeAdded(Id id) const;
    void enableInfo(Id id);
    void clear();
    static void globallySuppressInfo(Id id);
    static void initializeGloballySuppressed();
    static void clearGloballySuppressed();
    static bool anyGloballySuppressed();

signals:
    void changed();

private:
    QList<InfoBarEntry> m_infoBarEntries;
    QSet<Id> m_suppressed;
    static QSet<Id> globallySuppressed;
    friend class InfoBarDisplay;
};

class CORE_EXPORT InfoBarDisplay : public QObject
{
    Q_OBJECT

public:
    InfoBarDisplay(QObject *parent = 0);
    void setTarget(QBoxLayout *layout, int index);
    void setInfoBar(InfoBar *infoBar);

private slots:
    void cancelButtonClicked();
    void suppressButtonClicked();
    void update();
    void infoBarDestroyed();
    void widgetDestroyed();

private:
    QList<QWidget *> m_infoWidgets;
    InfoBar *m_infoBar;
    QBoxLayout *m_boxLayout;
    int m_boxIndex;
};

} // namespace Core

#endif // INFOBAR_H

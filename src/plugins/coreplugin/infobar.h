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

#include "core_global.h"
#include <coreplugin/id.h>

#include <QObject>
#include <QSet>

#include <functional>

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

    using CallBack = std::function<void()>;
    void setCustomButtonInfo(const QString &_buttonText, CallBack callBack);
    void setCancelButtonInfo(CallBack callBack);
    void setCancelButtonInfo(const QString &_cancelButtonText, CallBack callBack);
    void removeCancelButton();

    using DetailsWidgetCreator = std::function<QWidget*()>;
    void setDetailsWidgetCreator(const DetailsWidgetCreator &creator);

private:
    Id id;
    QString infoText;
    QString buttonText;
    CallBack m_buttonCallBack;
    QString cancelButtonText;
    CallBack m_cancelButtonCallBack;
    GlobalSuppressionMode globalSuppression;
    DetailsWidgetCreator m_detailsWidgetCreator;
    bool m_useCancelButton = true;
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
    static void globallyUnsuppressInfo(Id id);
    static void initializeGloballySuppressed();
    static void clearGloballySuppressed();
    static bool anyGloballySuppressed();

signals:
    void changed();

private:
    static void writeGloballySuppressedToSettings();

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

private:
    void update();
    void infoBarDestroyed();
    void widgetDestroyed();

    QList<QWidget *> m_infoWidgets;
    InfoBar *m_infoBar = nullptr;
    QBoxLayout *m_boxLayout = nullptr;
    int m_boxIndex = 0;
    bool m_isShowingDetailsWidget = false;
};

} // namespace Core

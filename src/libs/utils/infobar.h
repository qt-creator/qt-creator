// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "id.h"
#include "qtcsettings.h"

#include <QObject>
#include <QSet>
#include <QVariant>

#include <functional>

QT_BEGIN_NAMESPACE
class QBoxLayout;
class QSettings;
QT_END_NAMESPACE

namespace Utils {

class InfoBar;
class InfoBarDisplay;
class Theme;

class QTCREATOR_UTILS_EXPORT InfoBarEntry
{
public:
    enum class GlobalSuppression
    {
        Disabled,
        Enabled
    };

    InfoBarEntry() = default;
    InfoBarEntry(Id _id, const QString &_infoText, GlobalSuppression _globalSuppression = GlobalSuppression::Disabled);

    Id id() const;
    QString text() const;

    using CallBack = std::function<void()>;
    void addCustomButton(const QString &_buttonText, CallBack callBack, const QString &tooltip = {});
    void setCancelButtonInfo(CallBack callBack);
    void setCancelButtonInfo(const QString &_cancelButtonText, CallBack callBack);
    struct ComboInfo
    {
        QString displayText;
        QVariant data;
    };
    using ComboCallBack = std::function<void(const ComboInfo &)>;
    void setComboInfo(const QStringList &list, ComboCallBack callBack, const QString &tooltip = {}, int currentIndex = -1);
    void setComboInfo(const QList<ComboInfo> &infos, ComboCallBack callBack, const QString &tooltip = {}, int currentIndex = -1);
    void removeCancelButton();

    using DetailsWidgetCreator = std::function<QWidget*()>;
    void setDetailsWidgetCreator(const DetailsWidgetCreator &creator);

private:
    struct Button
    {
        QString text;
        CallBack callback;
        QString tooltip;
    };

    struct Combo
    {
        ComboCallBack callback;
        QList<ComboInfo> entries;
        QString tooltip;
        int currentIndex = -1;
    };

    Id m_id;
    QString m_infoText;
    QList<Button> m_buttons;
    QString m_cancelButtonText;
    CallBack m_cancelButtonCallBack;
    GlobalSuppression m_globalSuppression = GlobalSuppression::Disabled;
    DetailsWidgetCreator m_detailsWidgetCreator;
    bool m_useCancelButton = true;
    Combo m_combo;
    friend class InfoBar;
    friend class InfoBarDisplay;
};

class QTCREATOR_UTILS_EXPORT InfoBar : public QObject
{
    Q_OBJECT

public:
    void addInfo(const InfoBarEntry &info);
    void removeInfo(Id id);
    bool containsInfo(Id id) const;
    void suppressInfo(Id id);
    bool canInfoBeAdded(Id id) const;
    void unsuppressInfo(Id id);
    void clear();
    static void globallySuppressInfo(Id id);
    static void globallyUnsuppressInfo(Id id);
    static void clearGloballySuppressed();
    static bool anyGloballySuppressed();

    static void initialize(QtcSettings *settings);
    static QtcSettings *settings();

signals:
    void changed();

private:
    static void writeGloballySuppressedToSettings();

private:
    QList<InfoBarEntry> m_infoBarEntries;
    QSet<Id> m_suppressed;

    static QSet<Id> globallySuppressed;
    static QtcSettings *m_settings;

    friend class InfoBarDisplay;
};

class QTCREATOR_UTILS_EXPORT InfoBarDisplay : public QObject
{
public:
    InfoBarDisplay(QObject *parent = nullptr);
    void setTarget(QBoxLayout *layout, int index);
    void setInfoBar(InfoBar *infoBar);
    void setEdge(Qt::Edge edge);

    InfoBar *infoBar() const;

private:
    void update();
    void infoBarDestroyed();

    QList<QWidget *> m_infoWidgets;
    InfoBar *m_infoBar = nullptr;
    QBoxLayout *m_boxLayout = nullptr;
    Qt::Edge m_edge = Qt::TopEdge;
    int m_boxIndex = 0;
    bool m_isShowingDetailsWidget = false;
};

} // namespace Utils

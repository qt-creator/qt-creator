// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "minisplitter.h"

#include <utils/id.h>
#include <utils/store.h>

#include <QHash>

QT_BEGIN_NAMESPACE
class QSettings;
class QAbstractItemModel;
QT_END_NAMESPACE

namespace Utils {
class QtcSettings;
}

namespace Core {
class INavigationWidgetFactory;
class Command;
class NavigationWidget;
struct NavigationWidgetPrivate;
namespace Internal { class NavigationSubWidget; }

enum class Side {
    Left,
    Right
};

class CORE_EXPORT NavigationWidgetPlaceHolder : public QWidget
{
    Q_OBJECT
    friend class Core::NavigationWidget;

public:
    explicit NavigationWidgetPlaceHolder(Utils::Id mode, Side side, QWidget *parent = nullptr);
    ~NavigationWidgetPlaceHolder() override;
    static NavigationWidgetPlaceHolder *current(Side side);
    static void setCurrent(Side side, NavigationWidgetPlaceHolder *navWidget);
    void applyStoredSize();

private:
    void currentModeAboutToChange(Utils::Id mode);
    int storedWidth() const;

    Utils::Id m_mode;
    Side m_side;
    static NavigationWidgetPlaceHolder *s_currentLeft;
    static NavigationWidgetPlaceHolder *s_currentRight;
};

class CORE_EXPORT NavigationWidget : public MiniSplitter
{
    Q_OBJECT

public:
    enum FactoryModelRoles {
        FactoryObjectRole = Qt::UserRole,
        FactoryIdRole,
        FactoryActionIdRole,
        FactoryPriorityRole
    };

    explicit NavigationWidget(QAction *toggleSideBarAction, Side side);
    ~NavigationWidget() override;

    void setFactories(const QList<INavigationWidgetFactory*> &factories);

    Utils::Key settingsGroup() const;
    void saveSettings(Utils::QtcSettings *settings);
    void restoreSettings(Utils::QtcSettings *settings);

    QWidget *activateSubWidget(Utils::Id factoryId, int preferredPosition);
    void closeSubWidgets();

    bool isShown() const;
    void setShown(bool b);

    static NavigationWidget *instance(Side side);
    static QWidget *activateSubWidget(Utils::Id factoryId, Side fallbackSide);

    int storedWidth();

    // Called from the place holders
    void placeHolderChanged(NavigationWidgetPlaceHolder *holder);

    QHash<Utils::Id, Command *> commandMap() const;
    QAbstractItemModel *factoryModel() const;

protected:
    void resizeEvent(QResizeEvent *) override;

private:
    void closeSubWidget(Internal::NavigationSubWidget *subWidget);
    void updateToggleText();
    Internal::NavigationSubWidget *insertSubItem(int position,
                                                 int factoryIndex,
                                                 bool updateActivationsMap = true);
    int factoryIndex(Utils::Id id);
    Utils::Key settingsKey(const Utils::Key &key) const;

    NavigationWidgetPrivate *d;
};

} // namespace Core

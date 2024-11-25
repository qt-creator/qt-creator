// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <utils/aspects.h>
#include <utils/id.h>

#include <functional>
#include <memory>

namespace Core {

namespace Internal {
class IOptionsPageWidgetPrivate;
class IOptionsPagePrivate;
class IOptionsPageProviderPrivate;
} // namespace Internal

class CORE_EXPORT IOptionsPageWidget : public QWidget
{
    Q_OBJECT

public:
    IOptionsPageWidget();
    ~IOptionsPageWidget();
    void setOnApply(const std::function<void()> &func);
    void setOnCancel(const std::function<void()> &func);
    void setOnFinish(const std::function<void()> &func);

protected:
    friend class IOptionsPage;
    virtual void apply();
    virtual void cancel();
    virtual void finish();

private:
    std::unique_ptr<Internal::IOptionsPageWidgetPrivate> d;
};

class CORE_EXPORT IOptionsPage
{
    Q_DISABLE_COPY_MOVE(IOptionsPage)

public:
    explicit IOptionsPage(bool registerGlobally = true);
    virtual ~IOptionsPage();

    static const QList<IOptionsPage *> allOptionsPages();

    Utils::Id id() const;
    QString displayName() const;
    Utils::Id category() const;
    QString displayCategory() const;
    Utils::FilePath categoryIconPath() const;

    using WidgetCreator = std::function<QWidget *()>;
    void setWidgetCreator(const WidgetCreator &widgetCreator);

    virtual QWidget *widget();
    virtual void apply();
    virtual void cancel();
    virtual void finish();

    virtual bool matches(const QRegularExpression &regexp) const;

protected:
    virtual QStringList keywords() const;

    void setId(Utils::Id id);
    void setDisplayName(const QString &displayName);
    void setCategory(Utils::Id category);
    void setDisplayCategory(const QString &displayCategory);
    void setCategoryIconPath(const Utils::FilePath &categoryIconPath);
    void setSettingsProvider(const std::function<Utils::AspectContainer *()> &provider);

private:
    std::unique_ptr<Internal::IOptionsPagePrivate> d;
};

/*
    Alternative way for providing option pages instead of adding IOptionsPage
    objects into the plugin manager pool. Should only be used if creation of the
    actual option pages is not possible or too expensive at Qt Creator startup.
    (Like the designer integration, which needs to initialize designer plugins
    before the options pages get available.)
*/

class CORE_EXPORT IOptionsPageProvider
{
    Q_DISABLE_COPY_MOVE(IOptionsPageProvider)

public:
    IOptionsPageProvider();
    virtual ~IOptionsPageProvider();

    static const QList<IOptionsPageProvider *> allOptionsPagesProviders();

    Utils::Id category() const;
    QString displayCategory() const;
    Utils::FilePath categoryIconPath() const;

    virtual QList<IOptionsPage *> pages() const = 0;
    virtual bool matches(const QRegularExpression &regexp) const = 0;

protected:
    void setCategory(Utils::Id category);
    void setDisplayCategory(const QString &displayCategory);
    void setCategoryIconPath(const Utils::FilePath &iconPath);

    std::unique_ptr<Internal::IOptionsPageProviderPrivate> d;
};

// Which part of the settings page to pre-select, if applicable. In practice, this will
// usually be an item in some sort of (list) view.
void CORE_EXPORT setPreselectedOptionsPageItem(Utils::Id page, Utils::Id item);
Utils::Id CORE_EXPORT preselectedOptionsPageItem(Utils::Id page);

} // namespace Core

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"

#include <coreplugin/core_global.h>

#include <QDialog>
#include <QMutex>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QListWidget;
class QPushButton;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT UrlLocatorFilter final : public Core::ILocatorFilter
{
    Q_OBJECT
public:
    UrlLocatorFilter(Utils::Id id);
    UrlLocatorFilter(const QString &displayName, Utils::Id id);
    ~UrlLocatorFilter() final;

    // ILocatorFilter
    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
    void accept(const Core::LocatorFilterEntry &selection,
                QString *newText, int *selectionStart, int *selectionLength) const override;
    void restoreState(const QByteArray &state) override;
    bool openConfigDialog(QWidget *parent, bool &needsRefresh) override;

    void addDefaultUrl(const QString &urlTemplate);
    QStringList remoteUrls() const;

    void setIsCustomFilter(bool value);
    bool isCustomFilter() const;

protected:
    void saveState(QJsonObject &object) const final;
    void restoreState(const QJsonObject &object) final;

private:
    QString m_defaultDisplayName;
    QStringList m_defaultUrls;
    QStringList m_remoteUrls;
    bool m_isCustomFilter = false;
    mutable QMutex m_mutex;
};

namespace Internal {

class UrlFilterOptions : public QDialog
{
    Q_OBJECT
    friend class Core::UrlLocatorFilter;

public:
    explicit UrlFilterOptions(UrlLocatorFilter *filter, QWidget *parent = nullptr);

private:
    void addNewItem();
    void removeItem();
    void moveItemUp();
    void moveItemDown();
    void updateActionButtons();

    UrlLocatorFilter *m_filter = nullptr;
    QLineEdit *nameEdit;
    QListWidget *listWidget;
    QPushButton *remove;
    QPushButton *moveUp;
    QPushButton *moveDown;
    QLineEdit *shortcutEdit;
    QCheckBox *includeByDefault;
};

} // namespace Internal

} // namespace Core

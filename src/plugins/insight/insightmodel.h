// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <model.h>

#include <QAbstractListModel>
#include <QFileInfo>
#include <QPointer>
#include <QPoint>

#include <utils/filesystemwatcher.h>

#include <3rdparty/json/json.hpp>

namespace QmlDesigner {

class InsightView;

typedef nlohmann::json json;

class InsightModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
    Q_PROPERTY(QString token READ token NOTIFY tokenChanged)
    Q_PROPERTY(int minutes READ minutes NOTIFY minutesChanged)
    Q_PROPERTY(Qt::CheckState predefinedSelectState MEMBER m_predefinedCheckState NOTIFY
                   predefinedSelectStateChanged)
    Q_PROPERTY(Qt::CheckState customSelectState MEMBER m_customCheckState NOTIFY customSelectStateChanged)

    enum {
        CategoryName = Qt::DisplayRole,
        CategoryColor = Qt::UserRole,
        CategoryType,
        CategoryActive
    };

public:
    InsightModel(InsightView *view, class ExternalDependenciesInterface &externalDependencies);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setup();

    Q_INVOKABLE void addCategory();
    Q_INVOKABLE void removeCateogry(int idx);
    Q_INVOKABLE bool renameCategory(int idx, const QString &name);

    Q_INVOKABLE void setCategoryActive(int idx, bool value);

    bool enabled() const;
    Q_INVOKABLE void setEnabled(bool value);

    QString token() const;
    Q_INVOKABLE void setToken(const QString &value);

    int minutes() const;
    Q_INVOKABLE void setMinutes(int value);

    Q_INVOKABLE void selectAllPredefined();
    Q_INVOKABLE void selectAllCustom();

    void handleFileChange(const QString &path);

    void setAuxiliaryEnabled(bool value);
    void setAuxiliaryCategories(const std::vector<std::string> &categories);

    Q_INVOKABLE void hideCursor();
    Q_INVOKABLE void restoreCursor();
    Q_INVOKABLE void holdCursorInPlace();

    Q_INVOKABLE int devicePixelRatio();

signals:
    void enabledChanged();
    void tokenChanged();
    void minutesChanged();

    void predefinedSelectStateChanged();
    void customSelectStateChanged();

private:
    void parseMainQml();
    void parseDefaultConfig();
    void parseConfig();
    void parseQtdsConfig();

    void createQtdsConfig();
    void updateQtdsConfig();

    void selectAll(const std::vector<std::string> &categories, Qt::CheckState checkState);

    std::vector<std::string> predefinedCategories() const;
    std::vector<std::string> activeCategories() const;
    std::vector<std::string> customCategories() const;
    std::vector<std::string> categories() const;

    bool hasCategory(const QString &name) const;
    void updateCheckState();

    template<typename T>
    void writeConfigValue(const json::json_pointer &ptr, T value);

private:
    QPointer<InsightView> m_insightView;
    ExternalDependenciesInterface &m_externalDependencies;

    Utils::FileSystemWatcher *m_fileSystemWatcher;

    bool m_enabled = false;
    bool m_initialized = false;

    QFileInfo m_mainQmlInfo;
    QFileInfo m_configInfo;
    QFileInfo m_qtdsConfigInfo;

    json m_defaultConfig;
    json m_config;
    json m_qtdsConfig;

    Qt::CheckState m_predefinedCheckState;
    Qt::CheckState m_customCheckState;

    QPoint m_lastPos;
};

} // namespace QmlDesigner

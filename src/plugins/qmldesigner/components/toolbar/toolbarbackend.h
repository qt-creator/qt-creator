// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QObject>

namespace QmlDesigner {

class ActionInterface;

class CrumbleBarModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit CrumbleBarModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void handleCrumblePathChanged();

    Q_INVOKABLE void onCrumblePathElementClicked(int i);
};

class WorkspaceModel : public QAbstractListModel
{
    Q_OBJECT

    enum { DisplayNameRole = Qt::DisplayRole, FileNameRole = Qt::UserRole, Enabled };

public:
    explicit WorkspaceModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = DisplayNameRole) const override;
};

class RunManagerModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum RunManagerRoles {
        DisplayNameRole = Qt::DisplayRole,
        TargetNameRole = Qt::UserRole,
        Enabled
    };
    Q_ENUM(RunManagerRoles)

    explicit RunManagerModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    void reset();
};

class ActionSubscriber : public QObject
{
    Q_OBJECT

public:
    ActionSubscriber(QObject *parent = nullptr);

    Q_PROPERTY(QString actionId READ actionId WRITE setActionId NOTIFY actionIdChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(bool checked READ checked NOTIFY checkedChanged)
    Q_PROPERTY(QString tooltip READ tooltip NOTIFY tooltipChanged)

    Q_INVOKABLE void trigger();

    bool available() const;
    bool checked() const;

    QString actionId() const;
    void setActionId(const QString &id);

    QString tooltip() const;

signals:
    void actionIdChanged();
    void availableChanged();
    void checkedChanged();
    void tooltipChanged();

private:
    void setupNotifier();

    ActionInterface *m_interface = nullptr;
    QString m_actionId;
};

class ToolBarBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY navigationHistoryChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY navigationHistoryChanged)
    Q_PROPERTY(QStringList documentModel READ documentModel NOTIFY openDocumentsChanged)
    Q_PROPERTY(int documentIndex READ documentIndex NOTIFY documentIndexChanged)
    Q_PROPERTY(QString currentWorkspace READ currentWorkspace NOTIFY currentWorkspaceChanged)
    Q_PROPERTY(bool lockWorkspace READ lockWorkspace WRITE setLockWorkspace NOTIFY lockWorkspaceChanged)
    Q_PROPERTY(QStringList styles READ styles NOTIFY stylesChanged)
    Q_PROPERTY(bool isInDesignMode READ isInDesignMode NOTIFY isInDesignModeChanged)
    Q_PROPERTY(bool isInEditMode READ isInEditMode NOTIFY isInEditModeChanged)
    Q_PROPERTY(bool isInSessionMode READ isInSessionMode NOTIFY isInSessionModeChanged)
    Q_PROPERTY(bool isDesignModeEnabled READ isDesignModeEnabled NOTIFY isDesignModeEnabledChanged)
    Q_PROPERTY(int currentStyle READ currentStyle NOTIFY currentStyleChanged)
    Q_PROPERTY(QStringList kits READ kits NOTIFY kitsChanged)
    Q_PROPERTY(int currentKit READ currentKit NOTIFY currentKitChanged)
    Q_PROPERTY(bool isQt6 READ isQt6 NOTIFY isQt6Changed)
    Q_PROPERTY(bool isMCUs READ isMCUs NOTIFY isMCUsChanged)
    Q_PROPERTY(bool projectOpened READ projectOpened NOTIFY projectOpenedChanged)
    Q_PROPERTY(bool isSharingEnabled READ isSharingEnabled NOTIFY isSharingEnabledChanged)
    Q_PROPERTY(bool isDocumentDirty READ isDocumentDirty NOTIFY isDocumentDirtyChanged)

    Q_PROPERTY(bool isLiteModeEnabled READ isLiteModeEnabled CONSTANT)

    Q_PROPERTY(int runTargetIndex READ runTargetIndex NOTIFY runTargetIndexChanged)
    Q_PROPERTY(int runManagerState READ runManagerState NOTIFY runManagerStateChanged)

public:
    ToolBarBackend(QObject *parent  = nullptr);
    static void registerDeclarativeType();

    Q_INVOKABLE void triggerModeChange();
    Q_INVOKABLE void triggerProjectSettings();
    Q_INVOKABLE void runProject();
    Q_INVOKABLE void goForward();
    Q_INVOKABLE void goBackward();
    Q_INVOKABLE void openFileByIndex(int i);
    Q_INVOKABLE void closeCurrentDocument();
    Q_INVOKABLE void shareApplicationOnline();
    Q_INVOKABLE void setCurrentWorkspace(const QString &workspace);
    Q_INVOKABLE void setLockWorkspace(bool value);
    Q_INVOKABLE void editGlobalAnnoation();
    Q_INVOKABLE void showZoomMenu(int x, int y);
    Q_INVOKABLE void setCurrentStyle(int index);
    Q_INVOKABLE void setCurrentKit(int index);

    Q_INVOKABLE void openDeviceManager();
    Q_INVOKABLE void selectRunTarget(const QString &targetName);
    Q_INVOKABLE void toggleRunning();

    bool canGoBack() const;
    bool canGoForward() const;

    QStringList documentModel() const;

    void updateDocumentModel();
    int documentIndex() const;

    QString currentWorkspace() const;
    bool lockWorkspace() const;

    QStringList styles() const;

    bool isInDesignMode() const;
    bool isInEditMode() const;
    bool isInSessionMode() const;
    bool isDesignModeEnabled() const;
    int currentStyle() const;

    QStringList kits() const;

    int currentKit() const;

    bool isQt6() const;
    bool isMCUs() const;

    bool projectOpened() const;

    bool isSharingEnabled();

    bool isDocumentDirty() const;

    bool isLiteModeEnabled() const;

    int runTargetIndex() const;
    int runManagerState() const;

    static void launchGlobalAnnotations();

signals:
    void navigationHistoryChanged();
    void openDocumentsChanged();
    void documentIndexChanged();
    void currentWorkspaceChanged();
    void lockWorkspaceChanged();
    void stylesChanged();
    void isInDesignModeChanged();
    void isInEditModeChanged();
    void isInSessionModeChanged();
    void isDesignModeEnabledChanged();
    void currentStyleChanged();
    void kitsChanged();
    void currentKitChanged();
    void isQt6Changed();
    void isMCUsChanged();
    void projectOpenedChanged();
    void isSharingEnabledChanged();
    void isDocumentDirtyChanged();

    void runTargetIndexChanged();
    void runManagerStateChanged();

private:
    void setupWorkspaces();

    ActionInterface *m_zoomAction;

    QStringList m_openDocuments;
    QMetaObject::Connection m_kitConnection;
};

} // namespace QmlDesigner

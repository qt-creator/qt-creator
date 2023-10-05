// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>

#include <QMouseEvent>
#include <QObject>
#include <QQmlComponent>
#include <QQmlPropertyMap>
#include <QColor>
#include <QPoint>
#include <QUrl>

namespace QmlDesigner {

class Model;

class TextureEditorContextObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl specificsUrl READ specificsUrl WRITE setSpecificsUrl NOTIFY specificsUrlChanged)
    Q_PROPERTY(QString specificQmlData READ specificQmlData WRITE setSpecificQmlData NOTIFY specificQmlDataChanged)
    Q_PROPERTY(QQmlComponent *specificQmlComponent READ specificQmlComponent NOTIFY specificQmlComponentChanged)

    Q_PROPERTY(QString stateName READ stateName WRITE setStateName NOTIFY stateNameChanged)
    Q_PROPERTY(QStringList allStateNames READ allStateNames WRITE setAllStateNames NOTIFY allStateNamesChanged)

    Q_PROPERTY(bool isBaseState READ isBaseState WRITE setIsBaseState NOTIFY isBaseStateChanged)
    Q_PROPERTY(bool selectionChanged READ selectionChanged WRITE setSelectionChanged NOTIFY selectionChangedChanged)

    Q_PROPERTY(int majorVersion READ majorVersion WRITE setMajorVersion NOTIFY majorVersionChanged)

    Q_PROPERTY(bool hasAliasExport READ hasAliasExport NOTIFY hasAliasExportChanged)
    Q_PROPERTY(bool hasActiveTimeline READ hasActiveTimeline NOTIFY hasActiveTimelineChanged)
    Q_PROPERTY(bool hasQuick3DImport READ hasQuick3DImport WRITE setHasQuick3DImport NOTIFY hasQuick3DImportChanged)
    Q_PROPERTY(bool hasSingleModelSelection READ hasSingleModelSelection WRITE setHasSingleModelSelection
               NOTIFY hasSingleModelSelectionChanged)
    Q_PROPERTY(bool hasMaterialLibrary READ hasMaterialLibrary WRITE setHasMaterialLibrary NOTIFY hasMaterialLibraryChanged)
    Q_PROPERTY(bool isQt6Project READ isQt6Project NOTIFY isQt6ProjectChanged)

    Q_PROPERTY(QQmlPropertyMap *backendValues READ backendValues WRITE setBackendValues NOTIFY backendValuesChanged)

    Q_PROPERTY(QString activeDragSuffix READ activeDragSuffix NOTIFY activeDragSuffixChanged)

public:
    TextureEditorContextObject(QQmlContext *context, QObject *parent = nullptr);

    QUrl specificsUrl() const { return m_specificsUrl; }
    QString specificQmlData() const {return m_specificQmlData; }
    QQmlComponent *specificQmlComponent();
    QString stateName() const { return m_stateName; }
    QStringList allStateNames() const { return m_allStateNames; }

    bool isBaseState() const { return m_isBaseState; }
    bool selectionChanged() const { return m_selectionChanged; }

    QQmlPropertyMap *backendValues() const { return m_backendValues; }

    Q_INVOKABLE QString convertColorToString(const QVariant &color);
    Q_INVOKABLE QColor colorFromString(const QString &colorString);

    Q_INVOKABLE void insertKeyframe(const QString &propertyName);

    Q_INVOKABLE void hideCursor();
    Q_INVOKABLE void restoreCursor();
    Q_INVOKABLE void holdCursorInPlace();

    Q_INVOKABLE int devicePixelRatio();

    Q_INVOKABLE QStringList allStatesForId(const QString &id);

    Q_INVOKABLE bool isBlocked(const QString &propName) const;
    Q_INVOKABLE void goIntoComponent();
    Q_INVOKABLE QString resolveResourcePath(const QString &path);

    enum ToolBarAction {
        ApplyToSelected,
        AddNewTexture,
        DeleteCurrentTexture,
        OpenMaterialBrowser
    };
    Q_ENUM(ToolBarAction)

    int majorVersion() const;
    void setMajorVersion(int majorVersion);

    bool hasActiveTimeline() const;
    void setHasActiveTimeline(bool b);

    bool hasQuick3DImport() const;
    void setHasQuick3DImport(bool b);

    bool hasMaterialLibrary() const;
    void setHasMaterialLibrary(bool b);

    bool isQt6Project() const;
    void setIsQt6Project(bool b);

    bool hasSingleModelSelection() const;
    void setHasSingleModelSelection(bool b);

    QString activeDragSuffix() const;
    void setActiveDragSuffix(const QString &suffix);

    bool hasAliasExport() const { return m_aliasExport; }

    void setSelectedMaterial(const ModelNode &matNode);

    void setSpecificsUrl(const QUrl &newSpecificsUrl);
    void setSpecificQmlData(const QString &newSpecificQmlData);
    void setStateName(const QString &newStateName);
    void setAllStateNames(const QStringList &allStates);
    void setIsBaseState(bool newIsBaseState);
    void setSelectionChanged(bool newSelectionChanged);
    void setBackendValues(QQmlPropertyMap *newBackendValues);
    void setModel(Model *model);

    void triggerSelectionChanged();
    void setHasAliasExport(bool hasAliasExport);

signals:
    void specificsUrlChanged();
    void specificQmlDataChanged();
    void specificQmlComponentChanged();
    void stateNameChanged();
    void allStateNamesChanged();
    void isBaseStateChanged();
    void selectionChangedChanged();
    void backendValuesChanged();
    void majorVersionChanged();
    void hasAliasExportChanged();
    void hasActiveTimelineChanged();
    void hasQuick3DImportChanged();
    void hasMaterialLibraryChanged();
    void hasSingleModelSelectionChanged();
    void activeDragSuffixChanged();
    void isQt6ProjectChanged();

private:
    QUrl m_specificsUrl;
    QString m_specificQmlData;
    QQmlComponent *m_specificQmlComponent = nullptr;
    QQmlContext *m_qmlContext = nullptr;

    QString m_stateName;
    QStringList m_allStateNames;

    int m_majorVersion = 1;

    QQmlPropertyMap *m_backendValues = nullptr;
    Model *m_model = nullptr;

    QPoint m_lastPos;

    bool m_isBaseState = false;
    bool m_selectionChanged = false;
    bool m_aliasExport = false;
    bool m_hasActiveTimeline = false;
    bool m_hasQuick3DImport = false;
    bool m_hasMaterialLibrary = false;
    bool m_hasSingleModelSelection = false;
    bool m_isQt6Project = false;

    ModelNode m_selectedTexture;

    QString m_activeDragSuffix;
};

} // QmlDesigner

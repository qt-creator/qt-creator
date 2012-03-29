/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLJSINSPECTOR_H
#define QMLJSINSPECTOR_H

#include "qmljsprivateapi.h"

#include <qmlprojectmanager/qmlprojectrunconfiguration.h>
#include <utils/fileinprojectfinder.h>

#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsastfwd_p.h>

#include <debugger/debuggerconstants.h>

#include <QAction>
#include <QObject>

namespace ProjectExplorer {
class Project;
class Environment;
}

namespace TextEditor {
class ITextEditor;
}

namespace Core {
class IContext;
}

namespace QmlJS {
class ModelManagerInterface;
}

namespace QmlJSInspector {
namespace Internal {

class QmlJsInspectorToolBar;
class QmlJSPropertyInspector;
class ClientProxy;
class InspectorSettings;
class ContextCrumblePath;
class QmlJSLiveTextPreview;

class InspectorUi : public QObject
{
    Q_OBJECT

public:
    enum DebugMode {
        StandaloneMode,
        CppProjectWithQmlEngines,
        QmlProjectWithCppPlugins
    };

    InspectorUi(QObject *parent = 0);
    virtual ~InspectorUi();

    void saveSettings() const;
    void restoreSettings();

    bool showExperimentalWarning();
    void setShowExperimentalWarning(bool value);

    static InspectorUi *instance();

    QString findFileInProject(const QUrl &fileUrl) const;

    void setupUi();
    bool isConnected() const;
    void connected(ClientProxy *clientProxy);
    void disconnected();
    void setDebuggerEngine(QObject *qmlEngine);

signals:
    void statusMessage(const QString &text);
    void livePreviewActivated(bool isActivated);

public slots:
    void reloadQmlViewer();
    void serverReloaded();
    void setApplyChangesToQmlInspector(bool applyChanges);
    void onResult(quint32 queryId, const QVariant &result);

private slots:
    void enable();
    void disable();
    void gotoObjectReferenceDefinition(const QmlDebugObjectReference &obj);
    void selectItems(const QList<QmlDebugObjectReference> &objectReferences);
    void selectItems(const QList<int> &objectIds);
    void changeSelectedItems(const QList<QmlDebugObjectReference> &objects);
    void changePropertyValue(int debugId,const QString &propertyName, const QString &valueExpression);
    void objectTreeReady();
    void onRootContext(const QVariant &value);

    void updateEngineList();

    void removePreviewForEditor(Core::IEditor *newEditor);
    QmlJSLiveTextPreview *createPreviewForEditor(Core::IEditor *newEditor);

    void disableLivePreview();
    void crumblePathElementClicked(const QVariant &data);

    void updatePendingPreviewDocuments(QmlJS::Document::Ptr doc);
    void showDebuggerTooltip(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos);
    void onEngineStateChanged(Debugger::DebuggerState state);

private:
    void showRoot();
    void resetViews();

    void initializeDocuments();
    void applyChangesToQmlInspectorHelper(bool applyChanges);
    void setupDockWidgets();
    QString filenameForShadowBuildFile(const QString &filename) const;
    void populateCrumblePath(const QmlDebugObjectReference &objRef);
    bool isRoot(const QmlDebugObjectReference &obj) const;
    QmlDebugObjectReference objectReferenceForLocation(const QString &fileName, int cursorPosition=-1) const;

    void connectSignals();
    void disconnectSignals();

    void showObject(const QmlDebugObjectReference &obj);

    QmlDebugObjectReference findParentRecursive(
            int goalDebugId, const QList<QmlDebugObjectReference > &objectsToSearch);
private:
    bool m_listeningToEditorManager;
    QmlJsInspectorToolBar *m_toolBar;
    ContextCrumblePath *m_crumblePath;
    QmlJSPropertyInspector *m_propertyInspector;

    InspectorSettings *m_settings;
    ClientProxy *m_clientProxy;
    quint32 m_debugQuery;
    quint32 m_showObjectQueryId;
    QList<quint32> m_updateObjectQueryIds;

    // Qml/JS integration
    QHash<QString, QmlJSLiveTextPreview *> m_textPreviews;
    QmlJS::Snapshot m_loadedSnapshot; //the snapshot loaded by the viewer

    QStringList m_pendingPreviewDocumentNames;
    Utils::FileInProjectFinder m_projectFinder;

    static InspectorUi *m_instance;
    bool m_selectionCallbackExpected;
    bool m_cursorPositionChangedExternally;
    bool m_onCrumblePathClicked;
};

} // Internal
} // QmlJSInspector

#endif // QMLJSINSPECTOR_H

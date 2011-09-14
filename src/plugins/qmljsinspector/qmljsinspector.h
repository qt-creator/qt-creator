/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QMLJSINSPECTOR_H
#define QMLJSINSPECTOR_H

#include "qmljsprivateapi.h"

#include <qmlprojectmanager/qmlprojectrunconfiguration.h>
#include <utils/fileinprojectfinder.h>

#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsastfwd_p.h>

#include <QtGui/QAction>
#include <QtCore/QObject>

QT_FORWARD_DECLARE_CLASS(QLineEdit)

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
    QObject *debuggerEngine() const;

signals:
    void statusMessage(const QString &text);
    void livePreviewActivated(bool isActivated);

public slots:
    void reloadQmlViewer();
    void serverReloaded();
    void setApplyChangesToQmlInspector(bool applyChanges);

private slots:
    void enable();
    void disable();
    void gotoObjectReferenceDefinition(const QDeclarativeDebugObjectReference &obj);
    void selectItems(const QList<QDeclarativeDebugObjectReference> &objectReferences);
    void selectItems(const QList<int> &objectIds);
    void changeSelectedItems(const QList<QDeclarativeDebugObjectReference> &objects);
    void changePropertyValue(int debugId,const QString &propertyName, const QString &valueExpression);
    void objectTreeReady();

    void updateEngineList();

    void removePreviewForEditor(Core::IEditor *newEditor);
    QmlJSLiveTextPreview *createPreviewForEditor(Core::IEditor *newEditor);

    void disableLivePreview();
    void crumblePathElementClicked(const QVariant &data);

    void updatePendingPreviewDocuments(QmlJS::Document::Ptr doc);
    void showDebuggerTooltip(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos);
    void debugQueryUpdated(QDeclarativeDebugQuery::State);

private:
    bool addQuotesForData(const QVariant &value) const;
    void resetViews();

    void initializeDocuments();
    void applyChangesToQmlInspectorHelper(bool applyChanges);
    void setupDockWidgets();
    QString filenameForShadowBuildFile(const QString &filename) const;
    void populateCrumblePath(const QDeclarativeDebugObjectReference &objRef);
    bool isRoot(const QDeclarativeDebugObjectReference &obj) const;
    QDeclarativeDebugObjectReference objectReferenceForLocation(const QString &fileName, int cursorPosition=-1) const;

    void connectSignals();
    void disconnectSignals();

private:
    bool m_listeningToEditorManager;
    QmlJsInspectorToolBar *m_toolBar;
    ContextCrumblePath *m_crumblePath;
    QLineEdit *m_filterExp;
    QmlJSPropertyInspector *m_propertyInspector;

    InspectorSettings *m_settings;
    ClientProxy *m_clientProxy;
    QObject *m_qmlEngine;
    QDeclarativeDebugExpressionQuery *m_debugQuery;

    // Qml/JS integration
    QHash<QString, QmlJSLiveTextPreview *> m_textPreviews;
    QmlJS::Snapshot m_loadedSnapshot; //the snapshot loaded by the viewer

    QStringList m_pendingPreviewDocumentNames;
    Utils::FileInProjectFinder m_projectFinder;

    static InspectorUi *m_instance;
    bool m_selectionCallbackExpected;
    bool m_cursorPositionChangedExternally;
};

} // Internal
} // QmlJSInspector

#endif // QMLJSINSPECTOR_H

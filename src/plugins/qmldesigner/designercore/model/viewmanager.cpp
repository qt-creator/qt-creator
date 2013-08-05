#include "viewmanager.h"

#include "componentaction.h"
#include "designeractionmanager.h"
#include "designmodewidget.h"
#include "crumblebar.h"

#include <qmldesigner/qmldesignerplugin.h>

#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtsupportconstants.h>

namespace QmlDesigner {


static CrumbleBar *crumbleBar() {
    return QmlDesignerPlugin::instance()->mainWidget()->crumbleBar();
}

ViewManager::ViewManager()
{
}

ViewManager::~ViewManager()
{
    foreach (const QWeakPointer<AbstractView> &view, m_additionalViews)
        delete view.data();
}

DesignDocument *ViewManager::currentDesignDocument() const
{
    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();
}

QString ViewManager::pathToQt() const
{
    QtSupport::BaseQtVersion *activeQtVersion = QtSupport::QtVersionManager::instance()->version(currentDesignDocument()->qtVersionId());
    if (activeQtVersion && (activeQtVersion->qtVersion() >= QtSupport::QtVersionNumber(4, 7, 1))
            && (activeQtVersion->type() == QLatin1String(QtSupport::Constants::DESKTOPQT)
                || activeQtVersion->type() == QLatin1String(QtSupport::Constants::SIMULATORQT)))
        return activeQtVersion->qmakeProperty("QT_INSTALL_DATA");

    return QString();
}

void ViewManager::attachNodeInstanceView()
{
    setNodeInstanceViewQtPath(pathToQt());
    currentModel()->setNodeInstanceView(&m_nodeInstanceView);
}

void ViewManager::attachRewriterView()
{
    if (currentDesignDocument()->rewriterView()) {
        currentModel()->setRewriterView(currentDesignDocument()->rewriterView());
        currentDesignDocument()->rewriterView()->reactivateTextMofifierChangeSignals();
    }
}

void ViewManager::detachRewriterView()
{
    if (currentDesignDocument()->rewriterView()) {
        currentDesignDocument()->rewriterView()->deactivateTextMofifierChangeSignals();
        currentModel()->setRewriterView(0);
    }
}

void ViewManager::switchStateEditorViewToBaseState()
{
    if (m_statesEditorView.isAttached()) {
        m_savedState = m_statesEditorView.currentState();
        m_statesEditorView.setCurrentState(m_statesEditorView.baseState());
    }
}

void ViewManager::switchStateEditorViewToSavedState()
{
    if (m_savedState.isValid() && m_statesEditorView.isAttached())
        m_statesEditorView.setCurrentState(m_savedState);
}

void ViewManager::resetPropertyEditorView()
{
    m_propertyEditorView.resetView();
}

void ViewManager::registerFormEditorToolTakingOwnership(AbstractCustomTool *tool)
{
    m_formEditorView.registerTool(tool);
}

void ViewManager::registerViewTakingOwnership(AbstractView *view)
{
    m_additionalViews.append(view);
}

void ViewManager::detachViewsExceptRewriterAndComponetView()
{
    switchStateEditorViewToBaseState();
    detachAdditionalViews();
    currentModel()->detachView(DesignerActionManager::view());
    currentModel()->detachView(&m_formEditorView);
    currentModel()->detachView(&m_navigatorView);
    currentModel()->detachView(&m_itemLibraryView);
    currentModel()->detachView(&m_statesEditorView);
    currentModel()->detachView(&m_propertyEditorView);
    if (m_debugView.isAttached())
        currentModel()->detachView(&m_debugView);
    currentModel()->setNodeInstanceView(0);
}

void ViewManager::attachItemLibraryView()
{
    setItemLibraryViewResourcePath(QFileInfo(currentDesignDocument()->fileName()).absolutePath());
    currentModel()->attachView(&m_itemLibraryView);
}

void ViewManager::attachAdditionalViews()
{
    foreach (const QWeakPointer<AbstractView> &view, m_additionalViews)
        currentModel()->attachView(view.data());
}

void ViewManager::detachAdditionalViews()
{
    foreach (const QWeakPointer<AbstractView> &view, m_additionalViews)
        currentModel()->detachView(view.data());
}

void ViewManager::attachComponentView()
{
    documentModel()->attachView(&m_componentView);
    QObject::connect(m_componentView.action(), SIGNAL(currentComponentChanged(ModelNode)), currentDesignDocument(), SLOT(changeToSubComponent(ModelNode)));
}

void ViewManager::detachComponentView()
{
    QObject::disconnect(m_componentView.action(), SIGNAL(currentComponentChanged(ModelNode)), currentDesignDocument(), SLOT(changeToSubComponent(ModelNode)));
    documentModel()->detachView(&m_componentView);
}

void ViewManager::attachViewsExceptRewriterAndComponetView()
{
    if (QmlDesignerPlugin::instance()->settings().enableDebugView)
        currentModel()->attachView(&m_debugView);
    attachNodeInstanceView();
    currentModel()->attachView(&m_formEditorView);
    currentModel()->attachView(&m_navigatorView);
    attachItemLibraryView();
    currentModel()->attachView(&m_statesEditorView);
    currentModel()->attachView(&m_propertyEditorView);
    currentModel()->attachView(DesignerActionManager::view());
    attachAdditionalViews();
    switchStateEditorViewToSavedState();
}

void ViewManager::setItemLibraryViewResourcePath(const QString &resourcePath)
{
    m_itemLibraryView.setResourcePath(resourcePath);
}

void ViewManager::setComponentNode(const ModelNode &componentNode)
{
    m_componentView.setComponentNode(componentNode);
}

void ViewManager::setNodeInstanceViewQtPath(const QString &qtPath)
{
    m_nodeInstanceView.setPathToQt(qtPath);
}

static bool widgetInfoLessThan(const WidgetInfo &firstWidgetInfo, const WidgetInfo &secondWidgetInfo)
{
    return firstWidgetInfo.placementPriority < secondWidgetInfo.placementPriority;
}

QList<WidgetInfo> ViewManager::widgetInfos()
{
    QList<WidgetInfo> widgetInfoList;

    widgetInfoList.append(m_formEditorView.widgetInfo());
    widgetInfoList.append(m_itemLibraryView.widgetInfo());
    widgetInfoList.append(m_navigatorView.widgetInfo());
    widgetInfoList.append(m_propertyEditorView.widgetInfo());
    widgetInfoList.append(m_statesEditorView.widgetInfo());
    if (m_debugView.hasWidget())
        widgetInfoList.append(m_debugView.widgetInfo());

    foreach (const QWeakPointer<AbstractView> &abstractView, m_additionalViews) {
        if (abstractView && abstractView->hasWidget())
            widgetInfoList.append(abstractView->widgetInfo());
    }

    qSort(widgetInfoList.begin(), widgetInfoList.end(), widgetInfoLessThan);

    return widgetInfoList;
}

void ViewManager::disableWidgets()
{
    foreach (const WidgetInfo &widgetInfo, widgetInfos())
        widgetInfo.widget->setEnabled(false);
}

void ViewManager::enableWidgets()
{
    foreach (const WidgetInfo &widgetInfo, widgetInfos())
        widgetInfo.widget->setEnabled(true);
}

void ViewManager::pushFileOnCrumbleBar(const QString &fileName)
{
    crumbleBar()->pushFile(fileName);
}

void ViewManager::pushInFileComponentOnCrambleBar(const QString &componentId)

{
    crumbleBar()->pushInFileComponent(componentId);
}

void ViewManager::nextFileIsCalledInternally()
{
    crumbleBar()->nextFileIsCalledInternally();
}

NodeInstanceView *ViewManager::nodeInstanceView()
{
    return &m_nodeInstanceView;
}

QWidgetAction *ViewManager::componentViewAction()
{
    return m_componentView.action();
}

Model *ViewManager::currentModel() const
{
    return currentDesignDocument()->currentModel();
}

Model *ViewManager::documentModel() const
{
    return currentDesignDocument()->documentModel();
}

} // namespace QmlDesigner

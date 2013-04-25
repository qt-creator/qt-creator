#include "viewmanager.h"

#include "componentaction.h"
#include "formeditorwidget.h"
#include "toolbox.h"
#include "designeractionmanager.h"

#include <qmldesigner/qmldesignerplugin.h>

#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtsupportconstants.h>

namespace QmlDesigner {

ViewManager::ViewManager()
{
    //QObject::connect(&m_nodeInstanceView, SIGNAL(qmlPuppetCrashed()), designModeWidget, SLOT(qmlPuppetCrashed()));
    //QObject::connect(m_formEditorView.crumblePath(), SIGNAL(elementClicked(QVariant)), designModeWidget, SLOT(onCrumblePathElementClicked(QVariant)));
    m_formEditorView.formEditorWidget()->toolBox()->addLeftSideAction(m_componentView.action()); // ugly hack
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

void ViewManager::attachRewriterView(TextModifier *textModifier)
{
    if (currentDesignDocument()->rewriterView()) {
        currentDesignDocument()->rewriterView()->setTextModifier(textModifier);
        currentDesignDocument()->rewriterView()->reactivateTextMofifierChangeSignals();
        currentModel()->setRewriterView(currentDesignDocument()->rewriterView());
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

void ViewManager::pushFileOnCrambleBar(const QString &fileName)
{
    m_formEditorView.formEditorWidget()->formEditorCrumbleBar()->pushFile(fileName);
}

void ViewManager::pushInFileComponentOnCrambleBar(const QString &componentId)

{
    m_formEditorView.formEditorWidget()->formEditorCrumbleBar()->pushInFileComponent(componentId);
}

void ViewManager::nextFileIsCalledInternally()
{
    m_formEditorView.formEditorWidget()->formEditorCrumbleBar()->nextFileIsCalledInternally();
}

 QmlModelView *ViewManager::qmlModelView()
 {
     return &m_formEditorView;
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

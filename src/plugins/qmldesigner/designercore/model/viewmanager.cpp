#include "viewmanager.h"

#include "designdocument.h"
#include "componentaction.h"
#include "itemlibrarywidget.h"
#include "designmodewidget.h"
#include "formeditorwidget.h"
#include "toolbox.h"
#include "designeractionmanager.h"

#include <qmldesigner/qmldesignerplugin.h>

#include <utils/crumblepath.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtsupportconstants.h>

namespace QmlDesigner {

ViewManager::ViewManager()
{
    //QObject::connect(&m_nodeInstanceView, SIGNAL(qmlPuppetCrashed()), designModeWidget, SLOT(qmlPuppetCrashed()));
    //QObject::connect(m_formEditorView.crumblePath(), SIGNAL(elementClicked(QVariant)), designModeWidget, SLOT(onCrumblePathElementClicked(QVariant)));
    m_formEditorView.formEditorWidget()->toolBox()->addLeftSideAction(m_componentView.action()); // ugly hack
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

void ViewManager::detachViewsExceptRewriterAndComponetView()
{
    switchStateEditorViewToBaseState();
    currentModel()->detachView(DesignerActionManager::view());
    currentModel()->detachView(&m_formEditorView);
    currentModel()->detachView(&m_navigatorView);
    currentModel()->detachView(&m_itemLibraryView);
    currentModel()->detachView(&m_statesEditorView);
    currentModel()->detachView(&m_propertyEditorView);
    currentModel()->setNodeInstanceView(0);

    static bool enableViewLogger = !qgetenv("QTC_ENABLE_QMLDESIGNER_LOGGER").isEmpty();
    if (enableViewLogger)
        currentModel()->detachView(&m_viewLogger);
}

void ViewManager::attachItemLibraryView()
{
    setItemLibraryViewResourcePath(QFileInfo(currentDesignDocument()->fileName()).absolutePath());
    currentModel()->attachView(&m_itemLibraryView);
}

void ViewManager::attachComponentView()
{
    documentModel()->attachView(&m_componentView);
    QObject::connect(m_componentView.action(), SIGNAL(currentComponentChanged(ModelNode)), currentDesignDocument(), SLOT(changeCurrentModelTo(ModelNode)));
}

void ViewManager::detachComponentView()
{
    QObject::disconnect(m_componentView.action(), SIGNAL(currentComponentChanged(ModelNode)), currentDesignDocument(), SLOT(changeCurrentModelTo(ModelNode)));
    documentModel()->detachView(&m_componentView);
}

void ViewManager::attachViewsExceptRewriterAndComponetView()
{
    static bool enableViewLogger = !qgetenv("QTC_ENABLE_QMLDESIGNER_LOGGER").isEmpty();
    if (enableViewLogger)
        currentModel()->attachView(&m_viewLogger);

    attachNodeInstanceView();
    currentModel()->attachView(&m_formEditorView);
    currentModel()->attachView(&m_navigatorView);
    attachItemLibraryView();
    currentModel()->attachView(&m_statesEditorView);
    currentModel()->attachView(&m_propertyEditorView);
    currentModel()->attachView(DesignerActionManager::view());
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

QWidget *ViewManager::formEditorWidget()
{
    return m_formEditorView.widget();
}

QWidget *ViewManager::propertyEditorWidget()
{
    return m_propertyEditorView.widget();
}

QWidget *ViewManager::itemLibraryWidget()
{
    return m_itemLibraryView.widget();
}

QWidget *ViewManager::navigatorWidget()
{
    return m_navigatorView.widget();
}

QWidget *ViewManager::statesEditorWidget()
{
    return m_statesEditorView.widget();
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

Model *ViewManager::currentModel() const
{
    return currentDesignDocument()->currentModel();
}

Model *ViewManager::documentModel() const
{
    return currentDesignDocument()->documentModel();
}

} // namespace QmlDesigner

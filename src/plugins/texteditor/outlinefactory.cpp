#include "outlinefactory.h"
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <QVBoxLayout>
#include <QDebug>
#include <QToolButton>
#include <QLabel>
#include <QStackedWidget>

namespace TextEditor {
namespace Internal {

OutlineWidgetStack::OutlineWidgetStack(OutlineFactory *factory) :
    QStackedWidget(),
    m_factory(factory),
    m_syncWithEditor(true)
{
    QLabel *label = new QLabel(tr("No outline available"), this);
    label->setAlignment(Qt::AlignCenter);
    addWidget(label);

    m_toggleSync = new QToolButton;
    m_toggleSync->setIcon(QIcon(":/core/images/linkicon.png"));
    m_toggleSync->setCheckable(true);
    m_toggleSync->setChecked(true);
    m_toggleSync->setToolTip(tr("Synchronize with Editor"));
    connect(m_toggleSync, SIGNAL(clicked(bool)), this, SLOT(toggleCursorSynchronization()));

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(updateCurrentEditor(Core::IEditor*)));
    updateCurrentEditor(editorManager->currentEditor());
}

OutlineWidgetStack::~OutlineWidgetStack()
{
}

QToolButton *OutlineWidgetStack::toggleSyncButton()
{
    return m_toggleSync;
}

bool OutlineWidgetStack::isCursorSynchronized() const
{
    return m_syncWithEditor;
}

void OutlineWidgetStack::toggleCursorSynchronization()
{
    m_syncWithEditor = !m_syncWithEditor;
    if (IOutlineWidget *outlineWidget = qobject_cast<IOutlineWidget*>(currentWidget()))
        outlineWidget->setCursorSynchronization(m_syncWithEditor);
}

void OutlineWidgetStack::updateCurrentEditor(Core::IEditor *editor)
{
    IOutlineWidget *newWidget = 0;

    if (editor) {
        foreach (IOutlineWidgetFactory *widgetFactory, m_factory->widgetFactories()) {
            if (widgetFactory->supportsEditor(editor)) {
                newWidget = widgetFactory->createWidget(editor);
                break;
            }
        }
    }

    if (newWidget != currentWidget()) {
        // delete old widget
        if (IOutlineWidget *outlineWidget = qobject_cast<IOutlineWidget*>(currentWidget())) {
            removeWidget(outlineWidget);
            delete outlineWidget;
        }
        if (newWidget) {
            newWidget->setCursorSynchronization(m_syncWithEditor);
            addWidget(newWidget);
            setCurrentWidget(newWidget);
        }
    }
}

OutlineFactory::OutlineFactory() :
    Core::INavigationWidgetFactory()
{
}

QList<IOutlineWidgetFactory*> OutlineFactory::widgetFactories() const
{
    return m_factories;
}

void OutlineFactory::setWidgetFactories(QList<IOutlineWidgetFactory*> factories)
{
    m_factories = factories;
}

QString OutlineFactory::displayName() const
{
    return tr("Outline");
}

QString OutlineFactory::id() const
{
    return QLatin1String("Outline");
}

QKeySequence OutlineFactory::activationSequence() const
{
    return QKeySequence();
}

Core::NavigationView OutlineFactory::createWidget()
{
    Core::NavigationView n;
    OutlineWidgetStack *placeHolder = new OutlineWidgetStack(this);
    n.widget = placeHolder;
    n.dockToolBarWidgets.append(placeHolder->toggleSyncButton());
    return n;
}

} // namespace Internal
} // namespace TextEditor

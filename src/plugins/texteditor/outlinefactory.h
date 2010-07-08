#ifndef OUTLINE_H
#define OUTLINE_H

#include <texteditor/ioutlinewidget.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <QtGui/QStackedWidget>

namespace Core {
class IEditor;
}

namespace TextEditor {
namespace Internal {

class OutlineFactory;

class OutlineWidgetStack : public QStackedWidget
{
    Q_OBJECT
public:
    OutlineWidgetStack(OutlineFactory *factory);
    ~OutlineWidgetStack();

    QToolButton *toggleSyncButton();

private:
    bool isCursorSynchronized() const;
    QWidget *dummyWidget() const;

private slots:
    void toggleCursorSynchronization();
    void updateCurrentEditor(Core::IEditor *editor);

private:
    QStackedWidget *m_widgetStack;
    OutlineFactory *m_factory;
    QToolButton *m_toggleSync;
    bool m_syncWithEditor;
};

class OutlineFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
public:
    OutlineFactory();

    QList<IOutlineWidgetFactory*> widgetFactories() const;
    void setWidgetFactories(QList<IOutlineWidgetFactory*> factories);

    // from INavigationWidgetFactory
    virtual QString displayName() const;
    virtual QString id() const;
    virtual QKeySequence activationSequence() const;
    virtual Core::NavigationView createWidget();

private:
    QList<IOutlineWidgetFactory*> m_factories;
};

} // namespace Internal
} // namespace TextEditor

#endif // OUTLINE_H

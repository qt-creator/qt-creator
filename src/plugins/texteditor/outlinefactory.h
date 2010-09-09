#ifndef OUTLINE_H
#define OUTLINE_H

#include <texteditor/ioutlinewidget.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <QtGui/QStackedWidget>
#include <QtGui/QMenu>

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
    QToolButton *filterButton();

    void saveSettings(int position);
    void restoreSettings(int position);

private:
    bool isCursorSynchronized() const;
    QWidget *dummyWidget() const;
    void updateFilterMenu();

private slots:
    void toggleCursorSynchronization();
    void updateCurrentEditor(Core::IEditor *editor);

private:
    QStackedWidget *m_widgetStack;
    OutlineFactory *m_factory;
    QToolButton *m_toggleSync;
    QToolButton *m_filterButton;
    QMenu *m_filterMenu;
    bool m_syncWithEditor;
    int m_position;
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
    virtual int priority() const;
    virtual QString id() const;
    virtual QKeySequence activationSequence() const;
    virtual Core::NavigationView createWidget();
    virtual void saveSettings(int position, QWidget *widget);
    virtual void restoreSettings(int position, QWidget *widget);
private:
    QList<IOutlineWidgetFactory*> m_factories;
};

} // namespace Internal
} // namespace TextEditor

#endif // OUTLINE_H

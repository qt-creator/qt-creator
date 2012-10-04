#ifndef QMLCONSOLEPANE_H
#define QMLCONSOLEPANE_H

#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QToolButton;
class QLabel;
QT_END_NAMESPACE

namespace Utils {
class SavedAction;
}

namespace QmlJSTools {

namespace Internal {

class QmlConsoleView;
class QmlConsoleItemDelegate;
class QmlConsoleProxyModel;
class QmlConsoleItemModel;

class QmlConsolePane : public Core::IOutputPane
{
    Q_OBJECT
public:
    QmlConsolePane(QObject *parent);
    ~QmlConsolePane();

    QWidget *outputWidget(QWidget *);
    QList<QWidget *> toolBarWidgets() const;
    QString displayName() const { return tr("Console"); }
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);
    bool canFocus() const;
    bool hasFocus() const;
    void setFocus();

    bool canNext() const;
    bool canPrevious() const;
    void goToNext();
    void goToPrev();
    bool canNavigate() const;

    void readSettings();
    void setContext(const QString &context);

public slots:
    void writeSettings() const;

private:
    QToolButton *m_showDebugButton;
    QToolButton *m_showWarningButton;
    QToolButton *m_showErrorButton;
    Utils::SavedAction *m_showDebugButtonAction;
    Utils::SavedAction *m_showWarningButtonAction;
    Utils::SavedAction *m_showErrorButtonAction;
    QWidget *m_spacer;
    QLabel *m_statusLabel;
    QmlConsoleView *m_consoleView;
    QmlConsoleItemDelegate *m_itemDelegate;
    QmlConsoleProxyModel *m_proxyModel;
    QWidget *m_consoleWidget;
};

} // namespace Internal
} // namespace QmlJSTools

#endif // QMLCONSOLEPANE_H

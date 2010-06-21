#ifndef HTMLEDITOR_H
#define HTMLEDITOR_H

#include "coreplugin/editormanager/ieditor.h"

#include <QToolBar>

class HTMLEditorWidget;

struct HTMLEditorData;
class HTMLEditor : public Core::IEditor
{
    Q_OBJECT

public:
    HTMLEditor(HTMLEditorWidget* editorWidget);
    ~HTMLEditor();

    bool createNew(const QString& contents = QString());
    QString displayName() const;
    IEditor* duplicate(QWidget* parent);
    bool duplicateSupported() const;
    Core::IFile* file();
    bool isTemporary() const;
    const char* kind() const;
    bool open(const QString& fileName = QString()) ;
    bool restoreState(const QByteArray& state);
    QByteArray saveState() const;
    void setDisplayName(const QString &title);
    QToolBar* toolBar();

    // From Core::IContext
    QWidget* widget();
    QList<int> context() const;

protected slots:
    void slotTitleChanged(const QString& title)
    { setDisplayName(title); }

private:
    HTMLEditorData* d;
};

#endif // HTMLEDITOR_H

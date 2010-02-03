/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLHOVERHANDLER_H
#define QMLHOVERHANDLER_H

#include "qmlmodelmanagerinterface.h"

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QHelpEngineCore;
class QPoint;
class QStringList;
QT_END_NAMESPACE

namespace Core {
class IEditor;
}

namespace QmlJS {
    namespace Interpreter {
        class Engine;
        class Context;
        class Value;
    }
}

namespace TextEditor {
class ITextEditor;
}

namespace QmlJSEditor {
namespace Internal {

class QmlHoverHandler : public QObject
{
    Q_OBJECT

public:
    QmlHoverHandler(QObject *parent = 0);

public slots:
    void showToolTip(TextEditor::ITextEditor *editor, const QPoint &point, int pos);
    void updateContextHelpId(TextEditor::ITextEditor *editor, int pos);

private slots:
    void editorOpened(Core::IEditor *editor);

private:
    void updateHelpIdAndTooltip(TextEditor::ITextEditor *editor, int pos);
    QString prettyPrint(const QmlJS::Interpreter::Value *value, QmlJS::Interpreter::Context *context,
                        QStringList *baseClasses) const;

private:
    QmlModelManagerInterface *m_modelManager;
    QHelpEngineCore *m_helpEngine;
    QString m_helpId;
    QString m_toolTip;
    bool m_helpEngineNeedsSetup;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLHOVERHANDLER_H

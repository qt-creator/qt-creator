/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLJSHOVERHANDLER_H
#define QMLJSHOVERHANDLER_H

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljslookupcontext.h>

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtGui/QColor>

QT_BEGIN_NAMESPACE
class QPoint;
QT_END_NAMESPACE

namespace Core {
class IEditor;
}

namespace TextEditor {
class ITextEditor;
}

namespace QmlJSEditor {
namespace Internal {

class SemanticInfo;
class QmlJSTextEditor;

class HoverHandler : public QObject
{
    Q_OBJECT

public:
    HoverHandler(QObject *parent = 0);

public slots:
    void showToolTip(TextEditor::ITextEditor *editor, const QPoint &point, int pos);
    void updateContextHelpId(TextEditor::ITextEditor *editor, int pos);

private slots:
    void editorOpened(Core::IEditor *editor);

private:
    void resetMatchings();
    void identifyMatch(TextEditor::ITextEditor *editor, int pos);
    bool matchDiagnosticMessage(QmlJSTextEditor *qmlEditor, int pos);
    bool matchColorItem(const QmlJS::LookupContext::Ptr &lookupContext,
                        const QmlJS::Document::Ptr &qmlDocument,
                        const QList<QmlJS::AST::Node *> &astPath,
                        unsigned pos);
    void handleOrdinaryMatch(const QmlJS::LookupContext::Ptr &lookupContext,
                             QmlJS::AST::Node *node);

    void evaluateHelpCandidates();

    QString prettyPrint(const QmlJS::Interpreter::Value *value,
                        QmlJS::Interpreter::Context *context);

    QmlJS::ModelManagerInterface *m_modelManager;
    int m_matchingHelpCandidate;
    QStringList m_helpCandidates;
    QString m_toolTip;
    QColor m_colorTip;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLJSHOVERHANDLER_H

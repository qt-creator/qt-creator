/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CPPHOVERHANDLER_H
#define CPPHOVERHANDLER_H

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QHelpEngineCore;
class QPoint;
QT_END_NAMESPACE

namespace Core {
class IEditor;
}

namespace CppTools {
class CppModelManagerInterface;
}

namespace TextEditor {
class ITextEditor;
}

namespace CppEditor {
namespace Internal {

class CppHoverHandler : public QObject
{
    Q_OBJECT

public:
    CppHoverHandler(QObject *parent = 0);

public slots:
    void showToolTip(TextEditor::ITextEditor *editor, const QPoint &point, int pos);
    void updateContextHelpId(TextEditor::ITextEditor *editor, int pos);

private slots:
    void editorOpened(Core::IEditor *editor);

private:
    void updateHelpIdAndTooltip(TextEditor::ITextEditor *editor, int pos);

    CppTools::CppModelManagerInterface *m_modelManager;
    QHelpEngineCore *m_helpEngine;
    QString m_helpId;
    QString m_toolTip;
    bool m_helpEngineNeedsSetup;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPHOVERHANDLER_H

/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef CPPHOVERHANDLER_H
#define CPPHOVERHANDLER_H

#include <QtCore/QObject>
#include <QtCore/QPoint>

QT_BEGIN_NAMESPACE
class QHelpEngineCore;
QT_END_NAMESPACE

namespace TextEditor {
class ITextEditor;
}

namespace CppTools {
namespace Internal {

class CppModelManager;

class CppHoverHandler : public QObject
{
    Q_OBJECT

public:
    CppHoverHandler(CppModelManager *manager, QObject *parent);

public slots:
    void showToolTip(TextEditor::ITextEditor *editor, const QPoint &point, int pos);
    void updateContextHelpId(TextEditor::ITextEditor *editor, int pos);

private:
    void updateHelpIdAndTooltip(TextEditor::ITextEditor *editor, int pos);

    CppModelManager *m_manager;
    QHelpEngineCore *m_helpEngine;
    QString m_helpId;
    QString m_toolTip;
    bool m_helpEngineNeedsSetup;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPHOVERHANDLER_H

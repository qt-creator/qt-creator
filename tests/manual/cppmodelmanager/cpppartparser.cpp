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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include <QtCore/QDebug>

#include "cpppartparser.h"
#include "cppcodemodel.h"

#include "preprocessor.h"
#include "pp-stream.h"
#include "pp-engine.h"

#include "parser.h"
#include "control.h"

#include "binder.h"

CppPartParser *CppPartParser::m_instance = 0;

// ****************************
// CppStream
// ****************************

class CppStream : public Stream
{
public:
    CppStream(QByteArray *array);
    virtual ~CppStream();

private:
    QByteArray *m_array;
};

CppStream::CppStream(QByteArray *array)
    : Stream(array)
{
    m_array = array;
}

CppStream::~CppStream()
{
    delete m_array;
}

// ****************************
// CppPartPP
// ****************************

class CppPartPP : private Preprocessor
{
public:
    QByteArray process(QString &fileName, CppCodeModel *model);

protected:
    Stream* sourceNeeded(QString& fileName, IncludeType type);

private:
    CppCodeModel *m_model;
    pp *m_proc;
};

QByteArray CppPartPP::process(QString &fileName, CppCodeModel *model)
{
    QByteArray result;

    m_model = model;
    pp proc(this, (*model->macros()));
    m_proc = &proc;
    if (QByteArray *contents = m_model->contents(fileName)) {
        result = proc.processFile(*(contents), fileName);
        delete contents;
    }

    return result;
}

Stream* CppPartPP::sourceNeeded(QString& fileName, IncludeType type)
{
    QString localfile;
    if (type == IncludeLocal)
        localfile = m_proc->currentfile();

    QByteArray *contents = m_model->contents(fileName, localfile);
    if (!contents)
        return 0;

    return new CppStream(contents);
}

// ****************************
// CppPartParser
// ****************************

CppPartParser::CppPartParser(QObject *parent)
    : QThread(parent)
{
    m_cppPartPP = new CppPartPP();

    m_cancelParsing = false;
    m_currentModel = 0;
}

CppPartParser::~CppPartParser()
{
    delete m_cppPartPP;    
}

CppPartParser *CppPartParser::instance(QObject *parent)
{
    if (!m_instance)
        m_instance = new CppPartParser(parent);

    return m_instance;
}

void CppPartParser::parse(CppCodeModel *model)
{
    mutex.lock();
    if (!m_modelQueue.contains(model))
        m_modelQueue.enqueue(model);
    mutex.unlock();

    if (!isRunning()) {
        m_cancelParsing = false;
        start();
        setPriority(QThread::LowPriority);
    }
}

void CppPartParser::remove(CppCodeModel *model)
{
    mutex.lock();
    if (m_modelQueue.contains(model))
        m_modelQueue.removeAll(model);

    if (m_currentModel == model) {
        m_cancelParsing = true;
        mutex.unlock();
        wait();
        m_cancelParsing = false;
        start();
        setPriority(QThread::LowPriority);
    } else {
        mutex.unlock();
    }
}

void CppPartParser::run()
{
    while (!m_cancelParsing && !m_modelQueue.isEmpty()) {
        mutex.lock();
        m_currentModel = m_modelQueue.dequeue();
        mutex.unlock();

        QStringList files = m_currentModel->needsParsing();
        for (int i=0; i<files.count() && !m_cancelParsing; ++i) {
            QString resolvedName = files.at(i);
            QByteArray ppcontent = m_cppPartPP->process(
                resolvedName, m_currentModel);

            Control control;
            Parser p(&control);
            pool __pool;

            TranslationUnitAST *ast = p.parse(ppcontent, ppcontent.size(), &__pool);
            qDebug() << p.problemCount();

            Binder binder(m_currentModel, p.location());
            binder.run(ast, resolvedName);

            m_currentModel->store();
        }
    }
}


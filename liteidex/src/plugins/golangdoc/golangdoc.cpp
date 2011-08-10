/**************************************************************************
** This file is part of LiteIDE
**
** Copyright (c) 2011 LiteIDE Team. All rights reserved.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** In addition, as a special exception,  that plugins developed for LiteIDE,
** are allowed to remain closed sourced and can be distributed under any license .
** These rights are included in the file LGPL_EXCEPTION.txt in this package.
**
**************************************************************************/
// Module: golangdoc.cpp
// Creator: visualfc <visualfc@gmail.com>
// date: 2011-7-7
// $Id: golangdoc.cpp,v 1.0 2011-7-7 visualfc Exp $

#include "golangdoc.h"
#include "liteapi/litefindobj.h"
#include "litebuildapi/litebuildapi.h"
#include "processex/processex.h"
#include "fileutil/fileutil.h"
#include "documentbrowser/documentbrowser.h"

#include <QListView>
#include <QStringListModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QToolBar>
#include <QStatusBar>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QFile>
#include <QDir>
#include <QTextBrowser>
#include <QUrl>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QGroupBox>
#include <QToolButton>
#include <QTextCodec>
#include <QDesktopServices>
#include <QDomDocument>
#include <QDebug>

//lite_memory_check_begin
#if defined(WIN32) && defined(_MSC_VER) &&  defined(_DEBUG)
     #define _CRTDBG_MAP_ALLOC
     #include <stdlib.h>
     #include <crtdbg.h>
     #define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
     #define new DEBUG_NEW
#endif
//lite_memory_check_end


static QString docToNavdoc(const QString &data, QString &header, QString &nav)
{
    QDomDocument doc;
    QStringList srcLines = data.split("\n");
    QStringList navLines;
    QStringList dstLines;
    navLines.append("<table class=\"unruled\"><tbody><tr><td class=\"first\"><dl>");
    int index = 0;
    if (srcLines.length() >= 1) {
        //<!-- How to Write Go Code -->
        QString line = srcLines.at(0);
        int p1 = line.indexOf("<!--");
        int p2 = line.lastIndexOf("-->");
        if (p1 != -1 && p2 != -2) {
            QString title = line.mid(p1+4,p2-p1-4).trimmed();
            if (title.indexOf("title ") == 0) {
                header = QString("<h1>%1</h1>").arg(title.right(title.length()-6));
            } else {
                header = QString("<h1>%1</h1>").arg(title);
            }
        }
    }
    foreach(QString source, srcLines) {
        QString line = source.trimmed();
        index++;
        if (line.length() >= 10) {
            if (line.left(3) == "<h2") {
                if (doc.setContent(line)) {
                    QDomElement e = doc.firstChildElement("h2");
                    if (!e.isNull()) {
                        QString text = e.text();
                        QString id = e.attribute("id");
                        if (id.isEmpty()) {
                            id = QString("tmp_%1").arg(index);
                            e.setAttribute("id",id);
                        }
                        //<span class="navtop"><a href="#top">[Top]</a></span>
                        QDomElement span = doc.createElement("span");
                        span.setAttribute("class","navtop");
                        QDomElement a = doc.createElement("a");
                        a.setAttribute("href","#top");
                        QDomText top = doc.createTextNode("[Top]");
                        a.appendChild(top);
                        span.appendChild(a);
                        e.appendChild(span);

                        source = doc.toString();
                        navLines << QString("<dt><a href=\"#%1\">%2</a></dt>").arg(id).arg(text);
                    }
                }
            }
            else if (line.left(3) == "<h3") {
                if (doc.setContent(line)) {
                    QDomElement e = doc.firstChildElement("h3");
                    if (!e.isNull()) {
                        QString text = e.text();
                        QString id = e.attribute("id");
                        if (id.isEmpty()) {
                            id = QString("tmp_%1").arg(index);
                            e.setAttribute("id",id);
                        }
                        source = doc.toString();
                        navLines << QString("<dd><a href=\"#%1\">%2</a></dd>").arg(id).arg(text);
                    }
                }
           }
        }
        dstLines.append(source);
    }
    navLines.append("</dl></td><td><dl></dl></td><tr></tbody></table>");
    nav = navLines.join("");
    return dstLines.join("\n");
}


GolangDoc::GolangDoc(LiteApi::IApplication *app, QObject *parent) :
    QObject(parent),
    m_liteApp(app)
{
    m_findProcess = new ProcessEx(this);
    m_godocProcess = new ProcessEx(this);

    m_widget = new QWidget;
    m_findResultModel = new QStringListModel(this);

    m_findResultListView = new QListView;
    m_findResultListView->setEditTriggers(0);
    m_findResultListView->setModel(m_findResultModel);

    m_findComboBox = new QComboBox;
    m_findComboBox->setEditable(true);
    m_findComboBox->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    m_findComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);

    m_findAct = new QAction(tr("Find"),this);
    m_listPkgAct = new QAction(tr("List \"src/pkg\""),this);
    m_listCmdAct = new QAction(tr("List \"src/cmd\""),this);

    m_findMenu = new QMenu(tr("Find"));
    m_findMenu->addAction(m_findAct);
    m_findMenu->addSeparator();
    m_findMenu->addAction(m_listPkgAct);
    m_findMenu->addAction(m_listCmdAct);

    QToolButton *findBtn = new QToolButton;
    findBtn->setPopupMode(QToolButton::MenuButtonPopup);
    findBtn->setDefaultAction(m_findAct);
    findBtn->setMenu(m_findMenu);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setMargin(4);

    QHBoxLayout *findLayout = new QHBoxLayout;
    findLayout->setMargin(0);
    findLayout->addWidget(m_findComboBox);
    findLayout->addWidget(findBtn);

    mainLayout->addLayout(findLayout);
    mainLayout->addWidget(m_findResultListView);
    m_widget->setLayout(mainLayout);

    m_liteApp->dockManager()->addDock(m_widget,tr("GolangDoc"),Qt::LeftDockWidgetArea);

    m_docBrowser = new DocumentBrowser(m_liteApp,this);
    m_docBrowser->setName(tr("GodocBrowser"));
    m_docBrowser->setSearchPaths(QStringList() << m_liteApp->resourcePath()+"/golangdoc");

    m_rootLabel = new QLabel;
    m_docBrowser->statusBar()->addPermanentWidget(m_rootLabel);

    m_browserAct = m_liteApp->editorManager()->registerBrowser(m_docBrowser);
    QMenu *menu = m_liteApp->actionManager()->loadMenu("view");
    if (menu) {
        menu->addAction(m_browserAct);
    }

    QString path = m_liteApp->resourcePath()+"/golangdoc/godoc.html";
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        m_templateData = file.readAll();
        file.close();
    }

    connect(m_docBrowser,SIGNAL(requestUrl(QUrl)),this,SLOT(openUrl(QUrl)));
    connect(m_findComboBox,SIGNAL(activated(QString)),this,SLOT(findPackage(QString)));
    connect(m_godocProcess,SIGNAL(extOutput(QByteArray,bool)),this,SLOT(godocOutput(QByteArray,bool)));
    connect(m_godocProcess,SIGNAL(extFinish(bool,int,QString)),this,SLOT(godocFinish(bool,int,QString)));
    connect(m_findProcess,SIGNAL(extOutput(QByteArray,bool)),this,SLOT(findOutput(QByteArray,bool)));
    connect(m_findProcess,SIGNAL(extFinish(bool,int,QString)),this,SLOT(findFinish(bool,int,QString)));
    connect(m_findResultListView,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(doubleClickListView(QModelIndex)));
    connect(m_findAct,SIGNAL(triggered()),this,SLOT(findPackage()));
    connect(m_listPkgAct,SIGNAL(triggered()),this,SLOT(listPkg()));
    connect(m_listCmdAct,SIGNAL(triggered()),this,SLOT(listCmd()));

    m_envManager = LiteApi::findExtensionObject<LiteApi::IEnvManager*>(m_liteApp,"LiteApi.IEnvManager");
    if (m_envManager) {
        connect(m_envManager,SIGNAL(currentEnvChanged(LiteApi::IEnv*)),this,SLOT(currentEnvChanged(LiteApi::IEnv*)));
        currentEnvChanged(m_envManager->currentEnv());
    }
    m_lastNav = true;
    openUrl(QUrl("/doc/docs.html"));
}

GolangDoc::~GolangDoc()
{
    m_liteApp->settings()->setValue("golangdoc/goroot",m_goroot);
    if (m_docBrowser) {
        delete m_docBrowser;
    }
    delete m_findMenu;
    delete m_widget;
}

void GolangDoc::currentEnvChanged(LiteApi::IEnv*)
{
    QProcessEnvironment env = m_envManager->currentEnvironment();
    QString goroot = env.value("GOROOT");
    QString gobin = env.value("GOBIN");
    if (!goroot.isEmpty() && gobin.isEmpty()) {
        gobin = goroot+"/bin";
    }
    QString godoc = FileUtil::findExecute(gobin+"/godoc");
    if (godoc.isEmpty()) {
        godoc = FileUtil::lookPath("godoc",env,true);
    }
    QString find = FileUtil::findExecute(m_liteApp->applicationPath()+"/godocview");
    if (find.isEmpty()) {
        find = FileUtil::lookPath("godocview",env,true);
    }

    m_goroot = goroot;
    m_godocCmd = godoc;
    m_findCmd = find;
    m_rootLabel->setText("GOROOT="+goroot);

    m_findProcess->setEnvironment(env.toStringList());
    m_godocProcess->setEnvironment(env.toStringList());
}

void GolangDoc::activeBrowser()
{
    m_liteApp->editorManager()->activeBrowser(m_docBrowser);
}

void GolangDoc::listPkg()
{
    if (m_findCmd.isEmpty()) {
        return;
    }
    QStringList args;
    args << "-mode=lite" << "-list=src/pkg";
    m_findData.clear();
    m_findProcess->start(m_findCmd,args);
}

void GolangDoc::listCmd()
{
    if (m_findCmd.isEmpty()) {
        return;
    }
    QStringList args;
    args << "-mode=lite" << "-list=src/cmd";
    m_findData.clear();
    m_findProcess->start(m_findCmd,args);
}


void GolangDoc::findPackage(QString pkgname)
{
    if (pkgname.isEmpty()) {
        pkgname = m_findComboBox->currentText();
    }
    if (pkgname.isEmpty()) {
        return;
    }
    if (m_findCmd.isEmpty()) {
        return;
    }
    QStringList args;
    args << "-mode=lite" << "-find" << pkgname;
    m_findData.clear();
    m_findProcess->start(m_findCmd,args);
}

void GolangDoc::findOutput(QByteArray data,bool bStderr)
{
    if (bStderr) {
        return;
    }
    m_findData.append(data);
}

void GolangDoc::findFinish(bool error,int code,QString /*msg*/)
{
    if (!error && code == 0) {
        QStringList array = QString(m_findData.trimmed()).split(',');
        if (array.size() >= 2 && array.at(0) == "$find") {
            array.removeFirst();
            QString best = array.at(0);
            if (best.isEmpty()) {
                array.removeFirst();
            } else {
                godocPackage(best);
            }
            if (array.isEmpty()) {
                m_findResultModel->setStringList(QStringList() << "<not find>");
            } else {
                m_findResultModel->setStringList(array);
            }
        } else if (array.size() >= 1 && array.at(0) == "$list") {
            array.removeFirst();
            m_findResultModel->setStringList(array);
        }
    } else {
        m_findResultModel->setStringList(QStringList() << "<error>");
    }
}

void GolangDoc::godocPackage(QString package)
{
    if (package.isEmpty()) {
        return;
    }

    if (m_godocCmd.isEmpty()) {
        return;
    }
    m_godocData.clear();
    QStringList args;
    args << "-html=true" << package.split(" ");
    if (m_godocProcess->isRuning()) {
        m_godocProcess->waitForFinished(200);
    }
    m_lastUrl = QUrl(package);
    m_lastHeader = "Package "+package;
    m_lastNav = true;
    m_godocProcess->start(m_godocCmd,args);
}

void GolangDoc::godocOutput(QByteArray data,bool bStderr)
{
    if (bStderr) {
        return;
    }
    m_godocData.append(data);
}

void GolangDoc::godocFinish(bool error,int code,QString /*msg*/)
{
    if (!error && code == 0 && m_docBrowser != 0) {
        updateDoc(m_lastUrl,m_godocData,m_lastHeader,m_lastNav);
    }
}

void GolangDoc::updateDoc(const QUrl &url, const QByteArray &ba, const QString &header, bool toNav)
{
    QTextCodec *codec = QTextCodec::codecForName("utf-8");
    QString genHeader;
    QString nav;
    QString content = docToNavdoc(codec->toUnicode(ba),genHeader,nav);
    QString data = m_templateData;
    if (!header.isEmpty()) {
        data.replace("{header}",header);
    } else {
        data.replace("{header}",genHeader);
    }
    if (toNav) {
        data.replace("{nav}",nav);
    } else {
        data.replace("{nav}","");
    }
    data.replace("{content}",content);
    m_docBrowser->setUrlHtml(url,data);
    activeBrowser();
}

void GolangDoc::openUrl(QUrl url)
{
    m_lastUrl = url;
    m_lastHeader.clear();
    m_lastNav = true;

    if (!url.isRelative() &&url.scheme() != "file") {
        QDesktopServices::openUrl(url);
        return;
    }
    QDir dir(m_goroot);
    QString path = url.path();
    if (!path.isEmpty() && path.at(0) == '/') {
        path.remove(0,1);
    }
    QFileInfo i(dir,path);
    if (!i.exists()) {
        dir.cd("doc");
        i.setFile(dir,path);
    }

    if (i.exists()) {
        //dir
        if (i.isDir()) {
            QDir dir = i.dir();
            if (dir.dirName() == "pkg" || dir.dirName() == "cmd") {
                m_lastHeader = "Directory" + url.path();
                m_lastNav = false;
                if (m_findCmd.isEmpty()) {
                    return;
                }
                QStringList args;
                args << "-mode=html"<< QString("-list=src/%1").arg(dir.dirName());
                m_godocData.clear();
                m_godocProcess->start(m_findCmd,args);
                return;
            } else {
                QFileInfo test(dir,"index.html");
                if (test.exists()) {
                    i = test;
                }
            }
        }
        //file
        if (i.isFile()) {
            QString ext = i.suffix().toLower();
            if (ext == "html") {
                QFile file(i.filePath());
                if (file.open(QIODevice::ReadOnly)) {
                    QByteArray ba = file.readAll();
                    file.close();
                    if (i.fileName().compare("docs.html",Qt::CaseInsensitive) == 0) {
                        updateDoc(url,ba,QString(),false);
                    } else {
                        updateDoc(url,ba);
                    }
                }
            } else if (ext == "go") {
                LiteApi::IEditor *editor = m_liteApp->fileManager()->openEditor(i.filePath());
                if (editor) {
                    editor->setReadOnly(true);
                    QPlainTextEdit *ed = LiteApi::findExtensionObject<QPlainTextEdit*>(editor,"LiteApi.QPlainTextEdit");
                    if (ed && url.hasQueryItem("s")) {
                        QStringList pos = url.queryItemValue("s").split(":");
                        if (pos.length() == 2) {
                            bool ok = false;
                            int begin = pos.at(0).toInt(&ok);
                            if (ok) {
                                QTextCursor cur = ed->textCursor();
                                cur.setPosition(begin);
                                ed->setTextCursor(cur);
                                ed->centerCursor();
                            }
                        }
                    }
                }
            } else if (ext == "pdf") {
                QDesktopServices::openUrl(i.filePath());
            }
        }
        return;
    } else {
        godocPackage(url.toString());
    }
}

void GolangDoc::doubleClickListView(QModelIndex index)
{
    if (!index.isValid()) {
        return;
    }
    QString text = m_findResultModel->data(index,Qt::DisplayRole).toString();
    if (!text.isEmpty()) {
        godocPackage(text);
    }
}

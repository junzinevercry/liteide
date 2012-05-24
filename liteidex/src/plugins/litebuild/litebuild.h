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
// Module: litebuild.h
// Creator: visualfc <visualfc@gmail.com>
// date: 2011-3-26
// $Id: litebuild.h,v 1.0 2011-5-12 visualfc Exp $

#ifndef LITEBUILD_H
#define LITEBUILD_H

#include "liteapi/liteapi.h"
#include "liteenvapi/liteenvapi.h"
#include "litebuildapi/litebuildapi.h"

#include <QTextCursor>

class BuildManager;
class QComboBox;
class ProcessEx;
class LiteOutput;
class QStandardItemModel;

class LiteBuild : public LiteApi::ILiteBuild
{
    Q_OBJECT
public:
    explicit LiteBuild(LiteApi::IApplication *app, QObject *parent = 0);
    virtual ~LiteBuild();
public:
    virtual QString buildFilePath() const;
    virtual QString targetFilePath() const;
    virtual QMap<QString,QString> buildEnvMap() const;
    virtual QMap<QString,QString> liteideEnvMap() const;
    virtual LiteApi::IBuildManager *buildManager() const;
    QMap<QString,QString> buildEnvMap(LiteApi::IBuild *build, const QString &buildFilePath) const;
public:
    QString actionValue(const QString &value,QMap<QString,QString> &liteEnv,const QProcessEnvironment &env);
    void setProjectBuild(LiteApi::IBuild *build);
    void loadProjectInfo(const QString &filePath);
    void loadEditorInfo(const QString &filePath);
    LiteApi::IBuild *findProjectBuildByEditor(LiteApi::IEditor *editor);
    LiteApi::IBuild *findProjectBuild(LiteApi::IProject *project);
public slots:
    void appLoaded();
    void currentEnvChanged(LiteApi::IEnv*);
    void currentProjectChanged(LiteApi::IProject*);
    void reloadProject();
    void editorCreated(LiteApi::IEditor *editor);
    void currentEditorChanged(LiteApi::IEditor*);
    void buildAction();
    void execAction(const QString &mime,const QString &id);
    void extOutput(const QByteArray &output,bool bError);
    void extFinish(bool error,int exitCode, QString msg);
    void stopAction();
    void dbclickBuildOutput(const QTextCursor &cur);
    void enterTextBuildOutput(QString);
    void config();
protected:
    LiteApi::IApplication   *m_liteApp;
    BuildManager    *m_manager;
    LiteApi::IBuild *m_build;
    LiteApi::IEnvManager *m_envManager;
    QToolBar    *m_toolBar;
    QAction     *m_configAct;
    QStandardItemModel *m_liteideModel;
    QStandardItemModel *m_configModel;
    QStandardItemModel *m_customModel;
    QList<QAction*> m_actions;
    QMap<QString,QString> m_liteAppInfo;
    QMap<QString,QString> m_configMap;
    QMap<QString,QString> m_customMap;
    QString m_workDir;
    ProcessEx *m_process;
    LiteOutput *m_output;
    QString     m_outputRegex;
    QString     m_buildFilePath;
    QMap<QString,QString> m_editorInfo;
    QMap<QString,QString> m_projectInfo;
    QMap<QString,QString> m_targetInfo;
};

#endif // LITEBUILD_H

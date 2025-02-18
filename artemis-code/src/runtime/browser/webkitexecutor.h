/*
 * Copyright 2012 Aarhus University
 *
 * Licensed under the GNU General Public License, Version 3 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *          http://www.gnu.org/licenses/gpl-3.0.html
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WEBKITEXECUTOR_H
#define WEBKITEXECUTOR_H

#include <QObject>
#include <QSemaphore>
#include <QtWebKit>
#include <QtWebKit/qwebexecutionlistener.h>
#include <QSharedPointer>
#include <QNetworkReply>

#include "artemisglobals.h"

#include "runtime/executableconfiguration.h"
#include "runtime/browser/ajax/ajaxrequestlistener.h"
#include "runtime/appmodel.h"
#include "model/coverage/coveragelistener.h"
#include "model/pathtracer.h"
#include "strategies/inputgenerator/targets/jquerylistener.h"
#include "runtime/input/forms/formfieldinjector.h"

#include "executionresult.h"
#include "executionresultbuilder.h"
#include "artemiswebpage.h"

#include "concolic/executiontree/tracebuilder.h"
#include "concolic/traceeventdetectors.h"

namespace artemis
{

/**
 * Responsible for all direct interaction with WebKit and controlling the
 * low-level execution of the selected configurations.
 */
class WebKitExecutor : public QObject
{
    Q_OBJECT

public:
    WebKitExecutor(QObject* parent,
                   AppModelPtr appmodel,
                   QMap<QString, InjectionValue> presetFields,
                   JQueryListener* jqueryListener,
                   AjaxRequestListener* ajaxListener,
                   bool enableConstantStringInstrumentation,
                   bool enablePropertyAccessInstrumentation);
    ~WebKitExecutor();

    void executeSequence(ExecutableConfigurationConstPtr conf);
    void executeSequence(ExecutableConfigurationConstPtr conf, bool keepOpen);
    void detach();

    ArtemisWebPagePtr getPage();

    TraceBuilder* getTraceBuilder();

    QWebExecutionListener* mWebkitListener; // TODO should not be public

private:

    ArtemisWebPagePtr mPage;
    ExecutionResultBuilderPtr mResultBuilder;
    ExecutableConfigurationConstPtr currentConf;
    AjaxRequestListener* mAjaxListener;
    JQueryListener* mJquery;
    QMap<QString, InjectionValue> mPresetFields;

    CoverageListenerPtr mCoverageListener;
    JavascriptStatisticsPtr mJavascriptStatistics;
    PathTracerPtr mPathTracer;
    bool mNextOpCanceled;
    bool mKeepOpen;
    bool testingDone;

    TraceBuilder* mTraceBuilder;

signals:
    void sigExecutedSequence(ExecutableConfigurationConstPtr conf, QSharedPointer<ExecutionResult> res);
    void sigAbortedExecution(QString reason);

public slots:
    void slNAMFinished(QNetworkReply* reply);
    void slLoadFinished(bool ok);
    void slLoadProgress(int i);


};

}
#endif // WEBKITEXECUTOR_H

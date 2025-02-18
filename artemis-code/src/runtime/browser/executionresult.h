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

#ifndef EXECUTIONRESULT_H
#define EXECUTIONRESULT_H

#include <inttypes.h>

#include <QSet>
#include <QPair>
#include <QList>

#include "artemisglobals.h"
#include "runtime/input/events/eventhandlerdescriptor.h"
#include "runtime/input/forms/formfielddescriptor.h"
#include "runtime/browser/timer.h"
#include "runtime/browser/ajax/ajaxrequest.h"

namespace artemis
{

class ExecutionResult
{

public:
    ExecutionResult();

    QList<EventHandlerDescriptorConstPtr> getEventHandlers() const;
    QList<QSharedPointer<const FormFieldDescriptor> > getFormFields() const;

    bool isDomModified() const;
    long getPageStateHash() const;
    QString getPageContents() const;

    QSet<QSharedPointer<AjaxRequest> > getAjaxRequests() const;
    QList<int> getAjaxCallbackHandlers() const;

    QSet<QString> getEvalStrings();
    QList<QSharedPointer<Timer> > getTimers() const;

    QSet<QString> getJavascriptConstantsObservedForLastEvent() const;
    void addJavascriptConstantObservedForLastEvent(QString constant);

    QDebug friend operator<<(QDebug dbg, const ExecutionResult& e);

    friend class ExecutionResultBuilder;

private:
    QList<EventHandlerDescriptorConstPtr> mEventHandlers;
    QList<QSharedPointer<const FormFieldDescriptor> > mFormFields;

    bool mModifiedDom;
    long mStateHash;
    QString mPageContents;

    QSet<QSharedPointer<AjaxRequest> > mAjaxRequest;
    QList<int> mAjaxCallbackHandlers;

    QSet<QString> mEvaledStrings;
    QMap<int, QSharedPointer<Timer> > mTimers; // <timerId, Timer>

    QSet<QString> mJavascriptConstantsObservedForLastEvent;

};

typedef QSharedPointer<ExecutionResult> ExecutionResultPtr;
typedef QSharedPointer<const ExecutionResult> ExecutionResultConstPtr;
}

#endif // EXECUTIONRESULT_H

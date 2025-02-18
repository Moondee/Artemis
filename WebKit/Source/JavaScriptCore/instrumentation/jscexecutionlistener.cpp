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

#ifdef ARTEMIS

#include <iostream>
#include <cstdlib>
#include <QDebug>

#include "jscexecutionlistener.h"

using namespace std;

namespace jscinst {

JSCExecutionListener::JSCExecutionListener() :
    m_propertyAccessInstrumentationEnabled(false),
    m_constantStringInstrumentationEnabled(false)
{
}

void JSCExecutionListener::javascript_eval_call(const char* eval_string) {
    qWarning()  << "Warning: Default listener for javascript_eval_call was invoked, args: " << eval_string << endl;
}

void JSCExecutionListener::javascript_bytecode_executed(JSC::Interpreter*, JSC::CodeBlock*, JSC::Instruction*, const JSC::BytecodeInfo&) {
    qWarning()  << "Warning: Default listener for javascript_bytecode_executed was invoked " << endl;
    //exit(1);
}

void JSCExecutionListener::javascript_branch_executed(bool jump, Symbolic::Expression* condition, JSC::ExecState*, const JSC::Instruction*, const JSC::BytecodeInfo&) {
    qWarning() << "Warning: Default listener for javascript_branch_executed was invoked " << endl;
    //exit(1);
}

void JSCExecutionListener::javascriptConstantStringEncountered(std::string) {
    qWarning()  << "Warning: Default listener for javascript_constant_encountered was invoked " << endl;
    //exit(1);
}

void JSCExecutionListener::javascript_symbolic_field_read(std::string variable, bool isSymbolic) {
    qWarning() << "Warning: Default listener for javascript_symbolic_field_read was invoked." << endl;
}

void JSCExecutionListener::javascript_property_read(std::string, JSC::ExecState*)
{
    qWarning()  << "Warning: Default listener for javascript_property_read was invoked " << endl;
    //exit(1);
}

void JSCExecutionListener::javascript_property_written(std::string, JSC::ExecState*)
{
    qWarning()  << "Warning: Default listener for javascript_property_written was invoked " << endl;
    //exit(1);
}

JSCExecutionListener* jsc_listener = 0;

void register_jsc_listener(JSCExecutionListener* listener) {
    jsc_listener = listener;
}

JSCExecutionListener* get_jsc_listener() {
    if (jsc_listener == 0) {
        jsc_listener = new JSCExecutionListener();
    }
    return jsc_listener;
}

}
#endif

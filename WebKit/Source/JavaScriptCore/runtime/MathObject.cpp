/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008 Apple Inc. All Rights Reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "MathObject.h"
#include <iostream>

#include "Lookup.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include <time.h>
#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/RandomNumber.h>
#include <wtf/RandomNumberSeed.h>

#include <statistics/statsstorage.h>

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(MathObject);

static EncodedJSValue JSC_HOST_CALL mathProtoFuncAbs(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncACos(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncASin(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncATan(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncATan2(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncCeil(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncCos(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncExp(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncFloor(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncLog(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncMax(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncMin(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncPow(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncRandom(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncRound(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncSin(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncSqrt(ExecState*);
static EncodedJSValue JSC_HOST_CALL mathProtoFuncTan(ExecState*);

}

#include "MathObject.lut.h"

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(MathObject);

const ClassInfo MathObject::s_info = { "Math", &JSNonFinalObject::s_info, 0, ExecState::mathTable, CREATE_METHOD_TABLE(MathObject) };

/* Source for MathObject.lut.h
@begin mathTable
  abs           mathProtoFuncAbs               DontEnum|Function 1
  acos          mathProtoFuncACos              DontEnum|Function 1
  asin          mathProtoFuncASin              DontEnum|Function 1
  atan          mathProtoFuncATan              DontEnum|Function 1
  atan2         mathProtoFuncATan2             DontEnum|Function 2
  ceil          mathProtoFuncCeil              DontEnum|Function 1
  cos           mathProtoFuncCos               DontEnum|Function 1
  exp           mathProtoFuncExp               DontEnum|Function 1
  floor         mathProtoFuncFloor             DontEnum|Function 1
  log           mathProtoFuncLog               DontEnum|Function 1
  max           mathProtoFuncMax               DontEnum|Function 2
  min           mathProtoFuncMin               DontEnum|Function 2
  pow           mathProtoFuncPow               DontEnum|Function 2
  random        mathProtoFuncRandom            DontEnum|Function 0 
  round         mathProtoFuncRound             DontEnum|Function 1
  sin           mathProtoFuncSin               DontEnum|Function 1
  sqrt          mathProtoFuncSqrt              DontEnum|Function 1
  tan           mathProtoFuncTan               DontEnum|Function 1
@end
*/

MathObject::MathObject(JSGlobalObject* globalObject, Structure* structure)
    : JSNonFinalObject(globalObject->globalData(), structure)
{
}

void MathObject::finishCreation(ExecState* exec, JSGlobalObject* globalObject)
{
    Base::finishCreation(globalObject->globalData());
    ASSERT(inherits(&s_info));

    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "E"), jsNumber(exp(1.0)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "LN2"), jsNumber(log(2.0)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "LN10"), jsNumber(log(10.0)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "LOG2E"), jsNumber(1.0 / log(2.0)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "LOG10E"), jsNumber(0.4342944819032518), DontDelete | DontEnum | ReadOnly); // See ECMA-262 15.8.1.5
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "PI"), jsNumber(piDouble), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "SQRT1_2"), jsNumber(sqrt(0.5)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "SQRT2"), jsNumber(sqrt(2.0)), DontDelete | DontEnum | ReadOnly);
}

bool MathObject::getOwnPropertySlot(JSCell* cell, ExecState* exec, const Identifier& propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<JSObject>(exec, ExecState::mathTable(exec), jsCast<MathObject*>(cell), propertyName, slot);
}

bool MathObject::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<JSObject>(exec, ExecState::mathTable(exec), jsCast<MathObject*>(object), propertyName, descriptor);
}

// ------------------------------ Functions --------------------------------

EncodedJSValue JSC_HOST_CALL mathProtoFuncAbs(ExecState* exec)
{
    if (exec->argument(0).isSymbolic()) {
        Statistics::statistics()->accumulate("Concolic::MissingInstrumentation::mathProtoFuncAbs", 1);
    }
    return JSValue::encode(jsNumber(fabs(exec->argument(0).toNumber(exec))));
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncACos(ExecState* exec)
{
    if (exec->argument(0).isSymbolic()) {
        Statistics::statistics()->accumulate("Concolic::MissingInstrumentation::mathProtoFuncACos", 1);
    }
    return JSValue::encode(jsDoubleNumber(acos(exec->argument(0).toNumber(exec))));
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncASin(ExecState* exec)
{
    if (exec->argument(0).isSymbolic()) {
        Statistics::statistics()->accumulate("Concolic::MissingInstrumentation::mathProtoFuncASin", 1);
    }
    return JSValue::encode(jsDoubleNumber(asin(exec->argument(0).toNumber(exec))));
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncATan(ExecState* exec)
{
    if (exec->argument(0).isSymbolic()) {
        Statistics::statistics()->accumulate("Concolic::MissingInstrumentation::mathProtoFuncATan", 1);
    }
    return JSValue::encode(jsDoubleNumber(atan(exec->argument(0).toNumber(exec))));
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncATan2(ExecState* exec)
{
    if (exec->argument(0).isSymbolic() || exec->argument(1).isSymbolic()) {
        Statistics::statistics()->accumulate("Concolic::MissingInstrumentation::mathProtoFuncATan2", 1);
    }

    double arg0 = exec->argument(0).toNumber(exec);
    double arg1 = exec->argument(1).toNumber(exec);
    return JSValue::encode(jsDoubleNumber(atan2(arg0, arg1)));
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncCeil(ExecState* exec)
{
    JSValue origin = exec->argument(0);
    JSValue v = jsNumber(ceil(exec->argument(0).toNumber(exec)));

    if (origin.isSymbolic()) {
        if (origin.isNumber()) {
            v.makeSymbolic(origin.asSymbolic());
        } else {
            v.makeSymbolic((Symbolic::IntegerExpression*)new Symbolic::IntegerCoercion(origin.asSymbolic()));
        }
    }

    return JSValue::encode(v);
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncCos(ExecState* exec)
{
    if (exec->argument(0).isSymbolic()) {
        Statistics::statistics()->accumulate("Concolic::MissingInstrumentation::mathProtoFuncCos", 1);
    }
    return JSValue::encode(jsDoubleNumber(cos(exec->argument(0).toNumber(exec))));
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncExp(ExecState* exec)
{
    if (exec->argument(0).isSymbolic()) {
        Statistics::statistics()->accumulate("Concolic::MissingInstrumentation::mathProtoFuncExp", 1);
    }
    return JSValue::encode(jsDoubleNumber(exp(exec->argument(0).toNumber(exec))));
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncFloor(ExecState* exec)
{
    JSValue origin = exec->argument(0);
    JSValue v = jsNumber(origin.toNumber(exec));

    if (origin.isSymbolic()) {
        if (origin.isNumber()) {
            v.makeSymbolic(origin.asSymbolic());
        } else {
            v.makeSymbolic((Symbolic::IntegerExpression*)new Symbolic::IntegerCoercion(origin.asSymbolic()));
        }
    }

    return JSValue::encode(v);
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncLog(ExecState* exec)
{
    if (exec->argument(0).isSymbolic()) {
        Statistics::statistics()->accumulate("Concolic::MissingInstrumentation::mathProtoFuncLog", 1);
    }
    return JSValue::encode(jsDoubleNumber(log(exec->argument(0).toNumber(exec))));
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncMax(ExecState* exec)
{
    bool isSymbolic = false;
    std::list<Symbolic::Expression*> args;

    unsigned argsCount = exec->argumentCount();
    double result = -std::numeric_limits<double>::infinity();
    for (unsigned k = 0; k < argsCount; ++k) {
        JSValue v = exec->argument(k);

        double val = v.toNumber(exec);
        if (isnan(val)) {
            result = std::numeric_limits<double>::quiet_NaN();
            break;
        }
        if (val > result || (val == 0 && result == 0 && !signbit(val)))
            result = val;

        // Symbolic handling

        if (v.isSymbolic()) {
            isSymbolic = true;
            if (v.isNumber()) {
                args.push_back(v.asSymbolic());
            } else {
                args.push_back((Symbolic::IntegerExpression*)new Symbolic::IntegerCoercion(v.asSymbolic()));
            }
        } else {
            args.push_back(new Symbolic::ConstantInteger(val));
        }
    }

    JSValue res = jsNumber(result);

    if (isSymbolic) {
        res.makeSymbolic(new Symbolic::IntegerMaxMin(args, true));
    }
    return JSValue::encode(res);
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncMin(ExecState* exec)
{
    bool isSymbolic = false;
    std::list<Symbolic::Expression*> args;

    unsigned argsCount = exec->argumentCount();
    double result = +std::numeric_limits<double>::infinity();
    for (unsigned k = 0; k < argsCount; ++k) {
        JSValue v = exec->argument(k);

        double val = v.toNumber(exec);
        if (isnan(val)) {
            result = std::numeric_limits<double>::quiet_NaN();
            break;
        }
        if (val < result || (val == 0 && result == 0 && signbit(val)))
            result = val;

        // Symbolic handling

        if (v.isSymbolic()) {
            isSymbolic = true;
            if (v.isNumber()) {
                args.push_back(v.asSymbolic());
            } else {
                args.push_back((Symbolic::IntegerExpression*)new Symbolic::IntegerCoercion(v.asSymbolic()));
            }
        } else {
            args.push_back(new Symbolic::ConstantInteger(val));
        }
    }

    JSValue res = jsNumber(result);

    if (isSymbolic) {
        res.makeSymbolic(new Symbolic::IntegerMaxMin(args, false));
    }
    return JSValue::encode(res);
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncPow(ExecState* exec)
{
    // ECMA 15.8.2.1.13

    if (exec->argument(0).isSymbolic() || exec->argument(1).isSymbolic()) {
        Statistics::statistics()->accumulate("Concolic::MissingInstrumentation::mathProtoFuncPow", 1);
    }

    double arg = exec->argument(0).toNumber(exec);
    double arg2 = exec->argument(1).toNumber(exec);

    if (isnan(arg2))
        return JSValue::encode(jsNaN());
    if (isinf(arg2) && fabs(arg) == 1)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(pow(arg, arg2)));
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncRandom(ExecState* exec)
{
    Statistics::statistics()->accumulate("Concolic::MissingInstrumentation::mathProtoFuncRandom", 1);

    return JSValue::encode(jsDoubleNumber(exec->lexicalGlobalObject()->weakRandomNumber()));
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncRound(ExecState* exec)
{
    if (exec->argument(0).isSymbolic()) {
        Statistics::statistics()->accumulate("Concolic::MissingInstrumentation::mathProtoFuncRound", 1);
    }

    double arg = exec->argument(0).toNumber(exec);
    double integer = ceil(arg);
    return JSValue::encode(jsNumber(integer - (integer - arg > 0.5)));
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncSin(ExecState* exec)
{
    if (exec->argument(0).isSymbolic()) {
        Statistics::statistics()->accumulate("Concolic::MissingInstrumentation::mathProtoFuncSin", 1);
    }
    return JSValue::encode(exec->globalData().cachedSin(exec->argument(0).toNumber(exec)));
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncSqrt(ExecState* exec)
{
    if (exec->argument(0).isSymbolic()) {
        Statistics::statistics()->accumulate("Concolic::MissingInstrumentation::mathProtoFuncSqrt", 1);
    }
    return JSValue::encode(jsDoubleNumber(sqrt(exec->argument(0).toNumber(exec))));
}

EncodedJSValue JSC_HOST_CALL mathProtoFuncTan(ExecState* exec)
{
    if (exec->argument(0).isSymbolic()) {
        Statistics::statistics()->accumulate("Concolic::MissingInstrumentation::mathProtoFuncTan", 1);
    }
    return JSValue::encode(jsDoubleNumber(tan(exec->argument(0).toNumber(exec))));
}

} // namespace JSC

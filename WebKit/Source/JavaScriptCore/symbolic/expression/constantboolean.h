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

 // AUTO GENERATED - DO NOT MODIFY

#ifndef SYMBOLIC_CONSTANTBOOLEAN_H
#define SYMBOLIC_CONSTANTBOOLEAN_H

#include <string>

#include <list>

#include "visitor.h"
#include "booleanexpression.h"

#ifdef ARTEMIS

namespace Symbolic
{

class ConstantBoolean : public BooleanExpression
{
public:
    explicit ConstantBoolean(bool value);
    void accept(Visitor* visitor);
    void accept(Visitor* visitor, void* arg);

	inline bool getValue() {
		return m_value;
	}

private:
	bool m_value;

};
}

#endif
#endif // SYMBOLIC_CONSTANTBOOLEAN_H
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

#include <QPair>
#include <QString>
#include <QSet>
#include <QList>
#include <QHash>

#include "runtime/input/forms/formfielddescriptor.h"
#include "runtime/browser/artemiswebpage.h"


#ifndef FORMFIELDRESTRICTEDVALUES_H
#define FORMFIELDRESTRICTEDVALUES_H

namespace artemis
{

typedef struct {
    QString variable;
    QList<QString> values;
} SelectRestriction;

typedef struct {
    QString groupName;
    bool alwaysSet; // If no option is set by default, it is possible to submit with all variables set to false.
    QSet<QString> variables;
} RadioRestriction;

typedef QPair<QSet<SelectRestriction>, QSet<RadioRestriction> > FormRestrictions;




class FormFieldRestrictedValues
{
public:
    static FormRestrictions getRestrictions(QList<FormFieldDescriptorConstPtr> formFields, ArtemisWebPagePtr page);

    // Helper for cvc4.cpp to check whether a variable is safe to coerce.
    static bool safeForIntegerCoercion(FormRestrictions restrictions, QString variable);
    static bool symbolReferencesSelect(FormRestrictions restrictions, QString identifier);
    static bool isValidSelectValue(FormRestrictions restrictions, QString identifier, QString symbolvalue);
    static bool fuzzyMatchSelectValue(FormRestrictions restrictions, QString identifier, std::string* symbolvalue);

    // Helper for concolic runtime to find if there are restrictions on a certain variable.
    static QPair<bool, SelectRestriction> getRelevantSelectRestriction(FormRestrictions restrictions, QString identifier);

protected:
    // TODO: Duplicated functionality in ConcolicRuntime and the code generated by CodeGeneratorJS.pm.
    static QString getVariableName(FormFieldDescriptorConstPtr field);
};




// Hashing and comparison methods for the restriction types, so they can be put into sets, etc.

inline bool operator==(const SelectRestriction& a, const SelectRestriction& b)
{
    return a.variable == b.variable && a.values == b.values;
}

inline uint qHash(const SelectRestriction& key)
{
    return ::qHash(key.variable) ^ ::qHash(key.values.size());
}

inline bool operator==(const RadioRestriction& a, const RadioRestriction& b)
{
    return a.groupName == b.groupName && a.alwaysSet == b.alwaysSet && a.variables == b.variables;
}

inline uint qHash(const RadioRestriction& key)
{
    return ::qHash(key.groupName) ^ ::qHash((int)key.alwaysSet) ^ ::qHash(key.variables.size());
}



} //namespace artemis
#endif // FORMFIELDRESTRICTEDVALUES_H

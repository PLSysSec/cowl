/*
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 */

#ifndef COWLParser_h
#define COWLParser_h

#include "core/CoreExport.h"
#include "core/cowl/COWLPrincipal.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class Label;
class Privilege;

class CORE_EXPORT COWLParser {

 public:
   static COWLPrincipalType ValidatePrincipal(const String&);
   static Label* ParseLabelExpression(const String& principal, const String& self_url);
   static void parseLabeledDataHeader(const String& expr, const String& self_url, Label*& out_conf, Label*& out_int);
   static void parseLabeledContextHeader(const String& expr, const String& self_url, Label*& out_conf, Label*& out_int, Privilege*& out_priv);
};

}  // namespace blink

#endif // COWLParser_h

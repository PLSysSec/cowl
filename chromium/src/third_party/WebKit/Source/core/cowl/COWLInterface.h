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

#ifndef COWLInterface_h
#define COWLInterface_h

#include "bindings/core/v8/ExceptionState.h"
#include "core/CoreExport.h"
#include "core/cowl/COWL.h"
#include "core/dom/ExecutionContext.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"

namespace blink {

class CORE_EXPORT COWLInterface final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static void enable(ScriptState*);
  static bool isEnabled(const ScriptState*);

  static Label* confidentiality(const ScriptState*);
  static void setConfidentiality(ScriptState*, Label*, ExceptionState&);

  static Label* integrity(const ScriptState*);
  static void setIntegrity(ScriptState*, Label*, ExceptionState&);

  static Privilege* privilege(const ScriptState*);
  static void setPrivilege(ScriptState*, Privilege*, ExceptionState&);

  static COWL* GetCOWL(const ScriptState*);

  void Trace(blink::Visitor*);
};

}  // namespace blink

#endif  // COWLInterface.h

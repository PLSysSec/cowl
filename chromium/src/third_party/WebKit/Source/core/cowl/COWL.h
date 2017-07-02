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

#ifndef COWL_h
#define COWL_h

#include "bindings/core/v8/ExceptionState.h"
#include "core/CoreExport.h"
#include "core/cowl/Label.h"
#include "core/cowl/Privilege.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/SecurityContext.h"
#include "core/frame/LocalDOMWindow.h"
#include "platform/heap/Handle.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/weborigin/SecurityOrigin.h"

namespace blink {

class CORE_EXPORT COWL final : public GarbageCollectedFinalized<COWL>,
                               public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static COWL* Create();

  ~COWL();

  static void enable(ScriptState*);
  static bool isEnabled(const ScriptState*);

  static Label* confidentiality(const ScriptState*);
  static void setConfidentiality(ScriptState*, Label*, ExceptionState&);

  static Label* integrity(const ScriptState*);
  static void setIntegrity(ScriptState*, Label*, ExceptionState&);

  static Privilege* privilege(const ScriptState*);
  static void setPrivilege(ScriptState*, Privilege*, ExceptionState&);

  void BindToExecutionContext(ExecutionContext*);
  void SetupSelf(const SecurityOrigin&);
  void ApplyPolicySideEffectsToExecutionContext();

  static bool LabelRaiseWillResultInStuckContext(ScriptState*, Label*, Privilege*);

  Label* GetConfidentiality();
  Label* GetIntegrity();
  Privilege* GetPrivilege();

  DECLARE_TRACE();

 private:
  COWL();

  bool enabled_;
  Member<ExecutionContext> execution_context_;
  Member<Label> confidentiality_;
  Member<Label> integrity_;
  Member<Privilege> privilege_;
};

}  // namespace blink

#endif  // COWL.h

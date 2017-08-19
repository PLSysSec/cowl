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

#include "core/CoreExport.h"
#include "core/cowl/COWLParser.h"
#include "core/cowl/Label.h"
#include "core/cowl/Privilege.h"
#include "core/dom/ExecutionContext.h"
#include "core/inspector/ConsoleTypes.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/SecurityViolationReportingPolicy.h"

namespace blink {

class ConsoleMessage;
class ExecutionContext;
class ResourceRequest;
class SecurityOrigin;

class CORE_EXPORT COWL final : public GarbageCollectedFinalized<COWL> {

 public:
  static COWL* Create();
  ~COWL();
  DECLARE_TRACE();

  void BindToExecutionContext(ExecutionContext*);
  void SetupSelf(const SecurityOrigin&);
  void ApplyPolicySideEffectsToExecutionContext();

  bool LabelRaiseWillResultInStuckContext(Label*, Privilege*);
  bool WriteCheck(Label*, Label*);

  bool AllowRequest(const ResourceRequest&,
                    SecurityViolationReportingPolicy =
                    SecurityViolationReportingPolicy::kReport) const;

  void LogToConsole(const String& message, MessageLevel = kErrorMessageLevel) const;
  void LogToConsole(ConsoleMessage*, LocalFrame* = nullptr) const;

  bool IsEnabled() const;
  Label* GetConfidentiality() const;
  Label* GetIntegrity() const;
  Privilege* GetPrivilege() const;

  // TODO: make these private and COWLInterface a friend?
  void Enable();
  void SetConfidentiality(Label*);
  void SetIntegrity(Label*);
  void SetPrivilege(Privilege*);

 private:
  COWL();

  bool enabled_;
  Member<Label> confidentiality_;
  Member<Label> integrity_;
  Member<Privilege> privilege_;

  Member<ExecutionContext> execution_context_;
};

}  // namespace blink

#endif  // COWL.h

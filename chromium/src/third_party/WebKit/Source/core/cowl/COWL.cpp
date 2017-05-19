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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#include "core/cowl/COWL.h"

namespace blink {

COWL* COWL::Create() { return new COWL(); }

COWL::COWL()
  : enabled_(false),
    execution_context_(nullptr),
    confidentiality_(nullptr),
    integrity_(nullptr),
    privilege_(nullptr) {}

COWL::~COWL() {}

void COWL::enable(ScriptState* script_state) {
  ExecutionContext::From(script_state)->GetSecurityContext().GetCOWL()->enabled_ = true;
}

bool COWL::isEnabled(const ScriptState* script_state) {
  return ExecutionContext::From(script_state)->GetSecurityContext().GetCOWL()->enabled_;
}

Label* COWL::confidentiality(const ScriptState* script_state, ExceptionState& exception_state) {
  return ExecutionContext::From(script_state)->GetSecurityContext().GetCOWL()->confidentiality_;
}

void COWL::setConfidentiality(ScriptState* script_state, Label* label, ExceptionState& exception_state) {
  enable(script_state);

  Label* current_label = confidentiality(script_state, exception_state);
  Privilege* priv = privilege(script_state, exception_state);
  if (!label->subsumes(current_label, priv)) {
    exception_state.ThrowSecurityError("Label is not above the current label.");
    return;
  }
  if (LabelRaiseWillResultInStuckContext(script_state, label, priv)) {
    exception_state.ThrowSecurityError("Sorry cant do that, create a frame.");
    return;
  }
  ExecutionContext::From(script_state)->GetSecurityContext().GetCOWL()->confidentiality_ = label;
}

Label* COWL::integrity(const ScriptState* script_state, ExceptionState& exception_state) {
  return ExecutionContext::From(script_state)->GetSecurityContext().GetCOWL()->integrity_;
}

void COWL::setIntegrity(ScriptState* script_state, Label* label, ExceptionState& exception_state) {
  enable(script_state);

  Label* current_label = integrity(script_state, exception_state);
  Privilege* priv = privilege(script_state, exception_state);
  if (!current_label->subsumes(label, priv)) {
    exception_state.ThrowSecurityError("Label is not below the current label.");
    return;
  }
  ExecutionContext::From(script_state)->GetSecurityContext().GetCOWL()->integrity_ = label;
}

Privilege* COWL::privilege(const ScriptState* script_state, ExceptionState& exception_state) {
  return ExecutionContext::From(script_state)->GetSecurityContext().GetCOWL()->privilege_;
}

void COWL::setPrivilege(ScriptState* script_state, Privilege* priv, ExceptionState& exception_state) {
  enable(script_state);

  Label* current_label = confidentiality(script_state, exception_state);
  if (LabelRaiseWillResultInStuckContext(script_state, current_label, priv)) {
    exception_state.ThrowSecurityError("Sorry cant do that, create a frame.");
    return;
  }

  ExecutionContext::From(script_state)->GetSecurityContext().GetCOWL()->privilege_ = priv;
}

void COWL::BindToExecutionContext(ExecutionContext* execution_context) {
  execution_context_ = execution_context;
  ApplyPolicySideEffectsToExecutionContext();
}

void COWL::SetupSelf(const SecurityOrigin& security_origin) {
  confidentiality_ = Label::Create();
  integrity_ = Label::Create();
  privilege_ = Privilege::Create(security_origin.ToString());
}

void COWL::ApplyPolicySideEffectsToExecutionContext() {
  DCHECK(execution_context_ &&
      execution_context_->GetSecurityContext().GetSecurityOrigin());

  SetupSelf(*execution_context_->GetSecurityContext().GetSecurityOrigin());
}

bool COWL::LabelRaiseWillResultInStuckContext(ScriptState* script_state,
                                              Label* confidentiality,
                                              Privilege* priv) {
  LocalFrame* frame = ExecutionContext::From(script_state)->ExecutingWindow()->GetFrame();
  if (!frame)
    return false;

  // If |document|'s browsing context is not a top-level browsing context, then
  // return false.
  if (!frame->IsMainFrame())
    return false;

  Label* effective_label = confidentiality->Downgrade(priv);

  return !effective_label->IsEmpty();
}

DEFINE_TRACE(COWL) { 
  visitor->Trace(execution_context_);
  visitor->Trace(confidentiality_);
  visitor->Trace(integrity_);
  visitor->Trace(privilege_);
}

}  // namespace blink

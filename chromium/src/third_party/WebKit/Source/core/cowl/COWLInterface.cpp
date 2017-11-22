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

#include "core/cowl/COWLInterface.h"

namespace blink {

void COWLInterface::enable(ScriptState* script_state) {
  GetCOWL(script_state)->Enable();
}

bool COWLInterface::isEnabled(const ScriptState* script_state) {
  return GetCOWL(script_state)->IsEnabled();
}

Label* COWLInterface::confidentiality(const ScriptState* script_state) {
  return GetCOWL(script_state)->GetConfidentiality();
}

void COWLInterface::setConfidentiality(ScriptState* script_state, Label* conf, ExceptionState& exception_state) {
  COWL* cowl = GetCOWL(script_state);
  cowl->Enable();

  Label* current_label = cowl->GetConfidentiality();
  Privilege* priv = cowl->GetPrivilege();
  if (!conf->subsumes(current_label, priv)) {
    exception_state.ThrowSecurityError("Label is not above the current label.");
    return;
  }
  if (cowl->LabelRaiseWillResultInStuckContext(conf, priv)) {
    exception_state.ThrowSecurityError("Sorry cant do that, create a frame.");
    return;
  }
  cowl->SetConfidentiality(conf);
}

Label* COWLInterface::integrity(const ScriptState* script_state) {
  return GetCOWL(script_state)->GetIntegrity();
}

void COWLInterface::setIntegrity(ScriptState* script_state, Label* integrity, ExceptionState& exception_state) {
  COWL* cowl = GetCOWL(script_state);
  cowl->Enable();

  Label* current_label = cowl->GetIntegrity();
  Privilege* priv = cowl->GetPrivilege();
  if (!current_label->subsumes(integrity, priv)) {
    exception_state.ThrowSecurityError("Label is not below the current label.");
    return;
  }
  cowl->SetIntegrity(integrity);
}

Privilege* COWLInterface::privilege(const ScriptState* script_state) {
  return GetCOWL(script_state)->GetPrivilege();
}

void COWLInterface::setPrivilege(ScriptState* script_state, Privilege* priv, ExceptionState& exception_state) {
  COWL* cowl = GetCOWL(script_state);
  cowl->Enable();

  Label* current_label = cowl->GetConfidentiality();
  if (cowl->LabelRaiseWillResultInStuckContext(current_label, priv)) {
    exception_state.ThrowSecurityError("Sorry cant do that, create a frame.");
    return;
  }
  cowl->SetPrivilege(priv);
}

COWL* COWLInterface::GetCOWL(const ScriptState* script_state) {
  return ExecutionContext::From(script_state)->GetSecurityContext().GetCOWL();
}

void COWLInterface::Trace(blink::Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink

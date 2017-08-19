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

#include "core/cowl/LabeledObject.h"

#include "core/cowl/COWL.h"
#include "core/cowl/Label.h"
#include "core/cowl/Privilege.h"

namespace blink {

LabeledObject* LabeledObject::Create(ScriptState* script_state, ScriptValue obj, CILabel& labels, ExceptionState& exception_state) {
  COWL* cowl = ExecutionContext::From(script_state)->GetSecurityContext().GetCOWL();
  cowl->Enable();

  Label* confidentiality;
  if (labels.hasConfidentiality())
    confidentiality = labels.confidentiality();
  else
    confidentiality = cowl->GetConfidentiality();

  Label* integrity;
  if (labels.hasIntegrity())
    integrity = labels.integrity();
  else
    integrity = cowl->GetIntegrity();

  if (!cowl->WriteCheck(confidentiality, integrity)) {
    exception_state.ThrowSecurityError("Label of blob is not above current label or below current clearance.");
    return nullptr;
  }

  ScriptValue obj_clone =  StructuredClone(script_state, obj, exception_state);

  return new LabeledObject(obj_clone, confidentiality, integrity);
}


LabeledObject::LabeledObject(ScriptValue obj, Label* conf, Label* integrity)
  : obj_(obj),
    confidentiality_(conf),
    integrity_(integrity) {}

Label* LabeledObject::confidentiality() {
  return confidentiality_;
}

Label* LabeledObject::integrity() {
  return integrity_;
}

ScriptValue LabeledObject::protectedObject(ScriptState* script_state, ExceptionState& exception_state) {
  COWL* cowl = ExecutionContext::From(script_state)->GetSecurityContext().GetCOWL();
  cowl->Enable();

  Privilege* priv = cowl->GetPrivilege();

  Label* curr_conf = cowl->GetConfidentiality();
  Label* tmp_conf = curr_conf->and_(confidentiality_);
  Label* new_conf = tmp_conf->Downgrade(priv);

  if (cowl->LabelRaiseWillResultInStuckContext(new_conf, priv)) {
    exception_state.ThrowSecurityError("SecurityError: Will result in stuck-context, please use an iFrame");
    return ScriptValue::CreateNull(script_state);
  }
  cowl->SetConfidentiality(new_conf);

  Label* curr_integrity = cowl->GetIntegrity();
  Label* tmp_integrity = curr_integrity->or_(integrity_);
  Label* new_integrity = tmp_integrity->Downgrade(priv);

  cowl->SetIntegrity(new_integrity);

  ScriptValue obj_clone =  StructuredClone(script_state, obj_, exception_state);
  return obj_clone;
}

LabeledObject* LabeledObject::clone(ScriptState* script_state, CILabel& labels, ExceptionState& exception_state) {
  Label* new_conf;
  if (labels.hasConfidentiality())
    new_conf = labels.confidentiality();
  else
    new_conf = confidentiality_;

  Label* new_int;
  if (labels.hasIntegrity())
    new_int = labels.integrity();
  else
    new_int = integrity_;

  COWL* cowl = ExecutionContext::From(script_state)->GetSecurityContext().GetCOWL();
  Privilege* priv = cowl->GetPrivilege();

  if (!new_conf->subsumes(confidentiality_, priv)) {
    exception_state.ThrowSecurityError("SecurityError: Confidentiality label needs to be more restrictive");
    return nullptr;
  }
    
  if (!integrity_->subsumes(new_int, priv)) {
    exception_state.ThrowSecurityError("SecurityError: Check integrity label");
    return nullptr;
  }

  CILabel new_labels;
  new_labels.setConfidentiality(new_conf);
  new_labels.setIntegrity(new_int);
  return Create(script_state, obj_, new_labels, exception_state);
}

ScriptValue LabeledObject::GetObj() { return obj_; }

ScriptValue LabeledObject::StructuredClone(ScriptState* script_state,
                                           ScriptValue obj,
                                           ExceptionState& exception_state) {
  v8::Isolate* isolate = script_state->GetIsolate();
  v8::Local<v8::Value> value = obj.V8Value();
  RefPtr<SerializedScriptValue> serialized =
                                SerializedScriptValue::SerializeAndSwallowExceptions(isolate, value);
  v8::Local<v8::Value> result = serialized->Deserialize(isolate);
  // If there's a problem during deserialization, result would be null
  if (result->IsNull()) {
    exception_state.ThrowDOMException(kDataCloneError, "Object cannot be serialized");
    return ScriptValue::CreateNull(script_state);
  }
  ScriptValue obj_clone =  ScriptValue(script_state, result);
  return obj_clone;
}

DEFINE_TRACE(LabeledObject) {
  visitor->Trace(confidentiality_);
  visitor->Trace(integrity_);
}

}  // namespace blink

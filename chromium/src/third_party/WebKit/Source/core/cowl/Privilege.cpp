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

#include "core/cowl/Privilege.h"

#include "platform/UUID.h"
#include "core/cowl/Label.h"

namespace blink {

Privilege* Privilege::Create() {
  return new Privilege();
}

Privilege* Privilege::Create(String priv) {
  Label* label = Label::Create(priv);
  return new Privilege(label);
}

Privilege* Privilege::CreateForJSConstructor() {
  String uuid = "unique:";
  uuid.append(CreateCanonicalUUIDString());
  Label* label = Label::Create(uuid);
  return new Privilege(label);
}

Privilege::Privilege() {
  label_ = Label::Create();
}

Privilege::Privilege(Label* label) : label_(label) {}

Label* Privilege::asLabel() const {
  return label_->Clone();
}

Privilege* Privilege::combine(Privilege* other) const {
  Label* new_label = label_->and_(other->label_);
  return new Privilege(new_label);
}

Privilege* Privilege::delegate(Label* label, ExceptionState& exception_state) const {
  if (!label_->subsumes(label)) {
    exception_state.ThrowSecurityError("SecurityError: Earlier privilege does not subsume label.");
    return nullptr;
  }
  return new Privilege(label);
}

DEFINE_TRACE(Privilege) { visitor->Trace(label_); }

}  // namespace blink

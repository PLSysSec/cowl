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

#include "platform/UUID.h"
#include "core/cowl/Privilege.h"

namespace blink {

  Privilege* Privilege::CreateForJSConstructor() {
    String uuid = "unique:";
    uuid.Append(CreateCanonicalUUIDString());
    Label* label = Label::Create(uuid);
    return new Privilege(label);
  }

  bool Privilege::equals (Privilege* other) const {
    return label_->equals(other->DirectLabel());
  }

  bool Privilege::subsumes (Privilege* other) const {
    return label_->subsumes(other->DirectLabel());
  }

  Privilege* Privilege::combine(Privilege* other) {
    label_->And(other->DirectLabel());
    Privilege* this_ = this;
    return this_;
  }

  Privilege* Privilege::delegate(Label* label, ExceptionState& exception_state) {
    if (!label_->subsumes(label)) {
      exception_state.ThrowSecurityError("SecurityError: Earlier privilege does not subsume label.");
      return nullptr;
    }
    return new Privilege(label);
  }

  String Privilege::toString() const {
    String retval = "Privilege(";
    retval.Append(label_->toString());
    retval.Append(")");
    return retval;
  }

}  // namespace blink

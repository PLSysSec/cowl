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

#ifndef Privilege_h
#define Privilege_h

#include "platform/heap/Handle.h"
#include "bindings/core/v8/ScriptWrappable.h"
#include "bindings/core/v8/ExceptionState.h"
#include "platform/wtf/Forward.h"
#include "core/cowl/Label.h"

namespace blink {

class Privilege final : public GarbageCollectedFinalized<Privilege>,
                        public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static Privilege* Create() { return new Privilege; }
  static Privilege* CreateForJSConstructor();
  bool equals(Privilege*) const;
  bool subsumes(Privilege*) const;
  Privilege* combine(Privilege*) const;
  Privilege* delegate(Label*, ExceptionState&) const;
  bool isEmpty() const;
  Label* asLabel() const;
  String toString() const;

  DEFINE_INLINE_TRACE() { visitor->Trace(label_); }

 private:
  Privilege() { label_ = Label::Create(); }
  explicit Privilege(Label* label) : label_(label) {}
  Member<Label> label_;
};

}  // namespace blink

#endif  // Privilege_h

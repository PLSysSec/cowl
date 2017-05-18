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

#include "bindings/core/v8/ExceptionState.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class Label;

class Privilege final : public GarbageCollectedFinalized<Privilege>,
                        public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static Privilege* Create();
  static Privilege* Create(String);
  static Privilege* CreateForJSConstructor();
  Label* asLabel() const;
  Privilege* combine(Privilege*) const;
  Privilege* delegate(Label*, ExceptionState&) const;

  DECLARE_TRACE();

 private:
  Privilege();
  explicit Privilege(Label*);
  Member<Label> label_;
};

}  // namespace blink

#endif  // Privilege_h

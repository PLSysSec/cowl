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

#ifndef LabeledObject_h
#define LabeledObject_h

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptValue.h"
#include "bindings/core/v8/V8CILabel.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class Label;

class LabeledObject final : public GarbageCollectedFinalized<LabeledObject>,
                            public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static LabeledObject* Create(ScriptState*, ScriptValue, CILabel&, ExceptionState&);

  Label* confidentiality();
  Label* integrity();

  ScriptValue protectedObject(ScriptState*, ExceptionState&);

  LabeledObject* clone(ScriptState*, CILabel&, ExceptionState&);

  DECLARE_TRACE();

  // Helper functions
  ScriptValue GetObj();
  static ScriptValue StructuredClone(ScriptState*, ScriptValue, ExceptionState&);

 private:
  LabeledObject(ScriptValue, Label* conf, Label* integrity);
  ScriptValue obj_;
  Member<Label> confidentiality_;
  Member<Label> integrity_;
};

}  // namespace blink

#endif  // LabeledObject_h

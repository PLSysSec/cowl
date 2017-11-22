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

#ifndef Label_h
#define Label_h

#include "bindings/core/v8/ExceptionState.h"
#include "core/CoreExport.h"
#include "core/cowl/COWLPrincipal.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class Privilege;

typedef Vector<COWLPrincipal> DisjunctionSet;
typedef Vector<DisjunctionSet> DisjunctionSetArray;

class CORE_EXPORT Label final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static Label* Create(ExceptionState& = ASSERT_NO_EXCEPTION);
  static Label* Create(const String& principal, ExceptionState&);

  bool equals(Label*) const;
  bool subsumes(Label*) const;
  bool subsumes(Label*, Privilege*) const;
  Label* and_(const String& principal, ExceptionState&) const;
  Label* and_(Label*) const;
  Label* or_(const String& principal, ExceptionState&) const;
  Label* or_(Label*) const;
  String toString() const;

  // Helper functions
  static Label* Create(const DisjunctionSetArray);
  void InternalAnd(DisjunctionSet&, bool clone = false);
  void InternalOr(DisjunctionSet&);
  bool IsEmpty() const;
  Label* Clone() const;
  Label* Upgrade(Privilege*) const;
  Label* Downgrade(Privilege*) const;
  bool Contains(DisjunctionSet&) const;
  void RemoveRolesSubsumedBy(DisjunctionSet&);
  DisjunctionSetArray GetRoles();

  void Trace(blink::Visitor*);

 public:  // XXX TODO make private, unsafe
  DisjunctionSetArray* GetDirectRoles() { return &roles_; }

 private:
  Label();
  explicit Label(const DisjunctionSetArray& roles);
  Label(const Label&);

  DisjunctionSetArray roles_;
};

// Util functions
class DisjunctionSetUtils {
 public:
  static DisjunctionSet ConstructDset();
  static DisjunctionSet ConstructDset(const COWLPrincipal&);
  static DisjunctionSet CloneDset(const DisjunctionSet&);
  static bool Equals(const DisjunctionSet&, const DisjunctionSet&);
  static bool Subsumes(const DisjunctionSet&, const DisjunctionSet&);
  static void Or(DisjunctionSet&, DisjunctionSet&);
  static void InsertSorted(DisjunctionSet&, const COWLPrincipal&);
  static bool ContainsOriginPrincipal(const DisjunctionSet&);
  static String ToString(const DisjunctionSet&);
};

}  // namespace blink

#endif  // Label_h

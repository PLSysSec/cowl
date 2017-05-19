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
#include "platform/heap/Handle.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/wtf/Forward.h"
#include "core/cowl/COWLPrincipal.h"

namespace blink {

class Privilege;

typedef Vector<COWLPrincipal> DisjunctionSet;
typedef Vector<DisjunctionSet> DisjunctionSetArray;

class Label final : public GarbageCollectedFinalized<Label>,
                    public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static Label* Create();
  static Label* Create (const String& principal);
  bool equals(Label*) const;
  bool subsumes(Label*) const;
  bool subsumes(Label*, Privilege*) const;
  Label* and_(const String& principal) const;
  Label* and_(Label*) const;
  Label* or_(const String& principal) const;
  Label* or_(Label*) const;
  String toString() const;

  // Helper functions
  void InternalAnd(DisjunctionSet&, bool clone = false);
  void InternalOr(DisjunctionSet&);
  bool IsEmpty() const;
  Label* Clone() const;
  Label* Upgrade(Privilege*) const;
  Label* Downgrade(Privilege*) const;
  bool Contains(DisjunctionSet&) const;
  void RemoveRolesSubsumedBy(DisjunctionSet&);

  DEFINE_INLINE_TRACE() {}

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

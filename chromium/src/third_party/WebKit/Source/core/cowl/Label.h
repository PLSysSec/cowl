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

#include "platform/heap/Handle.h"
#include "bindings/core/v8/ScriptWrappable.h"
#include "bindings/core/v8/ExceptionState.h"
#include "platform/wtf/Forward.h"

namespace blink {

enum class COWLPrincipalType {
  kAppPrincipal,
  kUniquePrincipal,
  kOriginPrincipal,
  kInvalidPrincipal,
};

class COWLPrincipal final {
 public:
  COWLPrincipal(const String& principal, const COWLPrincipalType principal_type)
    : principal_(principal), principal_type_(principal_type) {}

  bool IsOriginPrincipal() const {
    return principal_type_ == COWLPrincipalType::kOriginPrincipal;
  }
  String ToString() const {
    return principal_;
  }
  bool operator== (const COWLPrincipal& other) const {
    return principal_ == other.principal_;
  }
  bool operator< (const COWLPrincipal& other) const {
    return CodePointCompareLessThan(principal_, other.principal_);
  }

 private:
  String principal_;
  COWLPrincipalType principal_type_;
};

typedef Vector<COWLPrincipal> DisjunctionSet;
typedef Vector<DisjunctionSet> DisjunctionSetArray;

class Label final : public GarbageCollectedFinalized<Label>,
                    public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static Label* Create() { return new Label; }
  static Label* Create (const String& principal);
  bool equals(Label*) const;
  bool subsumes(Label*) const;
  Label* and_(const String& principal) const;
  Label* and_(Label*) const;
  Label* or_(const String& principal) const;
  Label* or_(Label*) const;
  bool isEmpty() const { return !roles_.size(); }
  Label* clone() const { return new Label(roles_); }
  String toString() const;

  // Helper functions
  void InternalAnd(DisjunctionSet&, bool clone = false);
  void InternalOr(DisjunctionSet&);
  bool Contains(DisjunctionSet&) const;
  void RemoveRolesSubsumedBy(DisjunctionSet&);

  DEFINE_INLINE_TRACE() {}

 public:  // XXX TODO make private, unsafe
  DisjunctionSetArray* GetDirectRoles() { return &roles_; }

 private:
  Label() {}
  explicit Label(const DisjunctionSetArray& roles) : roles_(roles) {}
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

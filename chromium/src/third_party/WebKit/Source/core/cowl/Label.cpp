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

#include "core/cowl/Label.h"

#include <algorithm>
#include <iterator>
#include "core/cowl/COWLParser.h"
#include "core/cowl/Privilege.h"

namespace blink {

Label* Label::Create(ExceptionState& exception_state) {
  return new Label();
}

Label* Label::Create(const String& principal, ExceptionState& exception_state) {
  COWLPrincipalType principal_type = COWLParser::ValidatePrincipal(principal);

  if (principal_type == COWLPrincipalType::kInvalidPrincipal) {
    exception_state.ThrowTypeError("Invalid principal");
    return nullptr;
  }
  COWLPrincipal new_principal = COWLPrincipal(principal, principal_type);
  DisjunctionSet role = DisjunctionSetUtils::ConstructDset(new_principal);

  Label* label = Label::Create();
  label->roles_.push_back(role);
  return label;
}

Label* Label::Create(const DisjunctionSetArray roles) {
  return new Label(roles);
}

Label::Label() {}

Label::Label(const DisjunctionSetArray& roles) : roles_(roles) {}

Label::Label(const Label& other) {
  for (unsigned i = 0; i < other.roles_.size(); ++i) {
    DisjunctionSet dset = DisjunctionSetUtils::CloneDset(other.roles_[i]);
    roles_.push_back(dset);
  }
}

bool Label::equals(Label* other) const {
  // Break out early if the other and this are the same.
  if (other == this)
    return true;

  DisjunctionSetArray* other_roles = other->GetDirectRoles();

  // The other label is of a different size.
  if (other_roles->size() != roles_.size())
    return false;

  for (unsigned i = 0; i < roles_.size(); ++i) {
    /* This label contains a role that the other label does not, hence
     * they cannot be equal. */
    if (!other_roles->Contains(roles_[i]))
      return false;
  }
  return true;
}

bool Label::subsumes(Label* other) const {
  // Break out early if the other and this are the same.
  if (other == this)
    return true;

  DisjunctionSetArray* other_roles = other->GetDirectRoles();

  /* There are more roles in the other formula, this label cannot
   * imply (subsume) it. */
  if (other_roles->size() >  roles_.size())
    return false;

  for (unsigned i = 0; i < other_roles->size(); ++i) {
    /* The other label has a role (the ith role) that no role in this
     * label subsumes. */
    if (!Contains(other_roles->at(i)))
      return false;
  }
  return true;
}

bool Label::subsumes(Label* other, Privilege* priv) const {
  return and_(priv->asLabel())->subsumes(other);
}

Label* Label::and_(const String& principal, ExceptionState& exception_state) const {
  Label* label = Create(principal, exception_state);
  if (!label)
    return nullptr;
  return and_(label);
}

Label* Label::and_(Label* label) const {
  Label* _this = Clone();

  DisjunctionSetArray* other_roles = label->GetDirectRoles();
  for (unsigned i = 0; i < other_roles->size(); ++i) {
    _this->InternalAnd(other_roles->at(i));
  }
  return _this;
}

Label* Label::or_(const String& principal, ExceptionState& exception_state) const {
  if (IsEmpty())
    return nullptr;

  Label* label = Create(principal, exception_state);
  if (!label)
    return nullptr;
  return or_(label);
}

Label* Label::or_(Label* label) const {
  Label* _this = Clone();

  DisjunctionSetArray* other_roles = label->GetDirectRoles();
  for (unsigned i = 0; i < other_roles->size(); ++i) {
    _this->InternalOr(other_roles->at(i));
  }
  return _this;
}

String Label::toString() const {
  if (IsEmpty())
    return "'none'";

  size_t roles_length = roles_.size();
  String retval;
  if (roles_length > 1)
    retval = "(";
  else
    retval = "";

  for (size_t i = 0; i < roles_length; i++) {
    String role = DisjunctionSetUtils::ToString(roles_[i]);
    retval.append(role);
    if (i != (roles_.size() -1))
      retval.append(") AND (");
  }
  if (roles_length > 1)
    retval.append(")");
  else
    retval.append("");

  return retval;
}

// Helper functions
void Label::InternalAnd(DisjunctionSet& role, bool clone) {
  /* If there is no role in this label that subsumes |role|, append it
   * and remove any roles it subsumes.  An empty role is ignored.  */
  if (!Contains(role)) {
    // Remove any elements that this role subsumes
    RemoveRolesSubsumedBy(role);

    if (clone) {
      DisjunctionSet copy_dset = DisjunctionSetUtils::CloneDset(role);
      roles_.push_back(copy_dset);
    } else {
      roles_.push_back(role);
    }
  }
}

void Label::InternalOr(DisjunctionSet& role) {
  if (IsEmpty())
    return;

  /* Create a new label to which we'll add new roles containing the
   * above role. This eliminates the need to first do the disjunction
   * and then reduce the label to conjunctive normal form. */
  Label tmp_label;

  for (unsigned i = 0; i < roles_.size(); ++i) {
    DisjunctionSet& n_role = roles_.at(i);
    DisjunctionSetUtils::Or(n_role, role);

    tmp_label.InternalAnd(n_role);
  }
  roles_.swap(*(tmp_label.GetDirectRoles()));
}

bool Label::IsEmpty() const {
  return !roles_.size();
}

Label* Label::Clone() const {
  return new Label(*this);
}

Label* Label::Upgrade(Privilege* priv) const {
  return this->and_(priv->asLabel());
}

Label* Label::Downgrade(Privilege* priv) const {
  Label* new_label = Label::Create();

  Label* priv_label = priv->asLabel();
  for (unsigned i = 0; i < roles_.size(); ++i) {
    DisjunctionSet role = roles_[i];
    Label curr;
    curr.roles_.push_back(role);

    if (!priv_label->subsumes(&curr))
      new_label->InternalAnd(role, true);
  }
  return new_label;
}

bool Label::Contains(DisjunctionSet& role) const {
  for (unsigned i = 0; i < roles_.size(); ++i) {
    // find at least one role that subsumes the argument
    if (DisjunctionSetUtils::Subsumes(roles_[i], role))
      return true;
  }
  return false;
}

void Label::RemoveRolesSubsumedBy(DisjunctionSet& role) {
  auto pred = [&role] (DisjunctionSet& dset) {
    return DisjunctionSetUtils::Subsumes(dset, role);
  };
  auto new_last = std::remove_if(roles_.begin(), roles_.end(), pred);
  DisjunctionSetArray new_roles;
  std::copy(roles_.begin(), new_last, std::back_inserter(new_roles));
  roles_.swap(new_roles);
}

DisjunctionSetArray Label::GetRoles() { return roles_; }

// Util functions
DisjunctionSet DisjunctionSetUtils::ConstructDset() {
  DisjunctionSet dset;
  return dset;
}

DisjunctionSet DisjunctionSetUtils::ConstructDset(const COWLPrincipal& principal) {
  DisjunctionSet dset;
  dset.push_back(principal);
  return dset;
}

DisjunctionSet DisjunctionSetUtils::CloneDset(const DisjunctionSet& dset) {
  DisjunctionSet copy_dset;
  for (COWLPrincipal p : dset) {
    copy_dset.push_back(p);
  }
  return copy_dset;
}

// TODO: verify vector is sorted
bool DisjunctionSetUtils::Equals(
    const DisjunctionSet& dset1,
    const DisjunctionSet& dset2) {
  if (&dset1 == &dset2)
    return true;

  // The other role is of a different size, can't be equal.
  if (dset2.size() != dset1.size())
    return false;

  for (unsigned i = 0; i < dset1.size(); ++i) {
    /* This role contains a principal that the other role does not,
     * hence it cannot be equal to it. */
    if (dset1[i].ToString() != dset2[i].ToString())
      return false;
  }
  return true;
}

bool DisjunctionSetUtils::Subsumes(
    const DisjunctionSet& dset1,
    const DisjunctionSet& dset2) {
  if (&dset1 == &dset2)
    return true;

  // dset2 (Disjunction set) is smaller, dset1 cannot imply (subsume) it.
  if (dset2.size() < dset1.size())
    return false;

  for (const COWLPrincipal& p : dset1) {
    /* This role contains a principal that the other role does not,
     * hence it cannot imply (subsume) it. */
    if (!dset2.Contains(p))
      return false;
  }
  return true;
}

void DisjunctionSetUtils::Or(DisjunctionSet& dset1, DisjunctionSet& dset2) {
  for (const COWLPrincipal& p : dset2) {
    if (!dset1.Contains(p)) {
      DisjunctionSetUtils::InsertSorted(dset1, p);
    }
  }
}

void DisjunctionSetUtils::InsertSorted(
    DisjunctionSet& dset,
    const COWLPrincipal& principal) {
  auto it = std::lower_bound(dset.begin(), dset.end(), principal);
  size_t position = std::distance(dset.begin(), it);
  dset.insert(position, principal);
}

bool DisjunctionSetUtils::ContainsOriginPrincipal(const DisjunctionSet& dset) {
  bool found_origin_principal = false;
  for (const COWLPrincipal& p : dset) {
    if (p.IsOriginPrincipal()) {
      found_origin_principal = true;
      break;
    }
  }
  return found_origin_principal;
}

String DisjunctionSetUtils::ToString(const DisjunctionSet& dset) {
  String retval = "";
  for (size_t i = 0; i < dset.size(); ++i) {
    const COWLPrincipal& principal = dset[i];
    retval.append(principal.ToString());

    if (i != (dset.size() - 1))
      retval.append(" OR ");
  }
  retval.append("");
  return retval;
}

}  // namespace blink

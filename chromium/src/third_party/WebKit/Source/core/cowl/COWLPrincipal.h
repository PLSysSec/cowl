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

#ifndef COWLPrincipal_h
#define COWLPrincipal_h

#include "platform/wtf/text/WTFString.h"

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

}  // namespace blink

#endif  // COWLPrincipal_h

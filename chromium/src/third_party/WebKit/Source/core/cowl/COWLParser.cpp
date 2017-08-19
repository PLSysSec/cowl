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

#include "core/cowl/COWLParser.h"

#include "core/cowl/Label.h"
#include "core/cowl/Privilege.h"
#include "platform/network/ContentSecurityPolicyParsers.h"
#include "platform/UUID.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/text/ParsingUtilities.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {


static bool IsAllowedScheme(String scheme) {
  return (EqualIgnoringASCIICase("https", scheme) || EqualIgnoringASCIICase("http", scheme));
}

// unique-principal-expression = "unique:" UUID
static bool IsUniquePrincipal(String principal) {
  return (principal.StartsWithIgnoringCase("unique:") && IsValidUUID(principal.Substring(7)));
}

// app-principal-expression = "app:" 1*( ALPHA / DIGIT / "-" )
static bool IsAppPrincipal(String principal) {
  if (principal.StartsWithIgnoringCase("app:")) {
    String app = principal.Substring(4);
    Vector<UChar> characters;
    app.AppendTo(characters);

    const UChar *start = characters.data();
    const UChar *end = start + characters.size();

    if (start == end)
      return false;

    SkipWhile<UChar, IsHostCharacter>(start, end);

    if (start == end)
      return true;
  }
  return false;
}

// origin-principal-expression = "'self'" / host-source
static bool IsOriginPrincipal(String principal) {
  if (principal == "'self'")
    return true;

  KURL kurl = KURL(NullURL(), principal);
  String origin = SecurityOrigin::Create(kurl)->ToString();
  if (kurl.IsValid() && IsAllowedScheme(kurl.Protocol()) && EqualIgnoringASCIICase(origin, principal))
    return true;

  return false;
}

// principal-expression = origin-principal-expression 
//                      / app-principal-expression 
//                      / unique-principal-expression
COWLPrincipalType COWLParser::ValidatePrincipal(const String& principal) {
  if (IsUniquePrincipal(principal))
    return COWLPrincipalType::kUniquePrincipal;

  if (IsAppPrincipal(principal))
    return COWLPrincipalType::kAppPrincipal;

  if (IsOriginPrincipal(principal))
    return COWLPrincipalType::kOriginPrincipal;

  return COWLPrincipalType::kInvalidPrincipal;
}


// label-expression = empty-label / and-expression / or-expression / principal-expression
// and-expression   = *WSP "(" or-expression *WSP ")" *( 1*WSP "AND" WSP and-expression )
// or-expression    = *WSP principal-expression *( 1*WSP "OR" WSP or-expression ) 
// empty-label      = "'none'"
Label* COWLParser::ParseLabelExpression(const String& principal, const String& self_url) {
  Label* label = Label::Create();

  String expr = principal.SimplifyWhiteSpace();

  if (expr == "'none'")
    return label;

  Vector<String> ands;
  expr.Split("AND", ands);

  for (unsigned i = 0; i < ands.size(); ++i) {
    String and_exp = ands[i].SimplifyWhiteSpace();

    if (ands.size() > 1) {

      if (!and_exp.StartsWith('(') || !and_exp.EndsWith(')'))
        return nullptr;

      and_exp.Remove(0, 1); and_exp.Remove(and_exp.length() - 1, 1);
    }

    Label* or_exp = nullptr;

    Vector<String> ors;
    and_exp.Split("OR", ors);

    for (unsigned i = 0; i < ors.size(); ++i) {
      String prin = ors[i].SimplifyWhiteSpace();

      if (prin == "'self'")
        prin = self_url;

      prin.Ensure16Bit();

      if (!or_exp)
        or_exp = Label::Create(prin, ASSERT_NO_EXCEPTION);
      else
        or_exp = or_exp->or_(prin, ASSERT_NO_EXCEPTION);

      if (!or_exp)
        return nullptr;
    }
    label = label->and_(or_exp);
  }
  return label;
}

// data-metadata       = data-directive *( ";" [ data-directive ] )
// data-directive      = *WSP data-directive-name 1*WSP label-expression
// data-directive-name = "data-confidentiality" / "data-integrity"
void COWLParser::parseLabeledDataHeader(const String& expr, const String& self_url, Label*& out_conf, Label*& out_int) {
  Label* confidentiality = nullptr;
  Label* integrity = nullptr;

  Vector<String> tokens;
  expr.Split(';', tokens);
  for (unsigned i = 0; i < tokens.size(); ++i) {
    String tok = tokens[i].SimplifyWhiteSpace();

    size_t space_index = tok.find(' ');
    if (space_index == kNotFound) break;

    String directive_name = tok.Substring(0, space_index);
    String directive_value = tok.Substring(space_index);

    Label* label = COWLParser::ParseLabelExpression(directive_value, self_url);

    if (directive_name == "data-confidentiality" && !confidentiality) {
      confidentiality = label;
    } else if (directive_name == "data-integrity" && !integrity) {
      integrity = label;
    } else {
      break; // report an error?
    }
  }
  out_conf = confidentiality;
  out_int = integrity;
}

// ctx-metadata       = ctx-directive *( ";" [ ctx-directive ] )
// ctx-directive      = *WSP ctx-directive-name 1*WSP label-expression
// ctx-directive-name = "ctx-confidentiality" / "ctx-integrity" / "ctx-privilege"
void COWLParser::parseLabeledContextHeader(const String& expr, const String& self_url, Label*& out_conf, Label*& out_int, Privilege*& out_priv) {
  Label* confidentiality = nullptr;
  Label* integrity = nullptr;
  Privilege* privilege = nullptr;

  Vector<String> tokens;
  expr.Split(';', tokens);

  for (unsigned i = 0; i < tokens.size(); ++i) {
    String tok = tokens[i].SimplifyWhiteSpace();

    size_t space_index = tok.find(' ');
    if (space_index == kNotFound)
      break; // report an error?

    String directive_name = tok.Substring(0, space_index);
    String directive_value = tok.Substring(space_index);

    Label* label = COWLParser::ParseLabelExpression(directive_value, self_url);

    if (directive_name == "ctx-confidentiality" && !confidentiality) {
      confidentiality = label;
    } else if (directive_name == "ctx-integrity" && !integrity) {
      integrity = label;
    } else if (directive_name == "ctx-privilege" && !privilege && label) {
      privilege = Privilege::Create(label);
    } else {
      break; // report an error?
    }
  }
  out_conf = confidentiality;
  out_int = integrity;
  out_priv = privilege;
}

}  // namespace blink

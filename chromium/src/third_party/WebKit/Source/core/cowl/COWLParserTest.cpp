// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/cowl/COWLParser.h"

#include "core/cowl/Label.h"
#include "core/cowl/Privilege.h"
#include "platform/wtf/text/WTFString.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(COWLParserTest, ValidatePrincipal) {
  String principal;
  struct TestCase {
    String principal;
    COWLPrincipalType expected;
  } cases[] = {
    // Valid unique principals
    {"unique:a0281e1f-8412-4068-a7ed-e3f234d7fd5a", COWLPrincipalType::kUniquePrincipal},

    // Invalid unique principals
    {"unique:123213-invalid", COWLPrincipalType::kInvalidPrincipal},
    {"unique:", COWLPrincipalType::kInvalidPrincipal},

    // Valid app principals
    {"app:user1", COWLPrincipalType::kAppPrincipal},

    // Invalid app principals
    {"app:user1.", COWLPrincipalType::kInvalidPrincipal},
    {"app:", COWLPrincipalType::kInvalidPrincipal},

    // Valid origin principals
    {"'self'", COWLPrincipalType::kOriginPrincipal},
    {"https://a.com", COWLPrincipalType::kOriginPrincipal},
    {"https://a.com:1234", COWLPrincipalType::kOriginPrincipal},
    {"https://a", COWLPrincipalType::kOriginPrincipal},
    {"HTTPS://A.COM", COWLPrincipalType::kOriginPrincipal},
    {"http://a.com", COWLPrincipalType::kOriginPrincipal},

    // Invalid origins principals
    {"https:a.com", COWLPrincipalType::kInvalidPrincipal},
    {"https//a.com", COWLPrincipalType::kInvalidPrincipal},
    {"https:/a.com", COWLPrincipalType::kInvalidPrincipal},
    {"https://a.com/", COWLPrincipalType::kInvalidPrincipal},
    {"a.com", COWLPrincipalType::kInvalidPrincipal},
    {"ftp://a.com", COWLPrincipalType::kInvalidPrincipal},
  };
  for (const auto& test : cases) {
    EXPECT_EQ(test.expected, COWLParser::ValidatePrincipal(test.principal))
      << "COWLParser::ValidatePrincipal fail to parse: " << test.principal;
  }
}

TEST(COWLParserTest, ParseLabelExpression) {
  String expr, url, expected;
  Label *label;

  url = "https://a.com";

  // Valid expressions
  expr = "  'none'  ";
  expected = "'none'";
  label = COWLParser::ParseLabelExpression(expr, url);
  EXPECT_TRUE(label);
  if (label) EXPECT_EQ(label->toString(), expected);

  expr = " https://b.com  ";
  expected = "https://b.com";
  label = COWLParser::ParseLabelExpression(expr, url);
  EXPECT_TRUE(label);
  if (label) EXPECT_EQ(label->toString(), expected);

  expr = " 'self' OR https://b.com  ";
  expected = "https://a.com OR https://b.com";
  label = COWLParser::ParseLabelExpression(expr, url);
  EXPECT_TRUE(label);
  if (label) EXPECT_EQ(label->toString(), expected);

  expr = "  (  https://b.com   OR   app:user1  )   AND   (  'self'   OR   unique:a0281e1f-8412-4068-a7ed-e3f234d7fd5a  )  ";
  expected = "(app:user1 OR https://b.com) AND (https://a.com OR unique:a0281e1f-8412-4068-a7ed-e3f234d7fd5a)";
  label = COWLParser::ParseLabelExpression(expr, url);
  EXPECT_TRUE(label);
  if (label) EXPECT_EQ(label->toString(), expected);

  // Invalid: missing parentheses
  expr = " 'self' OR https://b.com  AND   https://c.com";
  expected = "";
  label = COWLParser::ParseLabelExpression(expr, url);
  EXPECT_FALSE(label);

  // Invalid: one principal is not valid
  expr = "  (  https://b.edu   OR   app:user1  )   AND   (  'self'   OR   unique:a0281e1f-invalid  )  ";
  expected = "";
  label = COWLParser::ParseLabelExpression(expr, url);
  EXPECT_FALSE(label);
}

TEST(COWLParserTest, ParseLabeledDataHeader) {
  String expr, url, expected;
  Label *conf, *integrity;

  expr = "data-confidentiality ('self') AND (https://b.com);"
         "data-integrity 'self'";
  url = "https://a.com";
  expected = "(https://a.com) AND (https://b.com)";
  conf = integrity = nullptr;
  COWLParser::parseLabeledDataHeader(expr, url, conf, integrity);
  EXPECT_TRUE(conf);
  EXPECT_TRUE(integrity);
  if (conf) EXPECT_EQ(conf->toString(), expected);
  if (integrity) EXPECT_EQ(integrity->toString(), "https://a.com");

  expr = "data-confidentiality app:user1;"
         "data-integrity b.com";
  url = "https://a.com";
  expected = "";
  conf = integrity = nullptr;
  COWLParser::parseLabeledDataHeader(expr, url, conf, integrity);
  EXPECT_TRUE(conf);
  EXPECT_FALSE(integrity);
  if (conf) EXPECT_EQ(conf->toString(), "app:user1");
}

TEST(COWLParserTest, ParseLabeledContextHeader) {
  String expr, url, expected;
  Label *conf, *integrity;
  Privilege *priv;

  expr = "ctx-confidentiality 'none';"
         "ctx-integrity 'self';"
         "ctx-privilege (https://university.edu OR app:user1) AND (unique:a0281e1f-8412-4068-a7ed-e3f234d7fd5a)";
  url = "https://a.com";
  expected = "(app:user1 OR https://university.edu) AND (unique:a0281e1f-8412-4068-a7ed-e3f234d7fd5a)";
  conf = integrity = nullptr;
  priv = nullptr;
  COWLParser::parseLabeledContextHeader(expr, url, conf, integrity, priv);
  EXPECT_TRUE(conf);
  EXPECT_TRUE(integrity);
  EXPECT_TRUE(priv);
  if (conf) EXPECT_EQ(conf->toString(), "'none'");
  if (integrity) EXPECT_EQ(integrity->toString(), "https://a.com");
  if (priv) EXPECT_EQ(priv->asLabel()->toString(), expected);
}

}  // namespace blink

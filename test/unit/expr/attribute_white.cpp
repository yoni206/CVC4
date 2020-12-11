/*********************                                                        */
/*! \file attribute_white.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Aina Niemetz, Morgan Deters
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief White box testing of Node attributes.
 **
 ** White box testing of Node attributes.
 **/

#include <string>

#include "base/check.h"
#include "expr/attribute.h"
#include "expr/node.h"
#include "expr/node_builder.h"
#include "expr/node_manager.h"
#include "expr/node_manager_attributes.h"
#include "expr/node_value.h"
#include "smt/smt_engine.h"
#include "smt/smt_engine_scope.h"
#include "test_expr.h"
#include "theory/theory.h"
#include "theory/theory_engine.h"
#include "theory/uf/theory_uf.h"

namespace CVC4 {

using namespace kind;
using namespace smt;
using namespace expr;
using namespace expr::attr;

namespace test {

struct Test1;
struct Test2;
struct Test3;
struct Test4;
struct Test5;

typedef Attribute<Test1, std::string> TestStringAttr1;
typedef Attribute<Test2, std::string> TestStringAttr2;

using TestFlag1 = Attribute<Test1, bool>;
using TestFlag2 = Attribute<Test2, bool>;
using TestFlag3 = Attribute<Test3, bool>;
using TestFlag4 = Attribute<Test4, bool>;
using TestFlag5 = Attribute<Test5, bool>;

class TestExprWhiteAttribute : public TestExprWhite
{
 protected:
  void SetUp() override
  {
    TestExprWhite::SetUp();
    d_booleanType.reset(new TypeNode(d_nodeManager->booleanType()));
  }
  std::unique_ptr<TypeNode> d_booleanType;
};

TEST_F(TestExprWhiteAttribute, attribute_ids)
{
  // Test that IDs for (a subset of) attributes in the system are
  // unique and that the LastAttributeId (which would be the next ID
  // to assign) is greater than all attribute IDs.

  // We used to check ID assignments explicitly.  However, between
  // compilation modules, you don't get a strong guarantee
  // (initialization order is somewhat implementation-specific, and
  // anyway you'd have to change the tests anytime you add an
  // attribute).  So we back off, and just test that they're unique
  // and that the next ID to be assigned is strictly greater than
  // those that have already been assigned.

  unsigned lastId = attr::LastAttributeId<std::string, false>::getId();
  EXPECT_LT(VarNameAttr::s_id, lastId);
  EXPECT_LT(TestStringAttr1::s_id, lastId);
  EXPECT_LT(TestStringAttr2::s_id, lastId);

  EXPECT_NE(VarNameAttr::s_id, TestStringAttr1::s_id);
  EXPECT_NE(VarNameAttr::s_id, TestStringAttr2::s_id);
  EXPECT_NE(TestStringAttr1::s_id, TestStringAttr2::s_id);

  lastId = attr::LastAttributeId<bool, false>::getId();
  EXPECT_LT(TestFlag1::s_id, lastId);
  EXPECT_LT(TestFlag2::s_id, lastId);
  EXPECT_LT(TestFlag3::s_id, lastId);
  EXPECT_LT(TestFlag4::s_id, lastId);
  EXPECT_LT(TestFlag5::s_id, lastId);
  EXPECT_NE(TestFlag1::s_id, TestFlag2::s_id);
  EXPECT_NE(TestFlag1::s_id, TestFlag3::s_id);
  EXPECT_NE(TestFlag1::s_id, TestFlag4::s_id);
  EXPECT_NE(TestFlag1::s_id, TestFlag5::s_id);
  EXPECT_NE(TestFlag2::s_id, TestFlag3::s_id);
  EXPECT_NE(TestFlag2::s_id, TestFlag4::s_id);
  EXPECT_NE(TestFlag2::s_id, TestFlag5::s_id);
  EXPECT_NE(TestFlag3::s_id, TestFlag4::s_id);
  EXPECT_NE(TestFlag3::s_id, TestFlag5::s_id);
  EXPECT_NE(TestFlag4::s_id, TestFlag5::s_id);

  lastId = attr::LastAttributeId<TypeNode, false>::getId();
  EXPECT_LT(TypeAttr::s_id, lastId);
}

TEST_F(TestExprWhiteAttribute, attributes)
{
  Node a = d_nodeManager->mkVar(*d_booleanType);
  Node b = d_nodeManager->mkVar(*d_booleanType);
  Node c = d_nodeManager->mkVar(*d_booleanType);
  Node unnamed = d_nodeManager->mkVar(*d_booleanType);

  a.setAttribute(VarNameAttr(), "a");
  b.setAttribute(VarNameAttr(), "b");
  c.setAttribute(VarNameAttr(), "c");

  // test that all boolean flags are FALSE to start
  Debug("boolattr") << "get flag 1 on a (should be F)\n";
  ASSERT_FALSE(a.getAttribute(TestFlag1()));
  Debug("boolattr") << "get flag 1 on b (should be F)\n";
  ASSERT_FALSE(b.getAttribute(TestFlag1()));
  Debug("boolattr") << "get flag 1 on c (should be F)\n";
  ASSERT_FALSE(c.getAttribute(TestFlag1()));
  Debug("boolattr") << "get flag 1 on unnamed (should be F)\n";
  ASSERT_FALSE(unnamed.getAttribute(TestFlag1()));

  Debug("boolattr") << "get flag 2 on a (should be F)\n";
  ASSERT_FALSE(a.getAttribute(TestFlag2()));
  Debug("boolattr") << "get flag 2 on b (should be F)\n";
  ASSERT_FALSE(b.getAttribute(TestFlag2()));
  Debug("boolattr") << "get flag 2 on c (should be F)\n";
  ASSERT_FALSE(c.getAttribute(TestFlag2()));
  Debug("boolattr") << "get flag 2 on unnamed (should be F)\n";
  ASSERT_FALSE(unnamed.getAttribute(TestFlag2()));

  Debug("boolattr") << "get flag 3 on a (should be F)\n";
  ASSERT_FALSE(a.getAttribute(TestFlag3()));
  Debug("boolattr") << "get flag 3 on b (should be F)\n";
  ASSERT_FALSE(b.getAttribute(TestFlag3()));
  Debug("boolattr") << "get flag 3 on c (should be F)\n";
  ASSERT_FALSE(c.getAttribute(TestFlag3()));
  Debug("boolattr") << "get flag 3 on unnamed (should be F)\n";
  ASSERT_FALSE(unnamed.getAttribute(TestFlag3()));

  Debug("boolattr") << "get flag 4 on a (should be F)\n";
  ASSERT_FALSE(a.getAttribute(TestFlag4()));
  Debug("boolattr") << "get flag 4 on b (should be F)\n";
  ASSERT_FALSE(b.getAttribute(TestFlag4()));
  Debug("boolattr") << "get flag 4 on c (should be F)\n";
  ASSERT_FALSE(c.getAttribute(TestFlag4()));
  Debug("boolattr") << "get flag 4 on unnamed (should be F)\n";
  ASSERT_FALSE(unnamed.getAttribute(TestFlag4()));

  Debug("boolattr") << "get flag 5 on a (should be F)\n";
  ASSERT_FALSE(a.getAttribute(TestFlag5()));
  Debug("boolattr") << "get flag 5 on b (should be F)\n";
  ASSERT_FALSE(b.getAttribute(TestFlag5()));
  Debug("boolattr") << "get flag 5 on c (should be F)\n";
  ASSERT_FALSE(c.getAttribute(TestFlag5()));
  Debug("boolattr") << "get flag 5 on unnamed (should be F)\n";
  ASSERT_FALSE(unnamed.getAttribute(TestFlag5()));

  // test that they all HAVE the boolean attributes
  ASSERT_TRUE(a.hasAttribute(TestFlag1()));
  ASSERT_TRUE(b.hasAttribute(TestFlag1()));
  ASSERT_TRUE(c.hasAttribute(TestFlag1()));
  ASSERT_TRUE(unnamed.hasAttribute(TestFlag1()));

  ASSERT_TRUE(a.hasAttribute(TestFlag2()));
  ASSERT_TRUE(b.hasAttribute(TestFlag2()));
  ASSERT_TRUE(c.hasAttribute(TestFlag2()));
  ASSERT_TRUE(unnamed.hasAttribute(TestFlag2()));

  ASSERT_TRUE(a.hasAttribute(TestFlag3()));
  ASSERT_TRUE(b.hasAttribute(TestFlag3()));
  ASSERT_TRUE(c.hasAttribute(TestFlag3()));
  ASSERT_TRUE(unnamed.hasAttribute(TestFlag3()));

  ASSERT_TRUE(a.hasAttribute(TestFlag4()));
  ASSERT_TRUE(b.hasAttribute(TestFlag4()));
  ASSERT_TRUE(c.hasAttribute(TestFlag4()));
  ASSERT_TRUE(unnamed.hasAttribute(TestFlag4()));

  ASSERT_TRUE(a.hasAttribute(TestFlag5()));
  ASSERT_TRUE(b.hasAttribute(TestFlag5()));
  ASSERT_TRUE(c.hasAttribute(TestFlag5()));
  ASSERT_TRUE(unnamed.hasAttribute(TestFlag5()));

  // test two-arg version of hasAttribute()
  bool bb = false;
  Debug("boolattr") << "get flag 1 on a (should be F)\n";
  ASSERT_TRUE(a.getAttribute(TestFlag1(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 1 on b (should be F)\n";
  ASSERT_TRUE(b.getAttribute(TestFlag1(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 1 on c (should be F)\n";
  ASSERT_TRUE(c.getAttribute(TestFlag1(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 1 on unnamed (should be F)\n";
  ASSERT_TRUE(unnamed.getAttribute(TestFlag1(), bb));
  ASSERT_FALSE(bb);

  Debug("boolattr") << "get flag 2 on a (should be F)\n";
  ASSERT_TRUE(a.getAttribute(TestFlag2(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 2 on b (should be F)\n";
  ASSERT_TRUE(b.getAttribute(TestFlag2(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 2 on c (should be F)\n";
  ASSERT_TRUE(c.getAttribute(TestFlag2(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 2 on unnamed (should be F)\n";
  ASSERT_TRUE(unnamed.getAttribute(TestFlag2(), bb));
  ASSERT_FALSE(bb);

  Debug("boolattr") << "get flag 3 on a (should be F)\n";
  ASSERT_TRUE(a.getAttribute(TestFlag3(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 3 on b (should be F)\n";
  ASSERT_TRUE(b.getAttribute(TestFlag3(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 3 on c (should be F)\n";
  ASSERT_TRUE(c.getAttribute(TestFlag3(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 3 on unnamed (should be F)\n";
  ASSERT_TRUE(unnamed.getAttribute(TestFlag3(), bb));
  ASSERT_FALSE(bb);

  Debug("boolattr") << "get flag 4 on a (should be F)\n";
  ASSERT_TRUE(a.getAttribute(TestFlag4(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 4 on b (should be F)\n";
  ASSERT_TRUE(b.getAttribute(TestFlag4(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 4 on c (should be F)\n";
  ASSERT_TRUE(c.getAttribute(TestFlag4(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 4 on unnamed (should be F)\n";
  ASSERT_TRUE(unnamed.getAttribute(TestFlag4(), bb));
  ASSERT_FALSE(bb);

  Debug("boolattr") << "get flag 5 on a (should be F)\n";
  ASSERT_TRUE(a.getAttribute(TestFlag5(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 5 on b (should be F)\n";
  ASSERT_TRUE(b.getAttribute(TestFlag5(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 5 on c (should be F)\n";
  ASSERT_TRUE(c.getAttribute(TestFlag5(), bb));
  ASSERT_FALSE(bb);
  Debug("boolattr") << "get flag 5 on unnamed (should be F)\n";
  ASSERT_TRUE(unnamed.getAttribute(TestFlag5(), bb));
  ASSERT_FALSE(bb);

  // setting boolean flags
  Debug("boolattr") << "set flag 1 on a to T\n";
  a.setAttribute(TestFlag1(), true);
  Debug("boolattr") << "set flag 1 on b to F\n";
  b.setAttribute(TestFlag1(), false);
  Debug("boolattr") << "set flag 1 on c to F\n";
  c.setAttribute(TestFlag1(), false);
  Debug("boolattr") << "set flag 1 on unnamed to T\n";
  unnamed.setAttribute(TestFlag1(), true);

  Debug("boolattr") << "set flag 2 on a to F\n";
  a.setAttribute(TestFlag2(), false);
  Debug("boolattr") << "set flag 2 on b to T\n";
  b.setAttribute(TestFlag2(), true);
  Debug("boolattr") << "set flag 2 on c to T\n";
  c.setAttribute(TestFlag2(), true);
  Debug("boolattr") << "set flag 2 on unnamed to F\n";
  unnamed.setAttribute(TestFlag2(), false);

  Debug("boolattr") << "set flag 3 on a to T\n";
  a.setAttribute(TestFlag3(), true);
  Debug("boolattr") << "set flag 3 on b to T\n";
  b.setAttribute(TestFlag3(), true);
  Debug("boolattr") << "set flag 3 on c to T\n";
  c.setAttribute(TestFlag3(), true);
  Debug("boolattr") << "set flag 3 on unnamed to T\n";
  unnamed.setAttribute(TestFlag3(), true);

  Debug("boolattr") << "set flag 4 on a to T\n";
  a.setAttribute(TestFlag4(), true);
  Debug("boolattr") << "set flag 4 on b to T\n";
  b.setAttribute(TestFlag4(), true);
  Debug("boolattr") << "set flag 4 on c to T\n";
  c.setAttribute(TestFlag4(), true);
  Debug("boolattr") << "set flag 4 on unnamed to T\n";
  unnamed.setAttribute(TestFlag4(), true);

  Debug("boolattr") << "set flag 5 on a to T\n";
  a.setAttribute(TestFlag5(), true);
  Debug("boolattr") << "set flag 5 on b to T\n";
  b.setAttribute(TestFlag5(), true);
  Debug("boolattr") << "set flag 5 on c to F\n";
  c.setAttribute(TestFlag5(), false);
  Debug("boolattr") << "set flag 5 on unnamed to T\n";
  unnamed.setAttribute(TestFlag5(), true);

  EXPECT_EQ(a.getAttribute(VarNameAttr()), "a");
  EXPECT_NE(a.getAttribute(VarNameAttr()), "b");
  EXPECT_NE(a.getAttribute(VarNameAttr()), "c");
  EXPECT_NE(a.getAttribute(VarNameAttr()), "");

  EXPECT_NE(b.getAttribute(VarNameAttr()), "a");
  EXPECT_EQ(b.getAttribute(VarNameAttr()), "b");
  EXPECT_NE(b.getAttribute(VarNameAttr()), "c");
  EXPECT_NE(b.getAttribute(VarNameAttr()), "");

  EXPECT_NE(c.getAttribute(VarNameAttr()), "a");
  EXPECT_NE(c.getAttribute(VarNameAttr()), "b");
  EXPECT_EQ(c.getAttribute(VarNameAttr()), "c");
  EXPECT_NE(c.getAttribute(VarNameAttr()), "");

  EXPECT_NE(unnamed.getAttribute(VarNameAttr()), "a");
  EXPECT_NE(unnamed.getAttribute(VarNameAttr()), "b");
  EXPECT_NE(unnamed.getAttribute(VarNameAttr()), "c");
  EXPECT_EQ(unnamed.getAttribute(VarNameAttr()), "");

  ASSERT_FALSE(unnamed.hasAttribute(VarNameAttr()));

  ASSERT_FALSE(a.hasAttribute(TestStringAttr1()));
  ASSERT_FALSE(b.hasAttribute(TestStringAttr1()));
  ASSERT_FALSE(c.hasAttribute(TestStringAttr1()));
  ASSERT_FALSE(unnamed.hasAttribute(TestStringAttr1()));

  ASSERT_FALSE(a.hasAttribute(TestStringAttr2()));
  ASSERT_FALSE(b.hasAttribute(TestStringAttr2()));
  ASSERT_FALSE(c.hasAttribute(TestStringAttr2()));
  ASSERT_FALSE(unnamed.hasAttribute(TestStringAttr2()));

  Debug("boolattr") << "get flag 1 on a (should be T)\n";
  ASSERT_TRUE(a.getAttribute(TestFlag1()));
  Debug("boolattr") << "get flag 1 on b (should be F)\n";
  ASSERT_FALSE(b.getAttribute(TestFlag1()));
  Debug("boolattr") << "get flag 1 on c (should be F)\n";
  ASSERT_FALSE(c.getAttribute(TestFlag1()));
  Debug("boolattr") << "get flag 1 on unnamed (should be T)\n";
  ASSERT_TRUE(unnamed.getAttribute(TestFlag1()));

  Debug("boolattr") << "get flag 2 on a (should be F)\n";
  ASSERT_FALSE(a.getAttribute(TestFlag2()));
  Debug("boolattr") << "get flag 2 on b (should be T)\n";
  ASSERT_TRUE(b.getAttribute(TestFlag2()));
  Debug("boolattr") << "get flag 2 on c (should be T)\n";
  ASSERT_TRUE(c.getAttribute(TestFlag2()));
  Debug("boolattr") << "get flag 2 on unnamed (should be F)\n";
  ASSERT_FALSE(unnamed.getAttribute(TestFlag2()));

  Debug("boolattr") << "get flag 3 on a (should be T)\n";
  ASSERT_TRUE(a.getAttribute(TestFlag3()));
  Debug("boolattr") << "get flag 3 on b (should be T)\n";
  ASSERT_TRUE(b.getAttribute(TestFlag3()));
  Debug("boolattr") << "get flag 3 on c (should be T)\n";
  ASSERT_TRUE(c.getAttribute(TestFlag3()));
  Debug("boolattr") << "get flag 3 on unnamed (should be T)\n";
  ASSERT_TRUE(unnamed.getAttribute(TestFlag3()));

  Debug("boolattr") << "get flag 4 on a (should be T)\n";
  ASSERT_TRUE(a.getAttribute(TestFlag4()));
  Debug("boolattr") << "get flag 4 on b (should be T)\n";
  ASSERT_TRUE(b.getAttribute(TestFlag4()));
  Debug("boolattr") << "get flag 4 on c (should be T)\n";
  ASSERT_TRUE(c.getAttribute(TestFlag4()));
  Debug("boolattr") << "get flag 4 on unnamed (should be T)\n";
  ASSERT_TRUE(unnamed.getAttribute(TestFlag4()));

  Debug("boolattr") << "get flag 5 on a (should be T)\n";
  ASSERT_TRUE(a.getAttribute(TestFlag5()));
  Debug("boolattr") << "get flag 5 on b (should be T)\n";
  ASSERT_TRUE(b.getAttribute(TestFlag5()));
  Debug("boolattr") << "get flag 5 on c (should be F)\n";
  ASSERT_FALSE(c.getAttribute(TestFlag5()));
  Debug("boolattr") << "get flag 5 on unnamed (should be T)\n";
  ASSERT_TRUE(unnamed.getAttribute(TestFlag5()));

  a.setAttribute(TestStringAttr1(), "foo");
  b.setAttribute(TestStringAttr1(), "bar");
  c.setAttribute(TestStringAttr1(), "baz");

  EXPECT_EQ(a.getAttribute(VarNameAttr()), "a");
  EXPECT_NE(a.getAttribute(VarNameAttr()), "b");
  EXPECT_NE(a.getAttribute(VarNameAttr()), "c");
  EXPECT_NE(a.getAttribute(VarNameAttr()), "");

  EXPECT_NE(b.getAttribute(VarNameAttr()), "a");
  EXPECT_EQ(b.getAttribute(VarNameAttr()), "b");
  EXPECT_NE(b.getAttribute(VarNameAttr()), "c");
  EXPECT_NE(b.getAttribute(VarNameAttr()), "");

  EXPECT_NE(c.getAttribute(VarNameAttr()), "a");
  EXPECT_NE(c.getAttribute(VarNameAttr()), "b");
  EXPECT_EQ(c.getAttribute(VarNameAttr()), "c");
  EXPECT_NE(c.getAttribute(VarNameAttr()), "");

  EXPECT_NE(unnamed.getAttribute(VarNameAttr()), "a");
  EXPECT_NE(unnamed.getAttribute(VarNameAttr()), "b");
  EXPECT_NE(unnamed.getAttribute(VarNameAttr()), "c");
  EXPECT_EQ(unnamed.getAttribute(VarNameAttr()), "");

  ASSERT_FALSE(unnamed.hasAttribute(VarNameAttr()));

  ASSERT_TRUE(a.hasAttribute(TestStringAttr1()));
  ASSERT_TRUE(b.hasAttribute(TestStringAttr1()));
  ASSERT_TRUE(c.hasAttribute(TestStringAttr1()));
  ASSERT_FALSE(unnamed.hasAttribute(TestStringAttr1()));

  ASSERT_FALSE(a.hasAttribute(TestStringAttr2()));
  ASSERT_FALSE(b.hasAttribute(TestStringAttr2()));
  ASSERT_FALSE(c.hasAttribute(TestStringAttr2()));
  ASSERT_FALSE(unnamed.hasAttribute(TestStringAttr2()));

  a.setAttribute(VarNameAttr(), "b");
  b.setAttribute(VarNameAttr(), "c");
  c.setAttribute(VarNameAttr(), "a");

  EXPECT_EQ(c.getAttribute(VarNameAttr()), "a");
  EXPECT_NE(c.getAttribute(VarNameAttr()), "b");
  EXPECT_NE(c.getAttribute(VarNameAttr()), "c");
  EXPECT_NE(c.getAttribute(VarNameAttr()), "");

  EXPECT_NE(a.getAttribute(VarNameAttr()), "a");
  EXPECT_EQ(a.getAttribute(VarNameAttr()), "b");
  EXPECT_NE(a.getAttribute(VarNameAttr()), "c");
  EXPECT_NE(a.getAttribute(VarNameAttr()), "");

  EXPECT_NE(b.getAttribute(VarNameAttr()), "a");
  EXPECT_NE(b.getAttribute(VarNameAttr()), "b");
  EXPECT_EQ(b.getAttribute(VarNameAttr()), "c");
  EXPECT_NE(b.getAttribute(VarNameAttr()), "");

  EXPECT_NE(unnamed.getAttribute(VarNameAttr()), "a");
  EXPECT_NE(unnamed.getAttribute(VarNameAttr()), "b");
  EXPECT_NE(unnamed.getAttribute(VarNameAttr()), "c");
  EXPECT_EQ(unnamed.getAttribute(VarNameAttr()), "");

  ASSERT_FALSE(unnamed.hasAttribute(VarNameAttr()));
}
}  // namespace test
}  // namespace CVC4

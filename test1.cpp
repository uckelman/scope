/*
	© 2009, Jon Stewart
	Released under the terms of the Boost license (http://www.boost.org/LICENSE_1_0.txt). See License.txt for details.
*/

#include "scope/test.h"

SCOPE_TEST(simpleTest) {
  SCOPE_ASSERT(true);
}

SCOPE_TEST_BELONGS_TO(simpleTest, coolio);

SCOPE_TEST(failTest) {
  SCOPE_ASSERT(false); // ha-ha!
}

void doNothing() {
  ;
}

SCOPE_TEST(TestExpectMacro) {
  SCOPE_EXPECT(throw int(1), int);
  SCOPE_EXPECT(doNothing(), int);
}
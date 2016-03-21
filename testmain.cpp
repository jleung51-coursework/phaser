#include <cstring>
#include <iostream>

#include <UnitTest++/UnitTest++.h>
#include <UnitTest++/TestReporterStdout.h>

using std::cerr;
using std::endl;
using std::strcmp;

/*
  Function object returning true for a specified test name
 */
struct MatchTestName {
  const char * const tname;
  MatchTestName (const char* tn) : tname {tn} {};
  bool operator () (const UnitTest::Test* const test) const { return strcmp(test->m_details.testName, tname) == 0; }
};

/*
  Locate and run all tests
 */
int main(int argc, const char* argv[]) {
  if (argc < 2)
    return UnitTest::RunAllTests();
  else if (argc >= 2){
    UnitTest::TestReporterStdout reporter;
    UnitTest::TestRunner runner (reporter);
    if (argc == 2)
      return runner.RunTestsIf (UnitTest::Test::GetTestList(),
                                argv[1],
                                UnitTest::True(),
                                0);
    else if (argc == 3)
      return runner.RunTestsIf (UnitTest::Test::GetTestList(),
                                argv[1],
                                MatchTestName(argv[2]),
                                0);
    else
      cerr << "Usage: tester [suite [test]]" << endl;
  }
}

// test.cpp : Defines the entry point for the console application.
//
#include "test.h"


int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  int ret = RUN_ALL_TESTS();
  return ret;
}


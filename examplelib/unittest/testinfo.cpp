#include "examplelib/core.h"

#include <gtest/gtest.h>

TEST (examplelib, getinfo) {
  ASSERT_STREQ ((examplelib::core::getinfo()), ("Hello, World!"));
}

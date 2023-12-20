#include "common/sequence_number.h"

#include "gtest/gtest.h"

namespace {

using ::tsdb2::common::SequenceNumber;

TEST(SequenceNumberTest, First) {
  SequenceNumber sn{123};
  EXPECT_EQ(sn.GetNext(), 123);
}

TEST(SequenceNumberTest, FirstDefault) {
  SequenceNumber sn;
  EXPECT_EQ(sn.GetNext(), 1);
}

TEST(SequenceNumberTest, Next) {
  SequenceNumber sn;
  sn.GetNext();
  EXPECT_EQ(sn.GetNext(), 2);
  EXPECT_EQ(sn.GetNext(), 3);
}

}  // namespace

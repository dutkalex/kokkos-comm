//@HEADER
// ************************************************************************
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Part of Kokkos, under the Apache License v2.0 with LLVM Exceptions.
// See https://kokkos.org/LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//@HEADER

#include <gtest/gtest.h>

#include "KokkosComm.hpp"

template <typename T>
class IReduce : public testing::Test {
 public:
  using Scalar = T;
};

using ScalarTypes = ::testing::Types<int, int64_t, float, double, Kokkos::complex<float>, Kokkos::complex<double>>;
TYPED_TEST_SUITE(IReduce, ScalarTypes);

/*!
Each rank fills its sendbuf[i] with `rank + i`

operation is sum, so recvbuf[i] should be sum(0..size) + i * size

*/
TYPED_TEST(IReduce, 1D_contig) {
  using TestScalar = typename TestFixture::Scalar;

  auto comm = KokkosComm::CommWorld();
  auto sum  = KokkosComm::Sum();
  int rank  = comm.rank();
  int size  = comm.size();

  Kokkos::View<TestScalar *> sendv("sendv", 10);
  Kokkos::View<TestScalar *> recvv;
  if (0 == rank) {
    Kokkos::resize(recvv, sendv.extent(0));
  }

  // fill send buffer
  Kokkos::parallel_for(
      sendv.extent(0), KOKKOS_LAMBDA(const int i) { sendv(i) = rank + i; });

  auto request = comm.ireduce(sendv, recvv, sum, 0);
  request.wait();

  if (0 == rank) {
    int errs;
    Kokkos::parallel_reduce(
        recvv.extent(0),
        KOKKOS_LAMBDA(const int &i, int &lsum) {
          TestScalar acc = 0;
          for (int r = 0; r < size; ++r) {
            acc += r + i;
          }
          lsum += recvv(i) != acc;
        },
        errs);
    ASSERT_EQ(errs, 0);
  }
}
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
class Reduce : public testing::Test {
 public:
  using Scalar = T;
};

using ScalarTypes = ::testing::Types<int, int64_t, float, double, Kokkos::complex<float>, Kokkos::complex<double>>;
TYPED_TEST_SUITE(Reduce, ScalarTypes);

/*!
Each rank fills its sendbuf[i] with `rank + i`

operation is sum, so recvbuf[i] should be sum(0..size) + i * size

*/
TYPED_TEST(Reduce, 1D_contig) {
  using TestScalar = typename TestFixture::Scalar;

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  Kokkos::View<TestScalar *> sendv("sendv", 10);
  Kokkos::View<TestScalar *> recvv;
  if (0 == rank) {
    Kokkos::resize(recvv, sendv.extent(0));
  }

  // fill send buffer
  Kokkos::parallel_for(
      sendv.extent(0), KOKKOS_LAMBDA(const int i) { sendv(i) = rank + i; });

  KokkosComm::reduce(Kokkos::DefaultExecutionSpace(), sendv, recvv, MPI_SUM, 0, MPI_COMM_WORLD);

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
          // if (recvv(i) != acc) {
          //   Kokkos::printf("%f != %f @ %lu\n", double(Kokkos::abs(recvv(i))),
          //   double(Kokkos::abs(acc)), size_t(i));
          // }
        },
        errs);
    ASSERT_EQ(errs, 0);
  }
}

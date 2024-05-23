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

#pragma once

#include <Kokkos_Core.hpp>

#include "KokkosComm_mpi.hpp"
#include "KokkosComm_concepts.hpp"
#include "KokkosComm_pack_traits.hpp"
#include "KokkosComm_traits.hpp"

namespace KokkosComm::Impl {
template <KokkosView SendView, KokkosView RecvView>
void sendrecv(SendView sv, RecvView rv, int rank, int tag, Communicator comm) {
  Kokkos::Tools::pushRegion("KokkosComm::Impl::sendrecv");
  using SVT = typename KokkosComm::Traits<SendView>;
  using RVT = typename KokkosComm::Traits<RecvView>;

  if (not SVT::is_contiguous(sv) or not RVT::is_contiguous(rv))
    throw std::runtime_error("only contiguous views supported for low-level sendrecv");

  comm.sendrecv(sv, rv, rank, tag);
  Kokkos::Tools::popRegion();
}
}  // namespace KokkosComm::Impl
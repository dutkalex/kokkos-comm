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

#include "KokkosComm_concepts.hpp"
#include "KokkosComm_pack_traits.hpp"
#include "KokkosComm_traits.hpp"

// impl
#include "KokkosComm_include_mpi.hpp"

namespace KokkosComm::Impl {

template <KokkosView RecvView>
void recv(const RecvView &rv, int src, int tag, MPI_Comm comm, MPI_Status *status) {
  Kokkos::Tools::pushRegion("KokkosComm::Impl::recv");
  using KCT = KokkosComm::Traits<RecvView>;

  if (KCT::is_contiguous(rv)) {
    using ScalarType = typename RecvView::non_const_value_type;
    MPI_Recv(KCT::data_handle(rv), KCT::span(rv), mpi_type_v<ScalarType>, src, tag, comm, status);
  } else {
    throw std::runtime_error("only contiguous views supported for low-level recv");
  }
  Kokkos::Tools::popRegion();
}

template <KokkosExecutionSpace ExecSpace, KokkosView RecvView>
void recv(const ExecSpace &space, RecvView &rv, int src, int tag, MPI_Comm comm) {
  Kokkos::Tools::pushRegion("KokkosComm::Impl::recv");

  using KCT  = KokkosComm::Traits<RecvView>;
  using KCPT = KokkosComm::PackTraits<RecvView>;

  if (KCPT::needs_unpack(rv)) {
    using Packer = typename KCPT::packer_type;
    using Args   = typename Packer::args_type;

    Args args = Packer::allocate_packed_for(space, "packed", rv);
    space.fence();  // make sure allocation is complete before recv
    MPI_Recv(KCT::data_handle(args.view), args.count, args.datatype, src, tag, comm, MPI_STATUS_IGNORE);
    Packer::unpack_into(space, rv, args.view);
  } else {
    using RecvScalar = typename RecvView::value_type;
    space.fence();  // can't recv in space until any preceeding work is done
    recv(rv, src, tag, comm, MPI_STATUS_IGNORE);
  }

  Kokkos::Tools::popRegion();
}
}  // namespace KokkosComm::Impl

// Copyright (C) 2016-2020 by the multiphenics authors
//
// This file is part of multiphenics.
//
// multiphenics is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// multiphenics is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with multiphenics. If not, see <http://www.gnu.org/licenses/>.
//

#include <dolfinx/common/IndexMap.h>
#include <dolfinx/common/Timer.h>
#include <dolfinx/fem/assemble_matrix_impl.h>
#include <dolfinx/fem/assemble_vector_impl.h>
#include <dolfinx/la/utils.h>
#include <dolfinx/mesh/Mesh.h>
#include <multiphenics/fem/block_assemble.h>
#include <multiphenics/fem/BlockDofMap.h>
#include <multiphenics/fem/BlockSparsityPatternBuilder.h>
#include <multiphenics/la/BlockPETScSubMatrix.h>
#include <multiphenics/la/BlockPETScSubVectorWrapper.h>

using namespace multiphenics;
using namespace multiphenics::fem;

using dolfinx::common::IndexMap;
using dolfinx::common::Timer;
using dolfinx::fem::DofMap;
using dolfinx::fem::Form;
using dolfinx::fem::FormIntegrals;
using dolfinx::fem::impl::assemble_vector;
using dolfinx::fem::impl::assemble_matrix;
using dolfinx::la::create_petsc_matrix;
using dolfinx::la::create_petsc_vector;
using dolfinx::la::petsc_error;
using dolfinx::la::SparsityPattern;
using dolfinx::mesh::Mesh;
using multiphenics::la::BlockPETScSubMatrix;
using multiphenics::la::BlockPETScSubVectorWrapper;

//-----------------------------------------------------------------------------
Vec multiphenics::fem::block_assemble(const BlockForm1& L)
{
  Vec b = init_vector(L);
  block_assemble(b, L);
  return b;
}
//-----------------------------------------------------------------------------
void multiphenics::fem::block_assemble(Vec b, const BlockForm1& L)
{
  // Assemble using standard assembler
  for (unsigned int i(0); i < L.block_size(0); ++i)
  {
    const Form & L_i(L(i));
    if (
      L_i.integrals().num_integrals(FormIntegrals::Type::cell) > 0
        ||
      L_i.integrals().num_integrals(FormIntegrals::Type::interior_facet) > 0
        ||
      L_i.integrals().num_integrals(FormIntegrals::Type::exterior_facet) > 0
    )
    {
      BlockPETScSubVectorWrapper b_i(b, i, L.block_function_spaces()[0]->block_dofmap(), ADD_VALUES);
      assemble_vector(b_i.content, L_i);
    }
  }
  // Finalize assembly of global tensor
  PetscErrorCode ierr;
  ierr = VecGhostUpdateBegin(b, ADD_VALUES, SCATTER_REVERSE);
  if (ierr != 0) petsc_error(ierr, __FILE__, "VecGhostUpdateBegin");
  ierr = VecGhostUpdateEnd(b, ADD_VALUES, SCATTER_REVERSE);
  if (ierr != 0) petsc_error(ierr, __FILE__, "VecGhostUpdateEnd");
}
//-----------------------------------------------------------------------------
Mat multiphenics::fem::block_assemble(const BlockForm2& a)
{
  Mat A = init_matrix(a);
  block_assemble(A, a);
  return A;
}
//-----------------------------------------------------------------------------
void multiphenics::fem::block_assemble(Mat A, const BlockForm2& a)
{
  // Assemble using standard assembler
  for (unsigned int i(0); i < a.block_size(0); ++i)
  {
    for (unsigned int j(0); j < a.block_size(1); ++j)
    {
      const Form & a_ij(a(i, j));
      if (
        a_ij.integrals().num_integrals(FormIntegrals::Type::cell) > 0
          ||
        a_ij.integrals().num_integrals(FormIntegrals::Type::interior_facet) > 0
          ||
        a_ij.integrals().num_integrals(FormIntegrals::Type::exterior_facet) > 0
      )
      {
        BlockPETScSubMatrix A_ij(A, {{i, j}}, {{a.block_function_spaces()[0]->block_dofmap(), a.block_function_spaces()[1]->block_dofmap()}});
        assemble_matrix(A_ij.mat(), a(i, j), {}, {});
      }
    }
  }
  // Finalize assembly of global tensor
  PetscErrorCode ierr;
  ierr = MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY);
  if (ierr != 0) petsc_error(ierr, __FILE__, "MatAssemblyBegin");
  ierr = MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY);
  if (ierr != 0) petsc_error(ierr, __FILE__, "MatAssemblyEnd");
}
//-----------------------------------------------------------------------------
Vec multiphenics::fem::init_vector(const BlockForm1& L)
{
  assert(L.block_function_spaces()[0]->block_dofmap()->index_map);
  return create_petsc_vector(*L.block_function_spaces()[0]->block_dofmap()->index_map);
}
//-----------------------------------------------------------------------------
Mat multiphenics::fem::init_matrix(const BlockForm2& a)
{
  // This method is adapted from
  //    dolfinx::fem::create_matrix in dolfinx/fem/utils.cpp

  // Get mesh
  assert(a.mesh());
  const Mesh& mesh = *(a.mesh());

  Timer t0("Build sparsity");

  // Get IndexMaps for each dimension
  std::array<std::shared_ptr<const IndexMap>, 2> index_maps{{
    a.block_function_spaces()[0]->block_dofmap()->index_map,
    a.block_function_spaces()[1]->block_dofmap()->index_map
  }};

  // Create sparsity pattern
  SparsityPattern pattern(mesh.mpi_comm(), index_maps);

  // Build sparsity pattern for each block
  for (std::size_t i = 0; i < a.block_size(0); i++)
  {
    for (std::size_t j = 0; j < a.block_size(1); j++)
    {
      const Form & a_ij(a(i, j));
      std::array<const BlockDofMap*, 2> dofmaps_ij{{
        &a.block_function_spaces()[0]->block_dofmap()->view(i),
        &a.block_function_spaces()[1]->block_dofmap()->view(j)
      }};
      if (
        a_ij.integrals().num_integrals(FormIntegrals::Type::cell) > 0
          ||
        a_ij.integrals().num_integrals(FormIntegrals::Type::interior_facet) > 0
          ||
        a_ij.integrals().num_integrals(FormIntegrals::Type::exterior_facet) > 0
      )
      {
        if (a_ij.integrals().num_integrals(FormIntegrals::Type::cell) > 0)
        {
          BlockSparsityPatternBuilder::cells(pattern, mesh.topology(), dofmaps_ij);
        }
        if (a_ij.integrals().num_integrals(FormIntegrals::Type::interior_facet) > 0)
        {
          mesh.create_entities(mesh.topology().dim() - 1);
          mesh.create_connectivity(mesh.topology().dim() - 1, mesh.topology().dim());
          BlockSparsityPatternBuilder::interior_facets(pattern, mesh.topology(), dofmaps_ij);
        }
        if (a_ij.integrals().num_integrals(FormIntegrals::Type::exterior_facet) > 0)
        {
          mesh.create_entities(mesh.topology().dim() - 1);
          mesh.create_connectivity(mesh.topology().dim() - 1, mesh.topology().dim());
          BlockSparsityPatternBuilder::exterior_facets(pattern, mesh.topology(), dofmaps_ij);
        }
      }
    }
  }

  // Keep diagonal element in sparsity pattern
  const std::int32_t local_size = index_maps[0]->size_local();
  Eigen::Array<PetscInt, Eigen::Dynamic, 1> diagonal_dof(1);
  for (std::int32_t I = 0; I < local_size; I++)
  {
    const PetscInt _I = I;
    diagonal_dof(0) = _I;
    pattern.insert(diagonal_dof, diagonal_dof);
  }

  // Finalize sparsity pattern
  pattern.assemble();
  t0.stop();

  // Initialize matrix
  Timer t1("Init tensor");
  Mat A = create_petsc_matrix(a.mesh()->mpi_comm(), pattern);
  t1.stop();

  PetscErrorCode ierr;

  // Insert zeros on the diagonal as diagonal entries may be
  // optimised away, e.g. when calling MatAssemblyBegin/MatAssemblyEnd.
  const PetscScalar block = 0.0;
  for (std::int32_t I = 0; I < local_size; I++)
  {
    const PetscInt _I = I;
    ierr = MatSetValueLocal(A, _I, _I, block, INSERT_VALUES);
    if (ierr != 0) petsc_error(ierr, __FILE__, "MatSetValueLocal");
  }

  // Wait with assembly flush
  ierr = MatAssemblyBegin(A, MAT_FLUSH_ASSEMBLY);
  if (ierr != 0) petsc_error(ierr, __FILE__, "MatAssemblyBegin");
  ierr = MatAssemblyEnd(A, MAT_FLUSH_ASSEMBLY);
  if (ierr != 0) petsc_error(ierr, __FILE__, "MatAssemblyEnd");

  // Return
  return A;
}

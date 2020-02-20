# Copyright (C) 2016-2020 by the multiphenics authors
#
# This file is part of multiphenics.
#
# multiphenics is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# multiphenics is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with multiphenics. If not, see <http://www.gnu.org/licenses/>.
#

from multiphenics.fem import block_adjoint, block_derivative, BlockDirichletBC, BlockForm1, BlockForm2, block_restrict
from multiphenics.function import BlockFunction, BlockFunctionSpace, block_split, BlockTestFunction, BlockTrialFunction
from multiphenics.la import BlockSLEPcEigenSolver, block_solve, SLEPcEigenSolver
from multiphenics.nls import BlockNewtonSolver, BlockNonlinearProblem

__all__ = [
    "block_adjoint",
    "block_derivative",
    "BlockDirichletBC",
    "BlockForm1",
    "BlockForm2",
    "BlockFunction",
    "BlockFunctionSpace",
    "BlockNewtonSolver",
    "BlockNonlinearProblem",
    "block_restrict",
    "BlockSLEPcEigenSolver",
    "block_solve",
    "block_split",
    "BlockTestFunction",
    "BlockTrialFunction",
    "SLEPcEigenSolver"
]

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

from numpy import empty
from ufl import Form
from dolfinx.fem import adjoint
from multiphenics.fem.block_form_2 import BlockForm2
from multiphenics.fem.block_replace_zero import _is_zero
from multiphenics.function import BlockTestFunction, BlockTrialFunction

def block_adjoint(block_form):
    assert isinstance(block_form, BlockForm2)
    N = block_form.block_size(0)
    M = block_form.block_size(1)
    block_adjoint_function_space = [block_form.block_function_spaces()[1], block_form.block_function_spaces()[0]]
    block_test_function_adjoint = BlockTestFunction(block_adjoint_function_space[0])
    block_trial_function_adjoint = BlockTrialFunction(block_adjoint_function_space[1])
    block_adjoint_form = empty((M, N), dtype=object)
    for I in range(N):
        for J in range(M):
            assert isinstance(block_form[I][J], Form) or _is_zero(block_form[I][J])
            if isinstance(block_form[I][J], Form):
                block_adjoint_form[J][I] = adjoint(block_form[I][J], (block_test_function_adjoint[J], block_trial_function_adjoint[I]))
            elif _is_zero(block_form[I][J]):
                block_adjoint_form[J][I] = 0
            else:
                raise TypeError("Invalid form")
    return BlockForm2(block_adjoint_form.tolist(), block_function_space=block_adjoint_function_space)

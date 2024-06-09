/*
 * @file example_basic3.cpp
 *
 * @copyright Copyright (C) 2024 Enrico Degregori <enrico.degregori@gmail.com>
 *
 * @author Enrico Degregori <enrico.degregori@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.

 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.

 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <vector>
#include "mpi.h"
#include "bindings/C++/distdir.hpp"

#define I_SRC 0
#define I_DST 1
#define NCOLS 4
#define NROWS 4
#define NLEVS 2

/**
 * @brief Basic example of exchange between two 3D domain decomposition each using 2 MPI processes.
 * 
 * @details The example uses a total of 4 MPI processes over a 4x4x2 global 3D domain.
 *          Processes 0,1 have the following domain decomposition:
 * 
 *          Rank: 0
 *          Indices: 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30
 *          Rank: 1
 *          Indices: 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31
 * 
 *          Processes 2,3 have the following domain decomposition:
 * 
 *          Rank: 2
 *          Indices: 0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23
 *          Rank: 3
 *          Indices: 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31
 * 
 *          Ranks 0,1 send data to ranks 2,3
 * 
 *          Exchange of integers is tested.
 * 
 * @ingroup examples
 */

int example_basic3() {

	distdir::distdir::Ptr distdir( new distdir::distdir() );

	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	int world_role;
	int npoints_local = NCOLS * NROWS / (world_size / 2);

	if (world_size != 4) return 1;

	std::vector<int> list;
	// index list with global indices
	if (world_rank < 2) {
		world_role = I_SRC;
		for (int i=0; i<npoints_local; i++)
			list.push_back( world_rank + i*2 );
	} else {
		world_role = I_DST;
		int nrows_local = NROWS / (world_size / 2);
		for (int i=0; i < nrows_local; i++)
			for (int j=0; j < NCOLS; j++)
				list.push_back( j + i * NCOLS + (world_rank - (world_size / 2)) * (NROWS - nrows_local) * NCOLS );
	}

	distdir::idxlist::Ptr idxlist_empty( new distdir::idxlist() );
	distdir::idxlist::Ptr idxlist( new distdir::idxlist(list) );

	distdir::map::Ptr map2d( new distdir::map(world_role == I_SRC ? idxlist : idxlist_empty,
	                                          world_role == I_SRC ? idxlist_empty : idxlist,
	                                          MPI_COMM_WORLD) );

	distdir::map::Ptr map( new distdir::map(map2d, NLEVS) );

	distdir::exchanger<int>::Ptr exchanger ( new distdir::exchanger<int>(map, MPI_INT) );

	std::vector<int> data;
	data.resize(npoints_local*NLEVS);

	// src MPI ranks fill data array
	if (world_role == I_SRC)
		for (int level = 0; level < NLEVS; level++)
			for (int i = 0; i < npoints_local; i++)
				data[i+level*npoints_local] = i + level*npoints_local + npoints_local * NLEVS * world_rank;

	exchanger->go(data, data);

	std::cout << world_rank << ": ";
	for (int level = 0; level < NLEVS; level++)
		for (int i = 0; i < npoints_local; i++)
			std::cout << data[i+level*npoints_local] << " ";
	std::cout << std::endl;

	exchanger.reset();
	map2d.reset();
	map.reset();
	idxlist.reset();
	idxlist_empty.reset();
	distdir.reset();

	return 0;
}

int main() {

	int err = example_basic3();
	if (err != 0) return err;

	return 0;
}

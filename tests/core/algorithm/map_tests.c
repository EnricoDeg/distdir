/*
 * @file map_tests.c
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

#include <stdarg.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "src/distdir.h"

#define LSIZE 8

/**
 * @brief test01 for map module
 * 
 * @details The test uses a total of 4 MPI processes over a 4x4 global 2D domain.
 *          Processes 0,1 have the following domain decomposition:
 * 
 *          Rank: 0
 *          Indices: 0, 1, 4, 5, 8, 9, 12, 13
 *          Rank: 1
 *          Indices: 2, 3, 6, 7, 10, 11, 14, 15
 * 
 *          Processes 2,3 have the following domain decomposition:
 * 
 *          Rank: 2
 *          Indices: 0, 1, 2, 3, 4, 5, 6, 7, 8
 *          Rank: 3
 *          Indices: 9, 10, 11, 12, 13, 14, 15
 * 
 *          Ranks 0,1 are sender processes and ranks 2,3 are receivers processes.
 * 
 *          The generated maps are tested.
 * 
 *          The test is based on the maps generated by example_basic1.
 * 
 * @ingroup map_tests
 */
static int map_test01(MPI_Comm comm) {

	const int I_SRC = 0;
	const int I_DST = 1;
	const int NCOLS = 4;
	const int NROWS = 4;

	int world_rank;
	MPI_Comm_rank(comm, &world_rank);
	int world_size;
	MPI_Comm_size(comm, &world_size);
	int world_role;
	int npoints_local = NCOLS * NROWS / (world_size / 2);
	int idxlist[npoints_local];
	t_idxlist *p_idxlist;
	t_idxlist *p_idxlist_empty;
	t_map *p_map;

	if (world_size != 4) return 1;

	// index list with global indices
	if (world_rank < 2) {
		world_role = I_SRC;
		int ncols_local = NCOLS / (world_size / 2);
		for (int i=0; i < NROWS; i++)
			for (int j=0; j < ncols_local; j++)
				idxlist[j+i*ncols_local] = j + i * NCOLS + world_rank * (NCOLS - ncols_local);
	} else {
		world_role = I_DST;
		int nrows_local = NROWS / (world_size / 2);
		for (int i=0; i < nrows_local; i++)
			for (int j=0; j < NCOLS; j++)
				idxlist[j+i*NCOLS] = j + i * NCOLS + (world_rank - (world_size / 2)) * (NROWS - nrows_local) * NCOLS;
	}

	p_idxlist = new_idxlist(idxlist, npoints_local);
	p_idxlist_empty = new_idxlist_empty();

	if (world_role == I_SRC) {
		p_map = new_map(p_idxlist, p_idxlist_empty, -1, comm);
	} else {
		p_map = new_map(p_idxlist_empty, p_idxlist, -1, comm);
	}

	// Check test results
	int error = 0;
	if (world_role == I_SRC) {

		if ( p_map->exch_send->count != 2 )
			error = 1;
		if ( p_map->exch_send->buffer_size != npoints_local )
			error = 1;
		for (int i = 0; i < p_map->exch_send->count; i++)
			if (p_map->exch_send->exch[i]->exch_rank != i + 2)
				error = 1;
#ifndef CUDA
		for (int i = 0; i < p_map->exch_send->buffer_size; i++)
			if (p_map->exch_send->buffer_idxlist[i] != i)
				error = 1;
#endif
		for (int i = 0; i < p_map->exch_send->count; i++)
			if (p_map->exch_send->buffer_offset[i] != i * 4)
				error = 1;
	} else {

		if ( p_map->exch_recv->count != 2 )
			error = 1;
		if ( p_map->exch_recv->buffer_size != npoints_local )
			error = 1;
		for (int i = 0; i < p_map->exch_recv->count; i++)
			if (p_map->exch_recv->exch[i]->exch_rank != i)
				error = 1;
#ifndef CUDA
		int solution[LSIZE] = {0, 1, 4, 5, 2, 3, 6, 7};
		for (int i = 0; i < p_map->exch_recv->buffer_size; i++)
			if (p_map->exch_recv->buffer_idxlist[i] != solution[i])
				error = 1;
#endif
		for (int i = 0; i < p_map->exch_recv->count; i++)
			if (p_map->exch_recv->buffer_offset[i] != i * 4)
				error = 1;
	}

	delete_idxlist(p_idxlist);
	delete_idxlist(p_idxlist_empty);
	delete_map(p_map);

	return error;
}

int main() {

	// Initialize the MPI environment
	MPI_Init(NULL, NULL);

	int error = 0;

	error += map_test01(MPI_COMM_WORLD);

	// Finalize the MPI environment.
	MPI_Finalize();
	return error;
}
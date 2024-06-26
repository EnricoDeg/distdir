!
! @file example_basic9.f90
!
! @copyright Copyright (C) 2024 Enrico Degregori <enrico.degregori@gmail.com>
!
! @author Enrico Degregori <enrico.degregori@gmail.com>
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions are met:
!
! 1. Redistributions of source code must retain the above copyright notice, this
!    list of conditions and the following disclaimer.
!
! 2. Redistributions in binary form must reproduce the above copyright notice,
!    this list of conditions and the following disclaimer in the documentation
!    and/or other materials provided with the distribution.
!
! 3. Neither the name of the copyright holder nor the names of its
!    contributors may be used to endorse or promote products derived from
!    this software without specific prior written permission.
! 
! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
! AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
! IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
! DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
! FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
! DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
! SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
! CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
! OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
! OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
!

PROGRAM example_basic9
	USE mpi
	USE distdir_mod
	USE, INTRINSIC :: ISO_C_BINDING, ONLY: c_loc

	IMPLICIT NONE

	INTEGER, ALLOCATABLE, DIMENSION(:) :: list
	INTEGER, ALLOCATABLE, TARGET, DIMENSION(:) :: data
	INTEGER :: i, j
	TYPE(t_idxlist)   :: idxlist, idxlist_empty
	TYPE(t_map)       :: map2d, map
	TYPE(t_exchanger) :: exchanger
	INTEGER :: error, world_rank, world_size, world_role
	INTEGER :: npoints_local, ncols_local, nrows_local


	INTEGER, PARAMETER :: I_SRC = 0
	INTEGER, PARAMETER :: I_DST = 1
	INTEGER, PARAMETER :: NCOLS = 4
	INTEGER, PARAMETER :: NROWS = 4
    INTEGER, PARAMETER :: NLEVS = 2
	INTEGER, PARAMETER :: comm  = MPI_COMM_WORLD
	INTEGER, PARAMETER :: hw    = DISTDIR_HW_CPU
	INTEGER, PARAMETER :: type  = MPI_INTEGER
	
	CALL distdir_initialize()

	CALL MPI_COMM_SIZE(comm, world_size, error)
	CALL MPI_COMM_RANK(comm, world_rank, error)

	! index list with global indices
	IF (world_rank < 2) THEN
		world_role = I_SRC;
		npoints_local = NCOLS * NROWS / (world_size-1);
		ALLOCATE(list(npoints_local))
		DO i = 0, npoints_local-1
			list(i+1) = world_rank + i*2;
		END DO
	ELSE
		world_role = I_DST;
		npoints_local = NROWS * NCOLS;
		ALLOCATE(list(npoints_local))
		DO i = 0, npoints_local-1
			list(i+1) = i;
		END DO
	END IF

	CALL new_idxlist(idxlist, list, npoints_local)
	CALL new_idxlist(idxlist_empty)

	IF (world_role == I_SRC) THEN
		CALL new_map(map, idxlist, idxlist_empty, comm)
	ELSE
		CALL new_map(map, idxlist_empty, idxlist, comm)
	END IF

	ALLOCATE(data(npoints_local))

	IF (world_role == I_SRC) THEN
		DO i = 0, npoints_local-1
			data(i+1) = i + npoints_local * world_rank;
		END DO
	END IF

	CALL new_exchanger(exchanger, map, type, hw)

	CALL exchanger_go(exchanger, C_LOC(data(1)), C_LOC(data(1)))

	write(*,*) world_rank, ': ', data(:)

	CALL delete_exchanger(exchanger)
	CALL delete_map(map)
	CALL delete_idxlist(idxlist_empty)
	CALL delete_idxlist(idxlist)

	DEALLOCATE(data)
	DEALLOCATE(list)
	
	CALL distdir_finalize()

END PROGRAM example_basic9
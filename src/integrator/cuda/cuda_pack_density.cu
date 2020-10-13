#include "cuda_pack_density.hpp"
#include "cuda_device_properties.hpp"

namespace GauXC      {
namespace integrator {
namespace cuda       {

using namespace GauXC::cuda;

#define WARP_X 32
#define WARP_Y 1
#define UNROLL_FACTOR 4
#define EFF_UNROLL 4
#define CUT_X 8
#define CUT_Y 4

template <typename T>
__global__ __launch_bounds__(1024, 1)
void submat_set_combined_kernel( size_t           ntasks,
                                 XCTaskDevice<T>* device_tasks,
                                 T*               A,
                                 size_t           LDA,
				 const int block_y,
				 const int block_x) {


  const int batch_id = blockIdx.z;
  auto& task = device_tasks[ batch_id ];

  const auto  ncut              = task.ncut;
  const auto* submat_cut_device = task.submat_cut;
  const auto  LDAS              = task.nbe;
        auto* ASmall_device     = task.nbe_scr;

  //if( LDAS == LDAB ) return;

  const int tid_xx = threadIdx.x % WARP_X;
  const int tid_xy = threadIdx.x / WARP_X;

  const int tid_yx = threadIdx.y % CUT_X;
  const int tid_yy = threadIdx.y / CUT_X;

  const int ncut_sub = submat_cut_device[ 4*0 + 3 ];
  const int start_cut_y = block_y ? 0 : ncut_sub;
  const int end_cut_y   = block_y ? ncut_sub : ncut;
  const int start_cut_x = block_x ? 0 : ncut_sub;
  const int end_cut_x   = block_x ? ncut_sub : ncut;

  for( int i_cut = tid_yy + start_cut_y; i_cut < end_cut_y; i_cut += CUT_Y ) {
    const int i_cut_first  = submat_cut_device[ 4*i_cut ];
    const int i_cut_second = submat_cut_device[ 4*i_cut + 1 ];
    const int delta_i      = i_cut_second - i_cut_first;
    const int i_cut_small  = submat_cut_device[ 4*i_cut + 2 ];

  for( int j_cut = tid_yx + start_cut_x; j_cut < end_cut_x; j_cut += CUT_X ) {
    const int j_cut_first  = submat_cut_device[ 4*j_cut ];
    const int j_cut_second = submat_cut_device[ 4*j_cut + 1 ];
    const int delta_j      = j_cut_second - j_cut_first;
    const int j_cut_small  = submat_cut_device[ 4*j_cut + 2 ];

    auto* ASmall_begin = ASmall_device + i_cut_small + j_cut_small*LDAS;
    auto* ABig_begin   = A   + i_cut_first + j_cut_first*LDA;

    int J;
    for( J = tid_xy; J < (delta_j / EFF_UNROLL) * EFF_UNROLL; J += EFF_UNROLL ) {
      for( int I = tid_xx; I < delta_i; I += WARP_X ) {

        double val[UNROLL_FACTOR];
        double* address[UNROLL_FACTOR];
#pragma unroll
        for (int k = 0; k < UNROLL_FACTOR; k++) {
          val[k] = ABig_begin[I + (J + k*WARP_Y)*LDA];
          address[k] = ASmall_begin + I + (J + k*WARP_Y) * LDAS;
        }
#pragma unroll
        for (int k = 0; k < UNROLL_FACTOR; k++) {
	  // Suggest that the result be evicted first
          asm ("st.global.cs.f64 [%0], %1;" :: "l"(address[k]), "d"(val[k]));
        }
      }
    }

    for ( ; J < delta_j; J += WARP_Y) {
      for( int I = tid_xx; I < delta_i; I += WARP_X ) {
        ASmall_begin[I + J*LDAS] = ABig_begin[I + J*LDA];
      }
    }
  }
  }
}


template <typename T>
void task_pack_density_matrix( size_t           ntasks,
                               XCTaskDevice<T>* device_tasks,
                               T*               P_device,
                               size_t           LDP,
                               cudaStream_t     stream ) {

  dim3 threads(32,32,1), blocks(1,1,ntasks);
  submat_set_combined_kernel<<< blocks, threads, 0, stream >>>(
    ntasks, device_tasks, P_device, LDP, 0, 0
  );

  submat_set_combined_kernel<<< blocks, threads, 0, stream >>>(
    ntasks, device_tasks, P_device, LDP, 0, 1
  );

  submat_set_combined_kernel<<< blocks, threads, 0, stream >>>(
    ntasks, device_tasks, P_device, LDP, 1, 0
  );

  submat_set_combined_kernel<<< blocks, threads, 0, stream >>>(
    ntasks, device_tasks, P_device, LDP, 1, 1
  );
}

template 
void task_pack_density_matrix( size_t                ntasks,
                               XCTaskDevice<double>* device_tasks,
                               double*               P_device,
                               size_t                LDP,
                               cudaStream_t          stream );

}
}
}

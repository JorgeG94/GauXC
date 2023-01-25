#include "hip_backend.hpp"

namespace GauXC {

HIPBackend::HIPBackend() {

  // Create HIP Stream and CUBLAS Handles and make them talk to eachother
  master_stream = std::make_shared< util::hip_stream >();
  master_handle = std::make_shared< util::hipblas_handle >();

  hipblasSetStream( *master_handle, *master_stream );

#ifdef GAUXC_ENABLE_MAGMA
  // Setup MAGMA queue with CUDA stream / cuBLAS handle
  master_magma_queue_ = std::make_shared< util::magma_queue >(0, *master_stream, *master_handle);
#endif

}

HIPBackend::~HIPBackend() noexcept = default;

HIPBackend::device_buffer_t HIPBackend::allocate_device_buffer(int64_t sz) {
  void* ptr;
  auto stat = hipMalloc(&ptr, sz);
  GAUXC_HIP_ERROR( "HIP Malloc Failed", stat );
  return device_buffer_t{ptr,sz};
}

size_t HIPBackend::get_available_mem() {
  size_t hip_avail, hip_total;
  auto stat = hipMemGetInfo( &hip_avail, &hip_total );
  GAUXC_HIP_ERROR( "MemInfo Failed", stat );
  return hip_avail;
}

void HIPBackend::free_device_buffer( void* ptr ) {
  auto stat = hipFree(ptr);
  GAUXC_HIP_ERROR( "Free Failed", stat );
}

void HIPBackend::master_queue_synchronize() {
  auto stat = hipStreamSynchronize( *master_stream );
  GAUXC_HIP_ERROR( "StreamSynchronized Failed", stat );
}

device_queue HIPBackend::queue() {
  return device_queue(master_stream);
}

void HIPBackend::create_blas_queue_pool(int32_t ns) {
  blas_streams.resize(ns);
  blas_handles.resize(ns);
  for( auto i = 0; i < ns; ++i ) {
    blas_streams[i] = std::make_shared<util::hip_stream>();
    blas_handles[i] = std::make_shared<util::hipblas_handle>();
    hipblasSetStream( *blas_handles[i], *blas_streams[i] );
  }
}

void HIPBackend::sync_master_with_blas_pool() {
  const auto n_streams = blas_streams.size();
  std::vector<util::hip_event> blas_events( n_streams );
  for( size_t iS = 0; iS < n_streams; ++iS )
    blas_events[iS].record( *blas_streams[iS] );

  for( auto& event : blas_events ) master_stream->wait(event);
}

void HIPBackend::sync_blas_pool_with_master() {
  util::hip_event master_event;
  master_event.record( *master_stream );
  for( auto& stream : blas_streams ) stream->wait( master_event );
}

size_t HIPBackend::blas_pool_size(){ return blas_streams.size(); }

device_queue HIPBackend::blas_pool_queue(int32_t i) {
  return device_queue( blas_streams.at(i) );
}

device_blas_handle HIPBackend::blas_pool_handle(int32_t i) {
  return device_blas_handle( blas_handles.at(i) );
}
device_blas_handle HIPBackend::master_blas_handle() {
  return device_blas_handle( master_handle );
}

void HIPBackend::copy_async_( size_t sz, const void* src, void* dest,
  std::string msg ) {
  auto stat = hipMemcpyAsync( dest, src, sz, hipMemcpyDefault, *master_stream );
  GAUXC_HIP_ERROR( "HIP Memcpy Async Failed ["+msg+"]", stat );
}

void HIPBackend::set_zero_(size_t sz, void* data, std::string msg ) {
  auto stat = hipMemset( data, 0, sz );
  GAUXC_HIP_ERROR( "HIP Memset Failed ["+msg+"]", stat );
}

void HIPBackend::set_zero_async_master_queue_(size_t sz, void* data, std::string msg ) {
  auto stat = hipMemsetAsync( data, 0, sz, *master_stream );
  GAUXC_HIP_ERROR( "HIP Memset Failed ["+msg+"]", stat );
}

void HIPBackend::copy_async_2d_( size_t M, size_t N, const void* A, size_t LDA,
  void* B, size_t LDB, std::string msg ) {
  auto stat = hipMemcpy2DAsync( B, LDB, A, LDA, M, N, hipMemcpyDefault,
    *master_stream );
  GAUXC_HIP_ERROR( "HIP 2D Memcpy Async Failed ["+msg+"]", stat );
}

std::unique_ptr<DeviceBackend> make_device_backend() {
  return std::make_unique<HIPBackend>();
}
}

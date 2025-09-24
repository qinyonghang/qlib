#include <cuda_runtime.h>
#include <iostream>

// constexpr struct config {
//     int gird_size;
//     int block_size;

//     constexpr config() {
//         cudaOccupancyMaxPotentialBlockSize
//     }
// } g_config;

template <class T>
__global__ void add(T *dst, T *src1, T src2) {
    auto i = threadIdx.x + blockIdx.x * blockDim.x;
    dst[i] = src1[i] + src2;
}

template <class T>
__global__ void add(T *dst, T *src1, T *src2) {
    auto i = threadIdx.x + blockIdx.x * blockDim.x;
    dst[i] = src1[i] + src2[i];
}

template <class T>
__global__ void sub(T *dst, T *src1, T src2) {
    auto i = threadIdx.x + blockIdx.x * blockDim.x;
    dst[i] = src1[i] - src2;
}

template <class T>
__global__ void sub(T *dst, T *src1, T *src2) {
    auto i = threadIdx.x + blockIdx.x * blockDim.x;
    dst[i] = src1[i] - src2[i];
}

template <class T>
__global__ void mul(T *dst, T *src1, T src2) {
    auto i = threadIdx.x + blockIdx.x * blockDim.x;
    dst[i] = src1[i] * src2;
}

template <class T>
__global__ void div(T *dst, T *src1, T src2) {
    auto i = threadIdx.x + blockIdx.x * blockDim.x;
    dst[i] = src1[i] / src2;
}

template <class T>
__global__ void mod(T *dst, T *src1, T *src2) {
    auto i = threadIdx.x + blockIdx.x * blockDim.x;
    dst[i] = src1[i] % src2[i];
}

__global__ void vectorAddKernel(int* a, int* b, int* result) {
    int i = threadIdx.x + blockIdx.x * blockDim.x;
    result[i] = a[i] + b[i];
}

void vectorAdd(int* h_a, int* h_b, int* h_result, int n) {
    const auto aligned_n = ((n + 255) / 256) * 256;
    int *d_a, *d_b, *d_result;

    cudaMalloc(&d_a, aligned_n * sizeof(int));
    cudaMalloc(&d_b, aligned_n * sizeof(int));
    cudaMalloc(&d_result, aligned_n * sizeof(int));
    // cudaMallocManaged(&d_a, aligned_n * sizeof(int));
    // cudaMallocManaged(&d_b, aligned_n * sizeof(int));
    // cudaMallocManaged(&d_result, aligned_n * sizeof(int));

    // memcpy(d_a, h_a, n * sizeof(int));
    // memcpy(d_b, h_b, n * sizeof(int));
    cudaMemcpy(d_a, h_a, n * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, h_b, n * sizeof(int), cudaMemcpyHostToDevice);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);
    vectorAddKernel<<<aligned_n / 256, 256>>>(d_a, d_b, d_result);
    cudaEventRecord(stop);

    cudaDeviceSynchronize();

    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start, stop);
    // std::cout << "GPU kernel time: " << milliseconds << " ms" << std::endl;

    // memcpy(h_result, d_result, n * sizeof(int));
    cudaMemcpy(h_result, d_result, n * sizeof(int), cudaMemcpyDeviceToHost);

    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_result);
}

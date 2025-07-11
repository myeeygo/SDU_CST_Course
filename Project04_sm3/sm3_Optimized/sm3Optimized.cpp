#include <cstring>
#include <immintrin.h>
#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <chrono>

// 常量定义
constexpr uint32_t T[64] = {
    0x79cc4519, 0xf3988a32, 0xe7311465, 0xce6228cb, 0x9cc45197, 0x3988a32f, 0x7311465e, 0xe6228cbc,
    0xcc451979, 0x988a32f3, 0x311465e7, 0x6228cbce, 0xc451979c, 0x88a32f39, 0x11465e73, 0x228cbce6,
    0x9dcc4519, 0x3988a32f, 0x7311465e, 0xe6228cbc, 0xcc451979, 0x988a32f3, 0x311465e7, 0x6228cbce,
    0xc451979c, 0x88a32f39, 0x11465e73, 0x228cbce6, 0x9dcc4519, 0x3988a32f, 0x7311465e, 0xe6228cbc,
    0xcc451979, 0x988a32f3, 0x311465e7, 0x6228cbce, 0xc451979c, 0x88a32f39, 0x11465e73, 0x228cbce6,
    0x9dcc4519, 0x3988a32f, 0x7311465e, 0xe6228cbc, 0xcc451979, 0x988a32f3, 0x311465e7, 0x6228cbce,
    0xc451979c, 0x88a32f39, 0x11465e73, 0x228cbce6, 0x9dcc4519, 0x3988a32f, 0x7311465e, 0xe6228cbc,
    0xcc451979, 0x988a32f3, 0x311465e7, 0x6228cbce, 0xc451979c, 0x88a32f39, 0x11465e73, 0x228cbce6
};

// 循环左移 (通常编译器会优化为ROL指令)
inline uint32_t ROTL32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

// P1函数 (消息扩展)
inline uint32_t P1(uint32_t x) {
    return x ^ ROTL32(x, 15) ^ ROTL32(x, 23);
}

// 压缩函数FF (0-15轮)
inline uint32_t FF0(uint32_t x, uint32_t y, uint32_t z) {
    return x ^ y ^ z;
}

// 压缩函数FF (16-63轮)
inline uint32_t FF1(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) | (x & z) | (y & z);
}

// 压缩函数GG (0-15轮)
inline uint32_t GG0(uint32_t x, uint32_t y, uint32_t z) {
    return x ^ y ^ z;
}

// 压缩函数GG (16-63轮)
inline uint32_t GG1(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) | (~x & z);
}

// 消息扩展 (AVX2优化)
void MessageExpansionAVX2(const uint32_t* block, uint32_t* W) {
    // 加载初始16个消息字
    __m128i w0 = _mm_loadu_si128((const __m128i*)(block + 0));
    __m128i w1 = _mm_loadu_si128((const __m128i*)(block + 4));
    __m128i w2 = _mm_loadu_si128((const __m128i*)(block + 8));
    __m128i w3 = _mm_loadu_si128((const __m128i*)(block + 12));

    // 存储初始W0-W15
    _mm_storeu_si128((__m128i*)(W + 0), w0);
    _mm_storeu_si128((__m128i*)(W + 4), w1);
    _mm_storeu_si128((__m128i*)(W + 8), w2);
    _mm_storeu_si128((__m128i*)(W + 12), w3);

    // 字节序转换掩码
    const __m128i bswap_mask = _mm_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);

    // 字节序转换 (大端转小端)
    w0 = _mm_shuffle_epi8(w0, bswap_mask);
    w1 = _mm_shuffle_epi8(w1, bswap_mask);
    w2 = _mm_shuffle_epi8(w2, bswap_mask);
    w3 = _mm_shuffle_epi8(w3, bswap_mask);

    // 循环 - 每次计算4个消息字
    for (int j = 16; j < 68; j += 4) {
        // 加载所需的消息字
        __m128i wj_16 = _mm_loadu_si128((const __m128i*)(W + j - 16));
        __m128i wj_9 = _mm_loadu_si128((const __m128i*)(W + j - 9));
        __m128i wj_3 = _mm_loadu_si128((const __m128i*)(W + j - 3));
        __m128i wj_13 = _mm_loadu_si128((const __m128i*)(W + j - 13));
        __m128i wj_6 = _mm_loadu_si128((const __m128i*)(W + j - 6));

        // 计算 Wj = P1(Wj-16 ^ Wj-9 ^ ROTL32(Wj-3, 15)) ^ ROTL32(Wj-13, 7) ^ Wj-6
        __m128i rot15 = _mm_or_si128(_mm_slli_epi32(wj_3, 15), _mm_srli_epi32(wj_3, 17));
        __m128i rot7 = _mm_or_si128(_mm_slli_epi32(wj_13, 7), _mm_srli_epi32(wj_13, 25));

        __m128i xor1 = _mm_xor_si128(wj_16, wj_9);
        __m128i xor2 = _mm_xor_si128(xor1, rot15);

        // 计算P1函数 (AVX2实现)
        __m128i p1_rot15 = _mm_or_si128(_mm_slli_epi32(xor2, 15), _mm_srli_epi32(xor2, 17));
        __m128i p1_rot23 = _mm_or_si128(_mm_slli_epi32(xor2, 23), _mm_srli_epi32(xor2, 9));
        __m128i p1 = _mm_xor_si128(_mm_xor_si128(xor2, p1_rot15), p1_rot23);

        __m128i result = _mm_xor_si128(_mm_xor_si128(p1, rot7), wj_6);

        // 存储结果
        _mm_storeu_si128((__m128i*)(W + j), result);
    }

    // 计算W'0-W'63
    for (int j = 0; j < 64; j++) {
        W[j + 68] = W[j] ^ W[j + 4];
    }
}

// 压缩函数宏定义
#define SM3_ROUND(A, B, C, D, E, F, G, H, WT, j) \
    do { \
        uint32_t SS1 = ROTL32(ROTL32(A, 12) + E + ROTL32(T[j], j % 32), 7); \
        uint32_t SS2 = SS1 ^ ROTL32(A, 12); \
        uint32_t TT1 = (j < 16 ? FF0(A, B, C) : FF1(A, B, C)) + D + SS2 + WT; \
        uint32_t TT2 = (j < 16 ? GG0(E, F, G) : GG1(E, F, G)) + H + SS1 + W[j]; \
        D = C; \
        C = ROTL32(B, 9); \
        B = A; \
        A = TT1; \
        H = G; \
        G = ROTL32(F, 19); \
        F = E; \
        E = P0(TT2); \
    } while (0)

// P0函数 (压缩过程)
inline uint32_t P0(uint32_t x) {
    return x ^ ROTL32(x, 9) ^ ROTL32(x, 17);
}

// 压缩函数 (4轮展开优化)
void SM3Compress(uint32_t* V, const uint32_t* W) {
    // 初始化寄存器
    uint32_t A = V[0], B = V[1], C = V[2], D = V[3];
    uint32_t E = V[4], F = V[5], G = V[6], H = V[7];

    // 前16轮
    for (int j = 0; j < 16; j += 4) {
        SM3_ROUND(A, B, C, D, E, F, G, H, W[j + 68], j);
        SM3_ROUND(D, A, B, C, H, E, F, G, W[j + 1 + 68], j + 1);
        SM3_ROUND(C, D, A, B, G, H, E, F, W[j + 2 + 68], j + 2);
        SM3_ROUND(B, C, D, A, F, G, H, E, W[j + 3 + 68], j + 3);
    }

    // 后48轮
    for (int j = 16; j < 64; j += 4) {
        SM3_ROUND(A, B, C, D, E, F, G, H, W[j + 68], j);
        SM3_ROUND(D, A, B, C, H, E, F, G, W[j + 1 + 68], j + 1);
        SM3_ROUND(C, D, A, B, G, H, E, F, W[j + 2 + 68], j + 2);
        SM3_ROUND(B, C, D, A, F, G, H, E, W[j + 3 + 68], j + 3);
    }

    // 更新中间状态
    V[0] ^= A;
    V[1] ^= B;
    V[2] ^= C;
    V[3] ^= D;
    V[4] ^= E;
    V[5] ^= F;
    V[6] ^= G;
    V[7] ^= H;
}

// SM3哈希计算 (AVX2优化)
void SM3(const uint8_t* data, size_t len, uint8_t digest[32]) {
    // 初始状态 (常量)
    uint32_t V[8] = {
        0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600,
        0xa96f30bc, 0x163138aa, 0xe38dee4d, 0xb0fb0e4e
    };

    // 计算块数
    size_t block_count = len / 64;
    size_t remaining = len % 64;

    // 处理完整块
    for (size_t i = 0; i < block_count; i++) {
        // 为消息扩展分配空间 (68+64=132个消息字)
        uint32_t W[132];
        MessageExpansionAVX2((const uint32_t*)(data + i * 64), W);
        SM3Compress(V, W);
    }

    // 处理剩余数据
    uint8_t last_block[128] = { 0 };
    size_t total_bits = len * 8;

    if (len > 0) {
        // 复制剩余数据
        memcpy(last_block, data + block_count * 64, remaining);

        // 添加填充位
        last_block[remaining] = 0x80;

        // 添加消息长度 (大端)
        if (remaining < 56) {
            // 空间足够直接写入当前块
            last_block[60] = (total_bits >> 24) & 0xFF;
            last_block[61] = (total_bits >> 16) & 0xFF;
            last_block[62] = (total_bits >> 8) & 0xFF;
            last_block[63] = total_bits & 0xFF;

            uint32_t W[132];
            MessageExpansionAVX2((const uint32_t*)last_block, W);
            SM3Compress(V, W);
        }
        else {
            // 需要额外的块
            last_block[60] = (total_bits >> 24) & 0xFF;
            last_block[61] = (total_bits >> 16) & 0xFF;
            last_block[62] = (total_bits >> 8) & 0xFF;
            last_block[63] = total_bits & 0xFF;

            uint32_t W1[132];
            MessageExpansionAVX2((const uint32_t*)last_block, W1);
            SM3Compress(V, W1);

            // 第二个块全0，仅添加长度
            memset(last_block, 0, 64);
            uint32_t W2[132];
            MessageExpansionAVX2((const uint32_t*)last_block, W2);
            SM3Compress(V, W2);
        }
    }

    // 输出最终结果
    for (int i = 0; i < 8; i++) {
        digest[i * 4 + 0] = (V[i] >> 24) & 0xFF;
        digest[i * 4 + 1] = (V[i] >> 16) & 0xFF;
        digest[i * 4 + 2] = (V[i] >> 8) & 0xFF;
        digest[i * 4 + 3] = V[i] & 0xFF;
    }
}

// 传统SM3消息扩展
void MessageExpansionScalar(const uint32_t* block, uint32_t* W) {
    // 复制初始16个消息字
    for (int i = 0; i < 16; ++i) {
        W[i] = block[i];
    }

    // 计算W16-W67
    for (int j = 16; j < 68; ++j) {
        W[j] = P1(W[j - 16] ^ W[j - 9] ^ ROTL32(W[j - 3], 15)) ^ ROTL32(W[j - 13], 7) ^ W[j - 6];
    }

    // 计算W'0-W'63
    for (int j = 0; j < 64; ++j) {
        W[j + 68] = W[j] ^ W[j + 4];
    }
}

// 传统SM3压缩函数
void SM3CompressScalar(uint32_t* V, const uint32_t* W) {
    uint32_t A = V[0], B = V[1], C = V[2], D = V[3];
    uint32_t E = V[4], F = V[5], G = V[6], H = V[7];

    for (int j = 0; j < 64; ++j) {
        uint32_t SS1 = ROTL32(ROTL32(A, 12) + E + ROTL32(T[j], j % 32), 7);
        uint32_t SS2 = SS1 ^ ROTL32(A, 12);

        uint32_t FF = (j < 16) ? FF0(A, B, C) : FF1(A, B, C);
        uint32_t GG = (j < 16) ? GG0(E, F, G) : GG1(E, F, G);

        uint32_t TT1 = FF + D + SS2 + W[j + 68];
        uint32_t TT2 = GG + H + SS1 + W[j];

        D = C;
        C = ROTL32(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = ROTL32(F, 19);
        F = E;
        E = P0(TT2);
    }

    V[0] ^= A;
    V[1] ^= B;
    V[2] ^= C;
    V[3] ^= D;
    V[4] ^= E;
    V[5] ^= F;
    V[6] ^= G;
    V[7] ^= H;
}

// 传统SM3哈希计算
void SM3Scalar(const uint8_t* data, size_t len, uint8_t digest[32]) {
    uint32_t V[8] = {
        0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600,
        0xa96f30bc, 0x163138aa, 0xe38dee4d, 0xb0fb0e4e
    };

    size_t block_count = len / 64;
    size_t remaining = len % 64;

    for (size_t i = 0; i < block_count; i++) {
        uint32_t W[132];
        MessageExpansionScalar((const uint32_t*)(data + i * 64), W);
        SM3CompressScalar(V, W);
    }

    uint8_t last_block[128] = { 0 };
    size_t total_bits = len * 8;

    if (len > 0) {
        memcpy(last_block, data + block_count * 64, remaining);
        last_block[remaining] = 0x80;

        if (remaining < 56) {
            last_block[60] = (total_bits >> 24) & 0xFF;
            last_block[61] = (total_bits >> 16) & 0xFF;
            last_block[62] = (total_bits >> 8) & 0xFF;
            last_block[63] = total_bits & 0xFF;

            uint32_t W[132];
            MessageExpansionScalar((const uint32_t*)last_block, W);
            SM3CompressScalar(V, W);
        }
        else {
            last_block[60] = (total_bits >> 24) & 0xFF;
            last_block[61] = (total_bits >> 16) & 0xFF;
            last_block[62] = (total_bits >> 8) & 0xFF;
            last_block[63] = total_bits & 0xFF;

            uint32_t W1[132];
            MessageExpansionScalar((const uint32_t*)last_block, W1);
            SM3CompressScalar(V, W1);

            memset(last_block, 0, 64);
            uint32_t W2[132];
            MessageExpansionScalar((const uint32_t*)last_block, W2);
            SM3CompressScalar(V, W2);
        }
    }

    for (int i = 0; i < 8; i++) {
        digest[i * 4 + 0] = (V[i] >> 24) & 0xFF;
        digest[i * 4 + 1] = (V[i] >> 16) & 0xFF;
        digest[i * 4 + 2] = (V[i] >> 8) & 0xFF;
        digest[i * 4 + 3] = V[i] & 0xFF;
    }
}


// 将哈希值转换为十六进制字符串
std::string SM3Hex(const uint8_t* data, size_t len) {
    uint8_t digest[32];
    SM3(data, len, digest);

    const char hex_chars[] = "0123456789abcdef";
    std::string result;
    result.reserve(64);

    for (int i = 0; i < 32; i++) {
        result += hex_chars[digest[i] >> 4];
        result += hex_chars[digest[i] & 0x0F];
    }

    return result;
}

int main() {
    // 测试数据
    const size_t TEST_SIZE = 1024 *  10; // 10KB
    uint8_t* test_data = new uint8_t[TEST_SIZE];
    memset(test_data, 0x61, TEST_SIZE); // 填充 'a'

    // 测试AVX2优化的SM3算法
    auto start_avx2 = std::chrono::high_resolution_clock::now();
    uint8_t digest_avx2[32];
    for (int i = 0; i < 10; ++i) {
        SM3(test_data, TEST_SIZE, digest_avx2);
    }
    auto end_avx2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_avx2 = end_avx2 - start_avx2;

    // 测试传统SM3算法
    auto start_scalar = std::chrono::high_resolution_clock::now();
    uint8_t digest_scalar[32];
    for (int i = 0; i < 10; ++i) {
        SM3Scalar(test_data, TEST_SIZE, digest_scalar);
    }
    auto end_scalar = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_scalar = end_scalar - start_scalar;

    // 输出结果
    std::cout << "AVX2 Optimized SM3 Time: " << duration_avx2.count() << " seconds" << std::endl;
    std::cout << "Traditional SM3 Time:     " << duration_scalar.count() << " seconds" << std::endl;
    std::cout << "Speedup Ratio:            " << duration_scalar.count() / duration_avx2.count() << std::endl;

    delete[] test_data;
    return 0;
}
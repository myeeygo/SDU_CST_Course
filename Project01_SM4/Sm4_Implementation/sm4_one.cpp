#include <iostream>
#include <iomanip>
#include <cstring>
#include <array>

// 传统sm4
class SM4 {
public:    
    SM4(){};
    void encrypt(unsigned char plaintext[16], unsigned char ciphertext[16], unsigned char key[16]){
        unsigned int roundKeys[32];
        keySchedule(key, roundKeys); // 轮密钥
        processBlock(plaintext, ciphertext, roundKeys);
    };
    void decrypt(unsigned char ciphertext[16], unsigned char plaintext[16], unsigned char key[16]){
        unsigned int roundKeys[32];
        keySchedule(key, roundKeys); // 轮密钥
        // 解轮密钥逆序使用
        unsigned int reverseKeys[32];
        for (int i = 0; i < 32; ++i) {
            reverseKeys[i] = roundKeys[31 - i];
        }
        processBlock(ciphertext, plaintext, reverseKeys);
    };

private:
    // static const unsigned char SM4_SBOX[256];
    // static const unsigned int SM4_CK[32];    
    unsigned int FK[4] = {
        0xA3B1BAC6, 0x56AA3350, 0x677D9197, 0xB27022DC
    };

    // 循环左移
    unsigned int rotateLeft(unsigned int x, int n) {
        unsigned int res = (x << n) | (x >> (32 - n));
        return res;
    }

    // 非线性变换 τ
    unsigned int tau_transform(unsigned int input) {
        unsigned char bytes[4];
        bytes[0] = SM4_SBOX[(input >> 24) & 0xFF];
        bytes[1] = SM4_SBOX[(input >> 16) & 0xFF];
        bytes[2] = SM4_SBOX[(input >> 8) & 0xFF];
        bytes[3] = SM4_SBOX[input & 0xFF];
        unsigned int res = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
        return res;
    }
            
    // 线性变换 L
    unsigned int linear_transform_L(unsigned int input) {
        unsigned int res=input ^ rotateLeft(input, 2) ^rotateLeft(input, 10) ^rotateLeft(input, 18) ^rotateLeft(input, 24);
        return res;
    }

    // 线性变换 L' (用于密钥扩展)
    unsigned int linear_transform_Lprime(unsigned int input) {
        unsigned int res=input ^ rotateLeft(input, 13) ^rotateLeft(input, 23);
        return res;
    }
   

    // 密钥扩展
    void keySchedule(const unsigned char key[16], unsigned int roundKeys[32]) {
        unsigned int K[36];

        // 初始化 K0-K3
        for (int i = 0; i < 4; ++i) {
            K[i] = (key[4 * i] << 24) | (key[4 * i + 1] << 16) |
                (key[4 * i + 2] << 8) | key[4 * i + 3];
            K[i] ^= FK[i]; // 异或固定参数
        }

        // 生成轮密钥
        for (int i = 0; i < 32; ++i) {
            unsigned int T = K[i + 1] ^ K[i + 2] ^ K[i + 3] ^ SM4_CK[i];
            T = tau_transform(T);
            T = linear_transform_Lprime(T);
            K[i + 4] = K[i] ^ T;
            roundKeys[i] = K[i + 4];
        }
    }

    // 轮函数 F
    unsigned int F(unsigned int X0, unsigned int X1, unsigned int X2, unsigned int X3, unsigned int rk) {
        unsigned int T = X1 ^ X2 ^ X3 ^ rk;
        T = tau_transform(T);
        T = linear_transform_L(T);
        T ^= X0;
        return T;
    }
    // 处理数据块
    void processBlock(const unsigned char input[16], unsigned char output[16], const unsigned int roundKeys[32]) {
        unsigned int X[36];
        // 初始化
        for (int i = 0; i < 4; ++i) {
            X[i] = (input[4 * i] << 24) | (input[4 * i + 1] << 16) |
                (input[4 * i + 2] << 8) | input[4 * i + 3];
        }
        // 32轮迭代
        for (int i = 0; i < 32; ++i) {
            X[i + 4] = F(X[i], X[i + 1], X[i + 2], X[i + 3], roundKeys[i]);
        }
        // 输出反序
        for (int i = 0; i < 4; ++i) {
            output[4 * i] = (X[35 - i] >> 24) & 0xFF;
            output[4 * i + 1] = (X[35 - i] >> 16) & 0xFF;
            output[4 * i + 2] = (X[35 - i] >> 8) & 0xFF;
            output[4 * i + 3] = X[35 - i] & 0xFF;
        }
        }

    // SM4 S盒 
const unsigned char SM4_SBOX[256] = {
    0xD6, 0x90, 0xE9, 0xFE, 0xCC, 0xE1, 0x3D, 0xB7, 0x16, 0xB6, 0x14, 0xC2, 0x28, 0xFB, 0x2C, 0x05,
    0x2B, 0x67, 0x9A, 0x76, 0x2A, 0xBE, 0x04, 0xC3, 0xAA, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99,
    0x9C, 0x42, 0x50, 0xF4, 0x91, 0xEF, 0x98, 0x7A, 0x33, 0x54, 0x0B, 0x43, 0xED, 0xCF, 0xAC, 0x62,
    0xE4, 0xB3, 0x1C, 0xA9, 0xC9, 0x08, 0xE8, 0x95, 0x80, 0xDF, 0x94, 0xFA, 0x75, 0x8F, 0x3F, 0xA6,
    0x47, 0x07, 0xA7, 0xFC, 0xF3, 0x73, 0x17, 0xBA, 0x83, 0x59, 0x3C, 0x19, 0xE6, 0x85, 0x4F, 0xA8,
    0x68, 0x6B, 0x81, 0xB2, 0x71, 0x64, 0xDA, 0x8B, 0xF8, 0xEB, 0x0F, 0x4B, 0x70, 0x56, 0x9D, 0x35,
    0x1E, 0x24, 0x0E, 0x5E, 0x63, 0x58, 0xD1, 0xA2, 0x25, 0x22, 0x7C, 0x3B, 0x01, 0x21, 0x78, 0x87,
    0xD4, 0x00, 0x46, 0x57, 0x9F, 0xD3, 0x27, 0x52, 0x4C, 0x36, 0x02, 0xE7, 0xA0, 0xC4, 0xC8, 0x9E,
    0xEA, 0xBF, 0x8A, 0xD2, 0x40, 0xC7, 0x38, 0xB5, 0xA3, 0xF7, 0xF2, 0xCE, 0xF9, 0x61, 0x15, 0xA1,
    0xE0, 0xAE, 0x5D, 0xA4, 0x9B, 0x34, 0x1A, 0x55, 0xAD, 0x93, 0x32, 0x30, 0xF5, 0x8C, 0xB1, 0xE3,
    0x1D, 0xF6, 0xE2, 0x2E, 0x82, 0x66, 0xCA, 0x60, 0xC0, 0x29, 0x23, 0xAB, 0x0D, 0x53, 0x4E, 0x6F,
    0xD5, 0xDB, 0x37, 0x45, 0xDE, 0xFD, 0x8E, 0x2F, 0x03, 0xFF, 0x6A, 0x72, 0x6D, 0x6C, 0x5B, 0x51,
    0x8D, 0x1B, 0xAF, 0x92, 0xBB, 0xDD, 0xBC, 0x7F, 0x11, 0xD9, 0x5C, 0x41, 0x1F, 0x10, 0x5A, 0xD8,
    0x0A, 0xC1, 0x31, 0x88, 0xA5, 0xCD, 0x7B, 0xBD, 0x2D, 0x74, 0xD0, 0x12, 0xB8, 0xE5, 0xB4, 0xB0,
    0x89, 0x69, 0x97, 0x4A, 0x0C, 0x96, 0x77, 0x7E, 0x65, 0xB9, 0xF1, 0x09, 0xC5, 0x6E, 0xC6, 0x84,
    0x18, 0xF0, 0x7D, 0xEC, 0x3A, 0xDC, 0x4D, 0x20, 0x79, 0xEE, 0x5F, 0x3E, 0xD7, 0xCB, 0x39, 0x48
};

// 固定参数 CK 
const unsigned int SM4_CK[32] = {
    0x00070E15, 0x1C232A31, 0x383F464D, 0x545B6269,
    0x70777E85, 0x8C939AA1, 0xA8AFB6BD, 0xC4CBD2D9,
    0xE0E7EEF5, 0xFC030A11, 0x181F262D, 0x343B4249,
    0x50575E65, 0x6C737A81, 0x888F969D, 0xA4ABB2B9,
    0xC0C7CED5, 0xDCE3EAF1, 0xF8FF060D, 0x141B2229,
    0x30373E45, 0x4C535A61, 0x686F767D, 0x848B9299,
    0xA0A7AEB5, 0xBCC3CAD1, 0xD8DFE6ED, 0xF4FB0209,
    0x10171E25, 0x2C333A41, 0x484F565D, 0x646B7279
};



};

// 优化的sm4
class OptimizedSM4 {
    public:
        OptimizedSM4() {
            // 预计算 T 表
            for (int i = 0; i < 256; ++i) {
                unsigned int b = SM4_SBOX[i];
                unsigned int t = b << 24;
                T0[i] = t ^ rotateLeft(t, 2) ^ rotateLeft(t, 10) ^rotateLeft(t, 18) ^ rotateLeft(t, 24);    
                t = b << 16;
                T1[i] = t ^ rotateLeft(t, 2) ^ rotateLeft(t, 10) ^rotateLeft(t, 18) ^ rotateLeft(t, 24);    
                t = b << 8;
                T2[i] = t ^ rotateLeft(t, 2) ^ rotateLeft(t, 10) ^rotateLeft(t, 18) ^ rotateLeft(t, 24);    
                t = b;
                T3[i] = t ^ rotateLeft(t, 2) ^ rotateLeft(t, 10) ^rotateLeft(t, 18) ^ rotateLeft(t, 24);
            }
    
            // 预计算 T' 表 ,用于密钥扩展
            for (int i = 0; i < 256; ++i) {
                unsigned int b = SM4_SBOX[i];
                unsigned int t = b << 24;
                T0_prime[i] = t ^ rotateLeft(t, 13) ^ rotateLeft(t, 23);    
                t = b << 16;
                T1_prime[i] = t ^ rotateLeft(t, 13) ^ rotateLeft(t, 23);    
                t = b << 8;
                T2_prime[i] = t ^ rotateLeft(t, 13) ^ rotateLeft(t, 23);    
                t = b;
                T3_prime[i] = t ^ rotateLeft(t, 13) ^ rotateLeft(t, 23);
            }
        }
    
        // 加密
        void encrypt(const unsigned char input[16], unsigned char output[16],
            const unsigned char key[16]) {
            std::array<unsigned int, 32> roundKeys;
            keySchedule(key, roundKeys);
            processBlock(input, output, roundKeys);
        }
    
        // 解密
        void decrypt(const unsigned char input[16], unsigned char output[16],
            const unsigned char key[16]) {
            std::array<unsigned int, 32> roundKeys;
            keySchedule(key, roundKeys);    
            // 反转轮密钥
            std::array<unsigned int, 32> reverseKeys;
            for (int i = 0; i < 32; ++i) {
                reverseKeys[i] = roundKeys[31 - i];
            }    
            processBlock(input, output, reverseKeys);
        }
    
    private:
        // T 表用于加密
        unsigned int T0[256], T1[256], T2[256], T3[256];
        // T' 表用于密钥扩展
        unsigned int T0_prime[256], T1_prime[256], T2_prime[256], T3_prime[256];    
        // SM4 算法常量定义
        static constexpr unsigned char SM4_SBOX[256] = {
            0xD6, 0x90, 0xE9, 0xFE, 0xCC, 0xE1, 0x3D, 0xB7, 0x16, 0xB6, 0x14, 0xC2, 0x28, 0xFB, 0x2C, 0x05,
            0x2B, 0x67, 0x9A, 0x76, 0x2A, 0xBE, 0x04, 0xC3, 0xAA, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99,
            0x9C, 0x42, 0x50, 0xF4, 0x91, 0xEF, 0x98, 0x7A, 0x33, 0x54, 0x0B, 0x43, 0xED, 0xCF, 0xAC, 0x62,
            0xE4, 0xB3, 0x1C, 0xA9, 0xC9, 0x08, 0xE8, 0x95, 0x80, 0xDF, 0x94, 0xFA, 0x75, 0x8F, 0x3F, 0xA6,
            0x47, 0x07, 0xA7, 0xFC, 0xF3, 0x73, 0x17, 0xBA, 0x83, 0x59, 0x3C, 0x19, 0xE6, 0x85, 0x4F, 0xA8,
            0x68, 0x6B, 0x81, 0xB2, 0x71, 0x64, 0xDA, 0x8B, 0xF8, 0xEB, 0x0F, 0x4B, 0x70, 0x56, 0x9D, 0x35,
            0x1E, 0x24, 0x0E, 0x5E, 0x63, 0x58, 0xD1, 0xA2, 0x25, 0x22, 0x7C, 0x3B, 0x01, 0x21, 0x78, 0x87,
            0xD4, 0x00, 0x46, 0x57, 0x9F, 0xD3, 0x27, 0x52, 0x4C, 0x36, 0x02, 0xE7, 0xA0, 0xC4, 0xC8, 0x9E,
            0xEA, 0xBF, 0x8A, 0xD2, 0x40, 0xC7, 0x38, 0xB5, 0xA3, 0xF7, 0xF2, 0xCE, 0xF9, 0x61, 0x15, 0xA1,
            0xE0, 0xAE, 0x5D, 0xA4, 0x9B, 0x34, 0x1A, 0x55, 0xAD, 0x93, 0x32, 0x30, 0xF5, 0x8C, 0xB1, 0xE3,
            0x1D, 0xF6, 0xE2, 0x2E, 0x82, 0x66, 0xCA, 0x60, 0xC0, 0x29, 0x23, 0xAB, 0x0D, 0x53, 0x4E, 0x6F,
            0xD5, 0xDB, 0x37, 0x45, 0xDE, 0xFD, 0x8E, 0x2F, 0x03, 0xFF, 0x6A, 0x72, 0x6D, 0x6C, 0x5B, 0x51,
            0x8D, 0x1B, 0xAF, 0x92, 0xBB, 0xDD, 0xBC, 0x7F, 0x11, 0xD9, 0x5C, 0x41, 0x1F, 0x10, 0x5A, 0xD8,
            0x0A, 0xC1, 0x31, 0x88, 0xA5, 0xCD, 0x7B, 0xBD, 0x2D, 0x74, 0xD0, 0x12, 0xB8, 0xE5, 0xB4, 0xB0,
            0x89, 0x69, 0x97, 0x4A, 0x0C, 0x96, 0x77, 0x7E, 0x65, 0xB9, 0xF1, 0x09, 0xC5, 0x6E, 0xC6, 0x84,
            0x18, 0xF0, 0x7D, 0xEC, 0x3A, 0xDC, 0x4D, 0x20, 0x79, 0xEE, 0x5F, 0x3E, 0xD7, 0xCB, 0x39, 0x48
        };    
        static constexpr unsigned int FK[4] = {
            0xA3B1BAC6, 0x56AA3350, 0x677D9197, 0xB27022DC
        };    
        static constexpr unsigned int SM4_CK[32] = {
            0x00070E15, 0x1C232A31, 0x383F464D, 0x545B6269,
            0x70777E85, 0x8C939AA1, 0xA8AFB6BD, 0xC4CBD2D9,
            0xE0E7EEF5, 0xFC030A11, 0x181F262D, 0x343B4249,
            0x50575E65, 0x6C737A81, 0x888F969D, 0xA4ABB2B9,
            0xC0C7CED5, 0xDCE3EAF1, 0xF8FF060D, 0x141B2229,
            0x30373E45, 0x4C535A61, 0x686F767D, 0x848B9299,
            0xA0A7AEB5, 0xBCC3CAD1, 0xD8DFE6ED, 0xF4FB0209,
            0x10171E25, 0x2C333A41, 0x484F565D, 0x646B7279
        };    
        // 循环左移
        static unsigned int rotateLeft(unsigned int x, int n) {
            return (x << n) | (x >> (32 - n));
        }    
        // 使用 T 表优化的轮函数
        unsigned int F(unsigned int X0, unsigned int X1, unsigned int X2,
            unsigned int X3, unsigned int rk) {
            unsigned int T = X1 ^ X2 ^ X3 ^ rk;    
            // 使用预计算的 T 表
            return X0 ^ T0[(T >> 24) & 0xFF] ^T1[(T >> 16) & 0xFF] ^T2[(T >> 8) & 0xFF] ^T3[T & 0xFF];
        }
    
        // 使用 T' 表优化的密钥扩展
        void keySchedule(const unsigned char key[16],
            std::array<unsigned int, 32>& roundKeys) {
            std::array<unsigned int, 36> K;    
            // 初始化 K0-K3
            for (int i = 0; i < 4; ++i) {
                K[i] = (key[4 * i] << 24) | (key[4 * i + 1] << 16) |
                    (key[4 * i + 2] << 8) | key[4 * i + 3];
                K[i] ^= FK[i];
            }    
            // 生成轮密钥（使用 T' 表优化）
            for (int i = 0; i < 32; ++i) {
                unsigned int T = K[i + 1] ^ K[i + 2] ^ K[i + 3] ^ SM4_CK[i];
    
                // 使用预计算的 T' 表
                T = T0_prime[(T >> 24) & 0xFF] ^
                    T1_prime[(T >> 16) & 0xFF] ^
                    T2_prime[(T >> 8) & 0xFF] ^
                    T3_prime[T & 0xFF];
    
                K[i + 4] = K[i] ^ T;
                roundKeys[i] = K[i + 4];
            }
        }
    
        // 处理数据块（优化内存布局）
        void processBlock(const unsigned char input[16], unsigned char output[16],
            const std::array<unsigned int, 32>& roundKeys) {
            std::array<unsigned int, 36> X;
    
            // 初始化 X0-X3
            for (int i = 0; i < 4; ++i) {
                X[i] = (input[4 * i] << 24) | (input[4 * i + 1] << 16) |
                    (input[4 * i + 2] << 8) | input[4 * i + 3];
            }
    
            // 32轮迭代（展开循环减少分支预测）
            for (int i = 0; i < 32; i += 4) {
                X[i + 4] = F(X[i], X[i + 1], X[i + 2], X[i + 3], roundKeys[i]);
                X[i + 5] = F(X[i + 1], X[i + 2], X[i + 3], X[i + 4], roundKeys[i + 1]);
                X[i + 6] = F(X[i + 2], X[i + 3], X[i + 4], X[i + 5], roundKeys[i + 2]);
                X[i + 7] = F(X[i + 3], X[i + 4], X[i + 5], X[i + 6], roundKeys[i + 3]);
            }
    
            // 最终输出（反序）
            for (int i = 0; i < 4; ++i) {
                output[4 * i] = (X[35] >> 24) & 0xFF;
                output[4 * i + 1] = (X[35] >> 16) & 0xFF;
                output[4 * i + 2] = (X[35] >> 8) & 0xFF;
                output[4 * i + 3] = X[35] & 0xFF;
                // 循环移位代替反序操作
                unsigned int temp = X[35];
                for (int j = 35; j > 32; --j) {
                    X[j] = X[j - 1];
                }
                X[32] = temp;
            }
        }
    };
    
// 测试函数
int testFunc_SM4() {
    SM4 sm4;
    OptimizedSM4 sm4Optimized;
    // 测试数据
    unsigned char key[16] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
        0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
    };
    unsigned char plaintext[16] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
        0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
    };
    unsigned char ciphertext[16];
    unsigned char decrypted[16];
    std::cout << "————————————————Traditional SM4 Function————————————————————" << std::endl;
    // 加密
    sm4.encrypt(plaintext, ciphertext, key);
    std::cout << "Ciphertext: ";
    for (int i = 0; i < 16; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
            << (int)ciphertext[i] << " ";
    }
    std::cout << std::endl;
    // 解密
    sm4.decrypt(ciphertext, decrypted, key);
    std::cout << "Decrypted:  ";
    for (int i = 0; i < 16; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
            << (int)decrypted[i] << " ";
    }
    std::cout << std::endl;
    // 验证解密结果
    if (memcmp(plaintext, decrypted, 16) == 0) {
        std::cout << "[sm4]Encryption and Decryption successful!" << std::endl;
    }
    else {
        std::cout << "[sm4]Encryption or Decryption failed!" << std::endl;
    }
    // 优化后的加密    
    std::cout << "————————————————Optimized SM4 Function————————————————————" << std::endl;

    sm4Optimized.encrypt(plaintext, ciphertext, key);
    std::cout << "Ciphertext: ";
    for (int i = 0; i < 16; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
            << (int)ciphertext[i] << " ";
    }
    std::cout << std::endl;
    // 解密
    sm4Optimized.decrypt(ciphertext, decrypted, key);
    std::cout << "Decrypted:  ";
    for (int i = 0; i < 16; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
            << (int)decrypted[i] << " ";
    }
    std::cout << std::endl;
    // 验证解密结果
    if (memcmp(plaintext, decrypted, 16) == 0) {
        std::cout << "[sm4Optimized]Encryption and Decryption successful!" << std::endl;
    }
    else {
        std::cout << "[sm4Optimized]Encryption or Decryption failed!" << std::endl;
    }

    return 0;
}


// 主函数
int main() {
    testFunc_SM4();
    return 0;
}

/**
实际运行输出：

————————————————Traditional SM4 Function————————————————————
Ciphertext: 68 1e df 34 d2 06 96 5e 86 b3 e9 4f 53 6e 42 46
Decrypted:  01 23 45 67 89 ab cd ef fe dc ba 98 76 54 32 10
[sm4]Encryption and Decryption successful!
————————————————Optimized SM4 Function————————————————————
Ciphertext: 68 1e df 34 d2 06 96 5e 86 b3 e9 4f 53 6e 42 46
Decrypted:  01 23 45 67 89 ab cd ef fe dc ba 98 76 54 32 10
[sm4Optimized]Encryption and Decryption successful! 

 */ 
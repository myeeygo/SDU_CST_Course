import hashlib
import random
from math import gcd

# 群G的参数（素数阶群，示例用小参数）
G_p = 23        # 群的模数（素数）
G_q = 11        # 群的阶（素数，G_p-1必须是G_q的倍数，23-1=22=2×11）
G_g = 2         # 群的生成元，满足G_g^G_q ≡ 1 mod G_p

# Paillier同态加密的参数（示例用较大参数）
paillier_p = 101  # 较大的素数
paillier_q = 103  # 较大的素数（与群的阶不同，避免冲突）

def H(u):
    """哈希函数，将标识符u映射到群G的元素"""
    h_bytes = hashlib.sha256(u.encode()).digest()
    h_int = int.from_bytes(h_bytes, 'big')
    exponent = h_int % G_q  # 映射到群的指数范围
    return pow(G_g, exponent, G_p)

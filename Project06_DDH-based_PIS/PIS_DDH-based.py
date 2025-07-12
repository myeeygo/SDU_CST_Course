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


class Paillier:
    def __init__(self, p=None, q=None, n=None, g=None):
        if p and q:
            # 解密方：需知道p和q，生成完整密钥（含私钥参数lam、mu）
            self.n = p * q
            lam = (p-1) * (q-1) // gcd(p-1, q-1)  # λ = lcm(p-1, q-1)
            self.lam = lam
            self.g = self.n + 1                   # 公钥g = n+1（标准Paillier选择）
            self.mu = pow(lam, -1, self.n)        # μ = λ⁻¹ mod n
        elif n and g:
            # 加密方：仅需公钥n和g，无法解密
            self.n = n
            self.g = g
        else:
            raise ValueError("需提供 (p,q) 作为解密方，或 (n,g) 作为加密方.")

    def encrypt(self, m, r=None):
        """加密（仅加密方可用，无需私钥）"""
        if m < 0 or m >= self.n:
            raise ValueError(f"明文必须满足 0 ≤ m < {self.n}，但输入为 {m}")
        n_sq = self.n * self.n
        if r is None:
            r = random.randint(1, self.n-1)
        c = (pow(self.g, m, n_sq) * pow(r, self.n, n_sq)) % n_sq
        return c

    def decrypt(self, c):
        """解密（仅解密方可调用，需私钥参数lam、mu）"""
        if not hasattr(self, 'lam') or not hasattr(self, 'mu'):
            raise PermissionError("加密方无法解密，需解密方私钥.")
        n_sq = self.n * self.n
        c_lam = pow(c, self.lam, n_sq)
        l = (c_lam - 1) // self.n
        m = (l * self.mu) % self.n
        return m

    def add(self, c1, c2):
        """同态加法（加密方、解密方均可用）"""
        n_sq = self.n * self.n
        return (c1 * c2) % n_sq

    def refresh(self, c):
        """密文刷新（加密方可用，随机化密文）"""
        n_sq = self.n * self.n
        r = random.randint(1, self.n-1)
        return (c * pow(r, self.n, n_sq)) % n_sq

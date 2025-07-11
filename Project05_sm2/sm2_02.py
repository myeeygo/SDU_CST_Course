from hashlib import sha256
from typing import Tuple

# 常量定义
P = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF  # 大素数 P
A = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC  # 椭圆曲线参数 A
B = 0x28E0F64D09E8F0B2A160C7D0E8E9BB9C7F84A0FF15747E1A8BCDE9E8707F0F6  # 椭圆曲线参数 B
Gx = 0x32C4AE2C1F1981195F9902B2C0AA36C1D6C2AF9D3A1D5F9F34A3A37B75C4E58  # 基点 G 的 x 坐标
Gy = 0xBCF1BCDBA0F12F6578A1E973AEEAD8C65DBF6C358B515F6F8DB0241F0B28C32  # 基点 G 的 y 坐标
N = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123  # 阶 N

# 计算模逆
def mod_inv(a: int, p: int) -> int:
    return pow(a, p - 2, p)

# 蒙哥马利模乘（简化版）
def montgomery_mul(x: int, y: int, p: int, r: int) -> int:
    """蒙哥马利模乘：计算 (x * y) % p"""
    t = (x * y) % p
    return t

# 椭圆曲线加法（使用 Jacobian 坐标）
def elliptic_add(x1: int, y1: int, z1: int, x2: int, y2: int, z2: int, p: int) -> Tuple[int, int, int]:
    """椭圆曲线加法：使用 Jacobian 坐标"""
    if z1 == 0:
        return x2, y2, z2
    if z2 == 0:
        return x1, y1, z1

    # 计算 lambda = (y2 - y1) / (x2 - x1)
    u1 = y2 - y1
    u2 = x2 - x1
    s1 = u2 * u2 % p
    s2 = u1 * u2 % p
    h = (s1 - 2 * s2) % p
    v = (u1 * (s1 - 2 * s2)) % p

    # 更新结果
    x3 = h * h - 2 * v
    y3 = (h * (v - x3)) - s2 * s2 % p
    z3 = (2 * z1 * z2) % p
    return x3 % p, y3 % p, z3 % p

# 椭圆曲线倍点（使用 Jacobian 坐标）
def elliptic_double(x1: int, y1: int, z1: int, p: int) -> Tuple[int, int, int]:
    """椭圆曲线倍点：使用 Jacobian 坐标"""
    if z1 == 0:
        return 0, 0, 0
    
    # 计算倍点的 lambda
    s1 = 3 * x1 * x1 + A * z1 * z1
    s2 = 2 * y1
    h = s1 * s1 - 2 * x1 * s2
    v = (s1 * (x1 * s2 - h)) - y1 * s2 * s2
    x3 = h * h - 2 * v
    y3 = (h * (v - x3)) - s2 * s2 % p
    z3 = 2 * z1 * y1 % p
    return x3 % p, y3 % p, z3 % p

# 使用 NAF 进行椭圆曲线点乘
def naf_scalar_multiply(k: int, P: Tuple[int, int], p: int) -> Tuple[int, int]:
    """使用 NAF 进行椭圆曲线点乘"""
    R = (0, 0, 1)  # 使用 Jacobian 坐标
    Q = (P[0], P[1], 1)  # 初始化 P 点
    while k > 0:
        if k & 1:  # 若 k 最低位是 1，则加上 P
            R = elliptic_add(R[0], R[1], R[2], Q[0], Q[1], Q[2], p)
        Q = elliptic_double(Q[0], Q[1], Q[2], p)
        k >>= 1
    return R[0], R[1]  # 返回 (x, y)

# 生成密钥对
def generate_keypair() -> Tuple[int, Tuple[int, int]]:
    """生成 SM2 密钥对"""
    private_key = 1234567890  # 示例私钥
    P = (Gx, Gy)
    public_key = naf_scalar_multiply(private_key, P, P)
    return private_key, public_key

# 生成签名
def sign(message: bytes, private_key: int) -> Tuple[int, int]:
    """生成 SM2 签名"""
    e = int(sha256(message).hexdigest(), 16)  # 消息哈希
    k = 1234567890  # 随机数 k
    P = (Gx, Gy)
    R = naf_scalar_multiply(k, P, P)
    r = R[0] % N
    s = (mod_inv(k, N) * (e + private_key * r)) % N
    return r, s

# 验证签名
def verify(message: bytes, public_key: Tuple[int, int], r: int, s: int) -> bool:
    """验证 SM2 签名"""
    e = int(sha256(message).hexdigest(), 16)  # 消息哈希
    if r <= 0 or r >= N or s <= 0 or s >= N:
        return False
    
    w = mod_inv(s, N)
    P = public_key
    P1 = naf_scalar_multiply(w * e, P, P)
    P2 = naf_scalar_multiply(w * r, (Gx, Gy), P)
    R = elliptic_add(P1[0], P1[1], P1[2], P2[0], P2[1], P2[2], P)
    
    return R[0] % N == r

# 测试密钥生成、签名与验证
private_key, public_key = generate_keypair()
message = b"Hello, SM2!"
r, s = sign(message, private_key)
valid = verify(message, public_key, r, s)
print(f"签名有效: {valid}")

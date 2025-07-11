import secrets
from hashlib import sha256
from gmssl import sm3, func

class SM2:
    # SM2椭圆曲线参数
    A = 0
    B = 7
    # 有限域的阶
    Q = 115792089237316195423570985008687907853269984665640564039457584007908834671663
    # 椭圆曲线的阶
    N = 115792089237316195423570985008687907852837564279074904382605163141518161494337
    G_X = 55066263022277343669578718895168534326250603453777594175500187360389116729240
    G_Y = 32670510020758816978083085130507043184471273380659243275938904335757337482424
    G = (G_X, G_Y)
    
    @staticmethod
    def legendre(y, p):
        """计算勒让德符号，返回y^(p-1)/2 mod p的值"""
        return pow(y, (p-1)//2, p)
    
    @staticmethod
    def tonelli_shanks(n, p):
        """Tonelli-Shanks算法，求二次剩余(x^2=y mod p)"""
        assert SM2.legendre(n, p) == 1, "不是二次剩余"
        
        if p % 4 == 3:
            return pow(n, (p + 1) // 4, p)
            
        q = p - 1
        s = 0
        while q % 2 == 0:
            q = q // 2
            s += 1
            
        # 寻找非二次剩余z
        for z in range(2, p):
            if SM2.legendre(z, p) == p - 1:
                c = pow(z, q, p)
                break
                
        r = pow(n, (q + 1) // 2, p)
        t = pow(n, q, p)
        m = s
        
        if t % p == 1:
            return r
        else:
            i = 0
            while t % p != 1:
                temp = pow(t, 2 ** (i + 1), p)
                i += 1
                if temp % p == 1:
                    b = pow(c, 2 ** (m - i - 1), p)
                    r = r * b % p
                    c = b * b % p
                    t = t * c % p
                    m = i
                    i = 0
            return r
    
    @staticmethod
    def extended_euclidean(a, b):
        """扩展欧几里得算法，计算gcd和贝祖系数"""
        if b == 0:
            return (a, 1, 0)
        else:
            g, x, y = SM2.extended_euclidean(b, a % b)
            return (g, y, x - (a // b) * y)
    
    @staticmethod
    def mod_inverse(a, n):
        """计算模逆元，即满足(a * x) % n == 1的x值"""
        g, x, y = SM2.extended_euclidean(a, n)
        if g != 1:
            raise ValueError("逆元不存在")
        else:
            return x % n
    
    @staticmethod
    def elliptic_add(p, q):
        """椭圆曲线上的点加法"""
        if p == 0:
            return q
        if q == 0:
            return p
            
        x1, y1 = p
        x2, y2 = q
        
        if x1 == x2 and y1 != y2:
            return 0  # 无穷远点
            
        if x1 == x2:
            # 计算切线斜率
            s = (3 * x1 * x1 + SM2.A) * SM2.mod_inverse(2 * y1, SM2.Q) % SM2.Q
        else:
            # 计算割线斜率
            s = (y2 - y1) * SM2.mod_inverse(x2 - x1, SM2.Q) % SM2.Q
            
        x3 = (s * s - x1 - x2) % SM2.Q
        y3 = (s * (x1 - x3) - y1) % SM2.Q
        
        return (x3, y3)
    
    @staticmethod
    def elliptic_double(p):
        """椭圆曲线上的倍点运算"""
        return SM2.elliptic_add(p, p)
    
    @staticmethod
    def elliptic_mult(k, p):
        """椭圆曲线上的标量乘法，使用蒙哥马利梯子算法提高安全性"""
        if k == 0:
            return 0
            
        k_bin = bin(k)[2:]
        r0 = 0
        r1 = p
        
        for bit in k_bin:
            if bit == '0':
                r1 = SM2.elliptic_add(r0, r1)
                r0 = SM2.elliptic_double(r0)
            else:
                r0 = SM2.elliptic_add(r0, r1)
                r1 = SM2.elliptic_double(r1)
                
        return r0
    
    @staticmethod
    def get_bit_num(x):
        """获取数值的二进制位数"""
        if isinstance(x, int):
            return x.bit_length()
        elif isinstance(x, str):
            return len(x.encode()) * 8
        elif isinstance(x, bytes):
            return len(x) * 8
        return 0
    
    @staticmethod
    def pre_compute(ID, a, b, G_X, G_Y, x_A, y_A):
        """预计算Z_A，用于用户标识和公钥的哈希"""
        a_str = str(a)
        b_str = str(b)
        G_X_str = str(G_X)
        G_Y_str = str(G_Y)
        x_A_str = str(x_A)
        y_A_str = str(y_A)
        ENTL = str(SM2.get_bit_num(ID))
        
        t = ENTL + ID + a_str + b_str + G_X_str + G_Y_str + x_A_str + y_A_str
        t_bytes = t.encode('utf-8')
        digest = sm3.sm3_hash(func.bytes_to_list(t_bytes))
        return int(digest, 16)
    
    @classmethod
    def generate_key(cls):
        """生成SM2公私钥对"""
        while True:
            private_key = secrets.randbelow(cls.N - 1) + 1
            public_key = cls.elliptic_mult(private_key, cls.G)
            if public_key != 0:  # 确保公钥不是无穷远点
                return private_key, public_key
    
    @classmethod
    def sign(cls, private_key, message, Z_A, user_id="1234567812345678"):
        """SM2签名算法"""
        if not isinstance(message, str):
            message = str(message)
            
        # 计算e = H(Z_A || M)
        M = str(Z_A) + message
        M_bytes = M.encode('utf-8')
        e = sm3.sm3_hash(func.bytes_to_list(M_bytes))
        e = int(e, 16)
        
        # 生成随机数k
        while True:
            k = secrets.randbelow(cls.N - 1) + 1
            random_point = cls.elliptic_mult(k, cls.G)
            x1 = random_point[0]
            r = (e + x1) % cls.N
            if r == 0 or r + k == cls.N:
                continue
                
            s = (cls.mod_inverse(1 + private_key, cls.N) * (k - r * private_key)) % cls.N
            if s != 0:
                break
                
        return (r, s)
    
    @classmethod
    def verify(cls, public_key, ID, message, signature):
        """SM2签名验证算法"""
        r, s = signature
        
        # 验证r和s的范围
        if not (1 <= r <= cls.N - 1 and 1 <= s <= cls.N - 1):
            return False
            
        # 计算Z_A
        Z = cls.pre_compute(ID, cls.A, cls.B, cls.G_X, cls.G_Y, public_key[0], public_key[1])
        
        # 计算e = H(Z || M)
        M = str(Z) + message
        M_bytes = M.encode('utf-8')
        e = sm3.sm3_hash(func.bytes_to_list(M_bytes))
        e = int(e, 16)
        
        t = (r + s) % cls.N
        if t == 0:
            return False
            
        # 计算sG + tP
        point1 = cls.elliptic_mult(s, cls.G)
        point2 = cls.elliptic_mult(t, public_key)
        point = cls.elliptic_add(point1, point2)
        
        if point == 0:
            return False
            
        x1 = point[0]
        R = (e + x1) % cls.N
        
        return R == r

if __name__ == '__main__':
    # 生成密钥对
    private_key, public_key = SM2.generate_key()
    message = "202000460012"
    ID = "1234567812345678"
    
    print("=" * 50)
    print("SM2算法演示")
    print("=" * 50)
    print(f"消息: {message}")
    print(f"公钥: {public_key}")
    
    # 预计算Z_A
    Z_A = SM2.pre_compute(ID, SM2.A, SM2.B, SM2.G_X, SM2.G_Y, public_key[0], public_key[1])
    
    # 签名
    signature = SM2.sign(private_key, message, Z_A, ID)
    print(f"签名: {signature}")
    
    # 验证
    valid = SM2.verify(public_key, ID, message, signature)
    print(f"验证结果: {'通过' if valid else '失败'}")
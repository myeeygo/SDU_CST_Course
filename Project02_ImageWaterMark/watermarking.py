import cv2
import numpy as np
from typing import Tuple, Optional

class WatermarkingSystem:
    def __init__(self, alpha: float = 0.1, seed: int = 42, watermark_image_path: Optional[str] = None):
        self.alpha = alpha      #  水印强度参数
        np.random.seed(seed)
        self.original_dct: Optional[np.ndarray] = None
        self.watermark: Optional[np.ndarray] = None
        self.original_shape: Optional[Tuple[int, int]] = None
        self.watermark_image_path = watermark_image_path    # 水印图像路径
    
    # 处理自定义水印图像：调整大小，二值化
    def process_custom_watermark(self, shape: Tuple[int, int]) -> np.ndarray:
        if self.watermark_image_path is None:
            raise ValueError("未提供水印图像路径")
            
        watermark_img = cv2.imread(self.watermark_image_path, cv2.IMREAD_GRAYSCALE)
        if watermark_img is None:
            raise FileNotFoundError(f"无法读取水印图像 {self.watermark_image_path}")
            
        watermark_img = cv2.resize(watermark_img, (shape[1], shape[0]))
        
        _, binary_watermark = cv2.threshold(
            watermark_img, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU
        )
        
        binary_watermark = np.where(binary_watermark > 127, 1, -1)
        return binary_watermark.astype(np.int8)
    
    # 生成水印：根据是否提供水印图像路径选择生成方式
    def generate_watermark(self, shape: Tuple[int, int]) -> np.ndarray:
        if self.watermark_image_path:
            return self.process_custom_watermark(shape)
        else:
            return np.random.choice([-1, 1], size=shape[:2])
        
    
    # 嵌入水印：读取图像，转换为YCbCr，分离Y通道，进行DCT变换，
    # 嵌入水印，进行逆DCT变换，合并通道，转换为BGR，保存图像
    def embed(self, image_path: str, output_path: str) -> np.ndarray:
        image = cv2.imread(image_path, cv2.IMREAD_COLOR)
        if image is None:
            raise FileNotFoundError(f"无法读取图像文件 {image_path}")
            
        self.original_shape = image.shape[:2]
        
        image_ycrcb = cv2.cvtColor(image, cv2.COLOR_BGR2YCrCb)
        y, cr, cb = cv2.split(image_ycrcb)
        
        self.watermark = self.generate_watermark(y.shape)
        
        dct_y = cv2.dct(np.float32(y))
        
        self.original_dct = dct_y.copy()
        
        height, width = dct_y.shape
        center_x, center_y = width // 2, height // 2
        radius = min(center_x, center_y) // 2
        
        y_indices, x_indices = np.ogrid[:height, :width]
        mask = ((x_indices - center_x)**2 + (y_indices - center_y)**2) <= radius**2
        
        dct_y[mask] += self.alpha * self.watermark[mask] * np.abs(dct_y[mask])
        
        watermarked_y = cv2.idct(dct_y)
        watermarked_y = np.uint8(np.clip(watermarked_y, 0, 255))
        
        watermarked_ycrcb = cv2.merge([watermarked_y, cr, cb])
        watermarked_bgr = cv2.cvtColor(watermarked_ycrcb, cv2.COLOR_YCrCb2BGR)
        cv2.imwrite(output_path, watermarked_bgr)
        
        return watermarked_bgr
    
    # 提取水印
    def extract(self, watermarked_image: np.ndarray) -> np.ndarray:
        if self.original_dct is None or self.watermark is None:
            raise RuntimeError("请先嵌入水印再进行提取")
            
        if watermarked_image.shape[:2] != self.original_shape:
            watermarked_image = cv2.resize(
                watermarked_image, 
                (self.original_shape[1], self.original_shape[0])
            )
        
        watermarked_ycrcb = cv2.cvtColor(watermarked_image, cv2.COLOR_BGR2YCrCb)
        y, _, _ = cv2.split(watermarked_ycrcb)
        
        dct_y = cv2.dct(np.float32(y))
        
        height, width = dct_y.shape     # 选择中频区域嵌入水印
        center_x, center_y = width // 2, height // 2
        radius = min(center_x, center_y) // 2
        
        y_indices, x_indices = np.ogrid[:height, :width]
        mask = ((x_indices - center_x)**2 + (y_indices - center_y)**2) <= radius**2
        
        extracted_watermark = np.zeros_like(self.watermark, dtype=np.float32)
        
        for i in range(height):
            for j in range(width):
                if mask[i, j]:
                    if np.abs(self.original_dct[i, j]) > 1e-4:
                        diff = dct_y[i, j] - self.original_dct[i, j]
                        extracted_watermark[i, j] = diff / (self.alpha * np.abs(self.original_dct[i, j]))
        
        extracted_watermark = np.sign(extracted_watermark)
        return extracted_watermark
    
    # 计算NCC
    def calculate_ncc(self, original_watermark: np.ndarray, extracted_watermark: np.ndarray) -> float:
        valid_mask = (original_watermark != 0) & (extracted_watermark != 0)
        
        if np.any(valid_mask):
            orig_valid = original_watermark[valid_mask]
            extr_valid = extracted_watermark[valid_mask]
            
            numerator = np.sum(orig_valid * extr_valid)
            denominator = np.sqrt(np.sum(orig_valid**2) * np.sum(extr_valid**2))
            
            if denominator > 1e-10: # 避免除以0
                return numerator / denominator
        return 0.0
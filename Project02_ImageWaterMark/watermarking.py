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
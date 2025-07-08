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
    
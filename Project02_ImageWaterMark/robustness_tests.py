import cv2
import numpy as np
from watermarking import WatermarkingSystem

class RobustnessTester:
    def __init__(self, watermarking_system: WatermarkingSystem):
        self.watermarking_system = watermarking_system

    # 旋转测试        
    def test_rotation(self, watermarked_image: np.ndarray, angle: float) -> float:
        height, width = watermarked_image.shape[:2]
        center = (width // 2, height // 2)
        rotation_matrix = cv2.getRotationMatrix2D(center, angle, 1.0)
        rotated_image = cv2.warpAffine(watermarked_image, rotation_matrix, (width, height), flags=cv2.INTER_CUBIC, borderMode=cv2.BORDER_REPLICATE)
        extracted_watermark = self.watermarking_system.extract(rotated_image)
        ncc = self.watermarking_system.calculate_ncc(self.watermarking_system.watermark, extracted_watermark)
        return ncc

    # 图像缩放
    def test_scaling(self, watermarked_image: np.ndarray, scale: float) -> float:
        scaled_image = cv2.resize(watermarked_image, None, fx=scale, fy=scale)
        scaled_image = cv2.resize(scaled_image, (watermarked_image.shape[1], watermarked_image.shape[0]))
        extracted_watermark = self.watermarking_system.extract(scaled_image)
        ncc = self.watermarking_system.calculate_ncc(self.watermarking_system.watermark, extracted_watermark)
        return ncc
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
    
    # 图像裁剪函数
    def test_cropping(self, watermarked_image: np.ndarray, crop_ratio: float) -> float:
        height, width = watermarked_image.shape[:2]
        crop_size = int(min(height, width) * crop_ratio)
        start_x = (width - crop_size) // 2
        start_y = (height - crop_size) // 2
        cropped_image = watermarked_image[start_y:start_y+crop_size, start_x:start_x+crop_size]
        extracted_watermark = self.watermarking_system.extract(cropped_image)
        ncc = self.watermarking_system.calculate_ncc(self.watermarking_system.watermark, extracted_watermark)
        return ncc   
    
    # 图像亮度
    def test_brightness(self, watermarked_image: np.ndarray, value: int) -> float:
        hsv = cv2.cvtColor(watermarked_image, cv2.COLOR_BGR2HSV)
        h, s, v = cv2.split(hsv)
        v = np.clip(v + value, 0, 255).astype(hsv.dtype)
        hsv = cv2.merge([h, s, v])
        brightened_image = cv2.cvtColor(hsv, cv2.COLOR_HSV2BGR)
        extracted_watermark = self.watermarking_system.extract(brightened_image)
        ncc = self.watermarking_system.calculate_ncc(self.watermarking_system.watermark, extracted_watermark)
        return ncc
    
     # 图像对比度
    def test_contrast(self, watermarked_image: np.ndarray, alpha: float) -> float:
        contrasted_image = cv2.convertScaleAbs(watermarked_image, alpha=alpha, beta=0)
        extracted_watermark = self.watermarking_system.extract(contrasted_image)
        ncc = self.watermarking_system.calculate_ncc(self.watermarking_system.watermark, extracted_watermark)
        return ncc
    
    # 图像噪声
    def test_noise(self, watermarked_image: np.ndarray, noise_level: float) -> float:
        row, col, ch = watermarked_image.shape
        mean = 0
        var = noise_level
        sigma = var ** 0.5
        gauss = np.random.normal(mean, sigma, (row, col, ch))
        gauss = gauss.reshape(row, col, ch)
        noisy_image = watermarked_image + gauss
        noisy_image = np.clip(noisy_image, 0, 255).astype(np.uint8)
        extracted_watermark = self.watermarking_system.extract(noisy_image)
        ncc = self.watermarking_system.calculate_ncc(self.watermarking_system.watermark, extracted_watermark)
        return ncc
        
    # JPEG压缩测试
    def test_jpeg_compression(self, watermarked_image: np.ndarray, quality: int) -> float:
        encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), quality]
        result, encimg = cv2.imencode('.jpg', watermarked_image, encode_param)
        decimg = cv2.imdecode(encimg, 1)
        extracted_watermark = self.watermarking_system.extract(decimg)
        ncc = self.watermarking_system.calculate_ncc(self.watermarking_system.watermark, extracted_watermark)
        return ncc
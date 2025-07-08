import cv2
import numpy as np
from watermarking import WatermarkingSystem

class RobustnessTester:
    def __init__(self, watermarking_system: WatermarkingSystem):
        """初始化鲁棒性测试器"""
        self.watermarking_system = watermarking_system
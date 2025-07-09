import cv2
import numpy as np
import matplotlib.pyplot as plt
from watermarking import WatermarkingSystem
from robustness_tests import RobustnessTester
import os
import traceback


# 设置matplotlib中文字体
plt.rcParams["font.family"] = ["SimHei", ]
plt.rcParams['axes.unicode_minus'] = False  

def main():
    try:
        # 初始化水印系统 - 指定自定义水印图像路径
        custom_watermark_path = 'custom_waterMark06.png'  
        watermarking = WatermarkingSystem(alpha=0.5, watermark_image_path=custom_watermark_path)
        
        # 嵌入水印
        original_image_path = 'Original.png'
        if not os.path.exists(original_image_path):
            raise FileNotFoundError(f"原始图像文件 {original_image_path} 不存在")
            
        watermarked_image_path = 'WaterMark.png'
        watermarked_image = watermarking.embed(original_image_path, watermarked_image_path)
        
        # 保存原始水印模式
        original_watermark = watermarking.watermark
        
        original_watermark_vis = np.uint8((original_watermark + 1) * 127.5)
        cv2.imwrite('original_watermark.png', original_watermark_vis)
        
        # 提取水印
        extracted_watermark = watermarking.extract(watermarked_image)
        ncc_original = watermarking.calculate_ncc(original_watermark, extracted_watermark)
        print(f"原始图像水印提取NCC: {ncc_original:.4f}")
        
        # 保存提取的水印
        extracted_watermark_vis = np.uint8((extracted_watermark + 1) * 127.5)
        cv2.imwrite('extracted_watermark.png', extracted_watermark_vis)
           
    except Exception as e:
        print(f"程序执行出错: {str(e)}")
        traceback.print_exc()

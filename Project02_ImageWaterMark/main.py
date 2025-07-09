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
           

        # 初始化鲁棒性测试器
        tester = RobustnessTester(watermarking)
        
        # 测试各种攻击
        attacks = {
            "Rotate 30 degrees": lambda img: tester.test_rotation(img, 30),
            "Scale by 0.8 times": lambda img: tester.test_scaling(img, 0.8),
            "Crop by 20%": lambda img: tester.test_cropping(img, 0.8),
            "Brightness +50": lambda img: tester.test_brightness(img, 50),
            "Contrast 1.5": lambda img: tester.test_contrast(img, 1.5),
            "Gaussian noise(a=20)": lambda img: tester.test_noise(img, 20),
            "JPEG compression(Q=70)": lambda img: tester.test_jpeg_compression(img, 70),
        }

        
        # 执行测试并显示结果
        results = {}
        for attack_name, attack_func in attacks.items():
            try:
                ncc = attack_func(watermarked_image)
                results[attack_name] = ncc
                print(f"After {attack_name}, NCC: {ncc:.4f}")
                
                # 保存攻击后的图像
                attacked_image = np.copy(watermarked_image)
                if attack_name == "Rotate 30 degrees":
                    height, width = attacked_image.shape[:2]
                    center = (width // 2, height // 2)
                    rotation_matrix = cv2.getRotationMatrix2D(center, 30, 1.0)
                    attacked_image = cv2.warpAffine(
                        attacked_image, rotation_matrix, (width, height),
                        flags=cv2.INTER_CUBIC, borderMode=cv2.BORDER_REPLICATE
                    )
                elif attack_name == "Scale by 0.8 times":
                    attacked_image = cv2.resize(attacked_image, None, fx=0.8, fy=0.8)
                    attacked_image = cv2.resize(attacked_image, (watermarked_image.shape[1], watermarked_image.shape[0]))
                elif attack_name == "Crop by 20%": 
                    height, width = attacked_image.shape[:2]
                    crop_size = int(min(height, width) * 0.8)
                    start_x = (width - crop_size) // 2
                    start_y = (height - crop_size) // 2
                    attacked_image = attacked_image[start_y:start_y+crop_size, start_x:start_x+crop_size]
                    attacked_image = cv2.resize(attacked_image, (width, height))
                elif attack_name =="Brightness +50": 
                    hsv = cv2.cvtColor(attacked_image, cv2.COLOR_BGR2HSV)
                    h, s, v = cv2.split(hsv)
                    v = np.clip(v + 50, 0, 255).astype(hsv.dtype)
                    attacked_image = cv2.cvtColor(cv2.merge([h, s, v]), cv2.COLOR_HSV2BGR)
                elif attack_name == "Contrast 1.5": 
                    attacked_image = cv2.convertScaleAbs(attacked_image, alpha=1.5, beta=0)
                elif attack_name == "Gaussian noise(a=20)": 
                    row, col, ch = attacked_image.shape
                    gauss = np.random.normal(0, 20, (row, col, ch))
                    attacked_image = np.clip(attacked_image + gauss, 0, 255).astype(np.uint8)
                elif attack_name == "JPEG compression(Q=70)":
                    encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), 70]
                    _, encimg = cv2.imencode('.jpg', attacked_image, encode_param)
                    attacked_image = cv2.imdecode(encimg, 1)
                
                cv2.imwrite(f'attacked_{attack_name.replace(" ", "_")}.png', attacked_image)
                
                # 提取并保存攻击后的水印
                extracted_after_attack = watermarking.extract(attacked_image)
                extracted_after_attack_vis = np.uint8((extracted_after_attack + 1) * 127.5)
                cv2.imwrite(f'extracted_after_{attack_name.replace(" ", "_")}.png', extracted_after_attack_vis)
                
            except Exception as e:
                print(f"执行 {attack_name} 测试时出错: {str(e)}")
                results[attack_name] = 0
        
        # 可视化测试结果
        plt.figure(figsize=(10, 6))
        plt.bar(results.keys(), results.values())
        plt.ylim(0, 1.1)
        plt.title('水印鲁棒性测试结果')
        plt.xlabel('攻击类型')
        plt.ylabel('归一化相关系数(NCC)')
        plt.xticks(rotation=45, ha='right')
        plt.tight_layout()
        plt.savefig('robustness_test_results.png')
        # plt.show()
        
        # 可视化原始图像、水印图像和提取的水印
        plt.figure(figsize=(15, 5))
        
        # 原始图像
        plt.subplot(131)
        original_image = cv2.imread(original_image_path)
        plt.imshow(cv2.cvtColor(original_image, cv2.COLOR_BGR2RGB))
        plt.title('原始图像')
        plt.axis('off')
        
        # 水印图像
        plt.subplot(132)
        plt.imshow(cv2.cvtColor(watermarked_image, cv2.COLOR_BGR2RGB))
        plt.title('水印图像')
        plt.axis('off')
        
        # 提取的水印
        plt.subplot(133)
        plt.imshow(extracted_watermark, cmap='gray')
        plt.title(f'提取的水印 (NCC: {ncc_original:.4f})')
        plt.axis('off')
        
        plt.tight_layout()
        plt.savefig('watermarking_demo.png')
        # plt.show()
        

    except Exception as e:
        print(f"程序执行出错: {str(e)}")
        traceback.print_exc()

if __name__ == "__main__":
    main()
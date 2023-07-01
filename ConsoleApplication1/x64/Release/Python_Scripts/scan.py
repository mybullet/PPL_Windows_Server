import datetime
import os
import sys
import traceback
from enum import Enum
import numpy as np
import cv2
import pyocr
from PIL import Image

def orderPoints(pts):
	# 一共4个坐标点
	rect = np.zeros((4, 2), dtype="float32")

	# 按顺序找到对应坐标0123分别是 左上，右上，右下，左下
	# 计算左上，右下
	s = pts.sum(axis = 1) # 对横纵坐标求和（沿着指定轴计算第N维的总和）
	rect[0] = pts[np.argmin(s)] # 最小的是左上角的点
	rect[2] = pts[np.argmax(s)] # 最大的是右下角的点

	# 计算右上和左下
	diff = np.diff(pts, axis = 1) # 沿着指定轴计算第N维的离散差值
	rect[1] = pts[np.argmin(diff)]
	rect[3] = pts[np.argmax(diff)]

	return rect

def fourPointTransform(image, pts):
	# 获取输入坐标点
	rect = orderPoints(pts)
	(tl, tr, br, bl) = rect

	# 计算输入的w和h值
	width_a = np.sqrt(((br[0] - bl[0]) ** 2) + ((br[1] - bl[1]) ** 2))
	width_b = np.sqrt(((tr[0] - tl[0]) ** 2) + ((tr[1] - tl[1]) ** 2))
	max_width = max(int(width_a), int(width_b))

	height_a = np.sqrt(((tr[0] - br[0]) ** 2) + ((tr[1] - br[1]) ** 2))
	height_b = np.sqrt(((tl[0] - bl[0]) ** 2) + ((tl[1] - bl[1]) ** 2))
	max_height = max(int(height_a), int(height_b))

	# 变换后对应坐标位置
	dst = np.array([
		[0, 0],
		[max_width - 1, 0],  # 防止出错，-1
		[max_width - 1, max_height - 1],
		[0, max_height - 1]], dtype = "float32")

	# 计算变换矩阵
	M = cv2.getPerspectiveTransform(rect, dst)
	warped = cv2.warpPerspective(image, M, (max_width, max_height))

	# 返回变换后结果
	return warped

def resize(image, width=None, height=None, inter=cv2.INTER_AREA):
	dim = None
	(h, w) = image.shape[:2]
	if width is None and height is None:
		return image
	if width is None:
		r = height / float(h)
		dim = (int(w * r), height)
	else:
		r = width / float(w)
		dim = (width, int(h * r))
	'''
	cv2.resize(image, dim, interpolation=inter)
		image: 输入图像
		dim: (width, height),目标图象的维度
		interpolation: 插值方法
	'''
	resized = cv2.resize(image, dim, interpolation=inter)
	return resized

class Language(Enum):
    """
    enum variable for language type
    """
    chi_all = 0  # 这是识别模型的名字
    chi_sim = 1
    chi_tra = 2
    eng = 3
    osd = 4

def setLangForTesseract(lang_type):
    ocr_tools = pyocr.get_available_tools()

    for tool in ocr_tools:
        if tool.get_name() == "Tesseract (sh)":
            used_ocr_tool = tool
            break

    langs = used_ocr_tool.get_available_languages()
    # print("Tesseract Supports languages: {}".format(", ".join(langs)))

    for lang in langs:
        if lang == lang_type.name:
            used_lang = lang
            break

    return used_ocr_tool,used_lang

def recognizeImageAndOutput(final_image_name, lang_type, output_text_file_name):
    tesseract_ocr_tool, tesseract_ocr_lang = setLangForTesseract(lang_type)

    txt = tesseract_ocr_tool.image_to_string(
        Image.open(final_image_name),
        lang=tesseract_ocr_lang,
        builder=pyocr.builders.TextBuilder()
    )

    with open(output_text_file_name, 'w', encoding='utf-8') as fObject:
        pyocr.builders.TextBuilder().write_file(fObject, txt)

def createText(name, msg):
    full_path = './ocr_logs/' + name + '.txt'
    file = open(full_path, 'w')
    file.write(msg)
    file.close()
'''
1. 从./images传入的图片，必须先保证名字的唯一性
2. 若返回非-1，则执行成功，可去./ocr_results文件夹下，根据返回的图片名字，检索结果文本
3. 若返回-1，则执行失败，可去./ocr_logs文件夹下，根据传入的图片名字，检索log日志
'''
def convertImage2String(image_name):
	try:
		# 预先定义文本或者图片的名字
		output_name = datetime.datetime.now().strftime('%Y%m%d_%H%M%S_') + image_name
		# 创建文件夹
		if not os.path.exists('./Images'):
			os.mkdir('./Images')
		if not os.path.exists('./Ocr_Preprocess'):
			os.mkdir('./Ocr_Preprocess')
		if not os.path.exists('./Ocr_Results'):
			os.mkdir('./Ocr_Results')
		if not os.path.exists('./Ocr_Logs'):
			os.mkdir('./Ocr_Logs')

		'''
		cv2.imread(filename, flags)
			filename: 读入image的完整路径
			flags: 标志位，{cv2.IMREAD_COLOR，cv2.IMREAD_GRAYSCALE，cv2.IMREAD_UNCHANGED}, 默认值为IMREAD_COLOR
		:return: BGR格式的彩色图象数据
		'''
		image = cv2.imread('./Images/' + image_name)

		'''
		shape的值依次为height, width, dimension;
		为加快计算速度，降低图象的分辨率，ratio为缩放比例
		'''
		ratio = image.shape[0] / 500.0

		# 原图，用于后续透视变换
		orig = image.copy()

		# 将图像基于高为500进行缩放
		image = resize(image, height=500)

		# 预处理
		gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY) # 图像处理前，先转换为灰度图像
		gray = cv2.GaussianBlur(gray, (5, 5), 0)
		edged = cv2.Canny(gray, 75, 200) # 边缘检测

		# 轮廓检测
		contours, hierarchy = cv2.findContours(edged.copy(), cv2.RETR_LIST, cv2.CHAIN_APPROX_SIMPLE) # 轮廓提取
		contours = sorted(contours, key = cv2.contourArea, reverse = True)[:1] # 基于轮廓面积以降序排序，取一个

		# 遍历轮廓
		for c in contours:
			peri = cv2.arcLength(c, True) # 计算轮廓的长度，True对应闭合
			# C表示输入的点集
			# epsilon表示从原始轮廓到近似轮廓的最大距离，它是一个准确度参数
			# True表示封闭的
			approx = cv2.approxPolyDP(c, 0.02 * peri, True) # 计算轮廓近似

			# 近似轮廓得到4个点,意味着可能得到的是矩形，四个（width, height）
			if len(approx) == 4:
				screen_contour = approx
				break

		# 透视变换
		warped = fourPointTransform(orig, screen_contour.reshape(4, 2) * ratio)

		# 图片预处理
		warped = cv2.cvtColor(warped, cv2.COLOR_BGR2GRAY)
		preprocess = 'thresh'
		if preprocess == "blur":
			final = cv2.medianBlur(warped, 3)

		if preprocess == "thresh":
			final = cv2.threshold(warped, 0, 255, cv2.THRESH_BINARY | cv2.THRESH_OTSU)[1]

		file_name = "{}.png".format("./Ocr_Preprocess/" + output_name)
		cv2.imwrite(file_name, final) # ocr需要numpy数据，所以需要转存
		recognizeImageAndOutput(file_name, Language.chi_all,  "./Ocr_Results/{}.txt".format(output_name)) # ocr
		return output_name
	except:
		createText(output_name, traceback.format_exc())
		return -1

"""
License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
"""
import sys
import math
import cv2
import rsid_py

PORT="COM9"

def resize_if_big(im_cv):
    """
    Max allowed buffer to enroll is 900kb, resize to smaller size if needed
    """
    MAX_IMG_SIZE = 900 * 1024
    height, width, channels = im_cv.shape
    img_size = width * height * channels

    if channels != 3:
        print("image must be bgr24")
        sys.exit(1)

    if img_size > MAX_IMG_SIZE:
        scale = math.sqrt(MAX_IMG_SIZE / img_size)
        im_cv = cv2.resize(im_cv, (0, 0), fx=scale, fy=scale)
        height, width, channels = im_cv.shape
        img_size_kb = int(width * height * channels/1024)
        print(f"Scaled down to {width}x{height} ({img_size_kb} KB) to fit max size")
    return im_cv

def enroll_with_image(user_id, filename):
    im_cv = cv2.imread(filename)
    im_cv = resize_if_big(im_cv)
    height, width, channels = im_cv.shape
    with rsid_py.FaceAuthenticator(PORT) as f:
        result = f.enroll_image(user_id, im_cv.flatten(), width, height)
        print(result)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python enroll_image.py <user-id> <image-filename>")
        sys.exit(1)
    enroll_with_image(sys.argv[1], sys.argv[2])
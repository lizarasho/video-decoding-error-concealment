import re
import subprocess

import numpy as np


def parse_resolution(stderr: str):
    return tuple(map(int, re.findall('(\d+)x(\d+)', stderr)[-1]))


def parse_frames(stderr: str):
    return int(re.findall('frame=\s+(\d+)', stderr)[-1])


def convert_to_yuv(input_file_name, output_file_name):
    command = f'ffmpeg -y -i {input_file_name} -c:v rawvideo -pix_fmt yuv420p {output_file_name}'
    process = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    resolution = parse_resolution(str(process.stderr))
    frames = parse_frames(str(process.stderr))
    return resolution, frames


class VideoCaptureYUV:
    def __init__(self, input_file_name, width, height):
        self.width = width
        self.height = height
        self.frame_length = self.width * self.height * 3 // 2
        self.shape = (int(self.height * 3 // 2), self.width)
        self.input_file = open(input_file_name, 'rb')

    def read_raw(self):
        raw = self.input_file.read(self.frame_length)
        yuv = np.frombuffer(raw, dtype=np.uint8)
        if len(yuv) != self.frame_length:
            return False, None
        return True, yuv

    def skip_frames(self, n):
        self.input_file.read(self.frame_length * n)

    def read(self):
        ret, yuv = self.read_raw()
        return ret, yuv
        # if not ret:
        #     return ret, yuv
        # bgr = cv2.cvtColor(yuv, cv2.COLOR_YUV2BGR_IYUV)
        # while True:
        #     cv2.imshow('BGR Image', bgr)
        #     if cv2.waitKey(20) & 0xFF == 27:
        #         break
        # return ret, bgr

    def read_frames(self, n):
        yuv_frames = []
        for current_frame in range(0, n):
            ret, yuv_frame = self.read()
            if ret:
                yuv_frames.append(yuv_frame)
            else:
                raise Exception(f'Unable to read all frames of the video')
        return yuv_frames


COMPONENTS = 3


def get_component_shift(component, frame_width, frame_height):
    if component == 0:
        # Y
        return 0
    if component == 1:
        # U
        return frame_width * frame_height
    if component == 2:
        # V
        return frame_width * frame_height * 5 // 4


def copy_yuv_unit(yuv_from, yuv_to, x_left, y_left, x_right, y_right, frame_width, frame_height):
    result = np.copy(yuv_to)
    full_block_width = x_right - x_left
    full_block_height = y_right - y_left
    for comp in range(0, COMPONENTS):
        comp_block_h = full_block_height if comp == 0 else full_block_height // 2
        comp_block_w = full_block_width if comp == 0 else full_block_width // 2
        comp_stride = frame_width if comp == 0 else frame_width // 2
        comp_y_left = y_left if comp == 0 else y_left // 2
        comp_x_left = x_left if comp == 0 else x_left // 2
        comp_shift = get_component_shift(comp, frame_width, frame_height)
        for y in range(comp_y_left, comp_y_left + comp_block_h):
            for x in range(comp_x_left, comp_x_left + comp_block_w):
                result[comp_shift + comp_stride * y + x] = yuv_from[comp_shift + comp_stride * y + x]
    return result


def write_yuv_file(output_file_name, yuv_frames):
    with open(output_file_name, 'wb') as f:
        for yuv_frame in yuv_frames:
            f.write(np.array(yuv_frame).tobytes())

#
# if __name__ == "__main__":
#     # convert_to_yuv("media_xiph_data/raw/bowing_cif.y4m", "media_xiph_data/raw/bowing_cif.yuv")
#     # file_name = "media_xiph_data/raw/bowing_cif.yuv"
#     # size = (HEIGHT, WIDTH)
#     # cap = VideoCaptureYUV(file_name, WIDTH, HEIGHT)
#     # # frames_number = 0
#     # # while True:
#     # #     ret, yuv_frame = cap.read()
#     # #     if ret:
#     # #         yuv_frame = yuv_frame.reshape(cap.shape)
#     # #         bgr_frame = cv2.cvtColor(yuv_frame, cv2.COLOR_YUV2BGR_IYUV)
#     # #         cv2.imshow("frame", bgr_frame)
#     # #         cv2.waitKey(30)
#     # #         frames_number += 1
#     # #     else:
#     # #         break
#     print(frames_number)
#     os.remove(file_name)

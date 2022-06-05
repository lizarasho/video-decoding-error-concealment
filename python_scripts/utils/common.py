import os


def calc_bytes_size(clip_path):
    return os.path.getsize(clip_path)


def calc_frame_size(width, height):
    return width * height * 3 // 2


def calc_frame_number(yuv_path, width, height):
    bytes_size = calc_bytes_size(yuv_path)
    frame_size = calc_frame_size(width, height)
    return bytes_size // frame_size

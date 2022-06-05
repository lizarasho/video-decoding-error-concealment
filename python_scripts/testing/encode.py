import os

from testing.settings.common import RESOLUTION, T_APP_ENCODER
from utils.common import calc_frame_number

DEFAULT_MODE = False

if DEFAULT_MODE:
    from testing.settings.default import ORIGINAL_STREAMS, ORIGINAL_YUVS

    CLIPS = [
        'flower_cif',
        'pamphlet_cif',
        'Johnny_hd',
        'KristenAndSara_hd',
        'grandma_qcif',
        'life_hd',
        'pedestrian_area_hd',
        'suzie_qcif',
        'touchdown_pass_hd',
        'tractor_hd'
    ]
else:
    from testing.settings.lose_packages import ORIGINAL_STREAMS, ORIGINAL_YUVS

    CLIPS = [
        # 'tractor_640x384',
        'touchdown_pass_640x384',
        # 'shields_rotated_504x504',
    ]

ENCODER_CONDIGS = 'TAppEncoder_configs'
SAMPLE_CONFIG = f'{ENCODER_CONDIGS}/encoder_lowdelay_P_main_V.cfg'

if not os.path.exists(ORIGINAL_STREAMS):
    os.makedirs(ORIGINAL_STREAMS)


def set_param(current_config_lines, param_name, param_value):
    out_config_lines = current_config_lines
    for i in range(len(out_config_lines)):
        config_line = out_config_lines[i]
        if config_line.startswith(param_name):
            default_param_value = config_line.split()[2]
            out_config_lines[i] = config_line.replace(default_param_value, param_value)
    return out_config_lines


def get_clip_config(clip_name, width, height):
    folder = f'{ENCODER_CONDIGS}/{width}x{height}'
    if not os.path.exists(folder):
        os.makedirs(folder)
    return f'{folder}/{clip_name}.cfg'


def generate_configs():
    with open(SAMPLE_CONFIG, 'r') as sample_config_file:
        sample_config_lines = sample_config_file.readlines()

    for clip_name in CLIPS:
        input_file_name = f'{ORIGINAL_YUVS}/{clip_name}.yuv'
        output_file_name = f'{ORIGINAL_STREAMS}/{clip_name}.h265'
        width = RESOLUTION[clip_name][0]
        height = RESOLUTION[clip_name][1]
        frames_number = calc_frame_number(input_file_name, width, height)

        params = [
            ['InputFile', input_file_name],
            ['BitstreamFile', output_file_name],
            ['FramesToBeEncoded', str(frames_number)],
            ['SourceWidth', str(width)],
            ['SourceHeight', str(height)]
        ]

        out_config_lines = sample_config_lines
        for param_name, param_value in params:
            out_config_lines = set_param(out_config_lines, param_name, param_value)
        out_config = ''.join(out_config_lines)
        print(out_config)

        with open(get_clip_config(clip_name, width, height), 'w') as out_config_file:
            out_config_file.write(out_config)


def encode():
    for clip_name in CLIPS:
        print(f'Started encoding clip {clip_name}...')
        width = RESOLUTION[clip_name][0]
        height = RESOLUTION[clip_name][1]
        command = f'{T_APP_ENCODER} -c {get_clip_config(clip_name, width, height)}'
        os.system(f'{command}')


if __name__ == '__main__':
    # generate_configs()
    encode()

import os

from testing.settings.common import NAL_INFO

DEFAULT_MODE = False

if DEFAULT_MODE:
    from testing.settings.default import ORIGINAL_STREAMS, NAL_UNITS_DESC

    CLIPS = [
        'Johnny_hd',
        'KristenAndSara_hd',
        'flower_cif',
        'grandma_qcif',
        'life_hd',
        'pamphlet_cif',
        'pedestrian_area_hd',
        'suzie_qcif',
        'touchdown_pass_hd',
        'tractor_hd'
    ]
else:
    from testing.settings.lose_packages import ORIGINAL_STREAMS, NAL_UNITS_DESC

    CLIPS = [
        # 'tractor_640x384_QP_28',
        # 'tractor_640x384_QP_30',
        # 'tractor_640x384_QP_32',
        # 'touchdown_pass_640x384_QP_28',
        # 'touchdown_pass_640x384_QP_30',
        # 'touchdown_pass_640x384_QP_32',
        # 'touchdown_pass_640x384_QP_33',
        # 'touchdown_pass_640x384_QP_35',
        # 'touchdown_pass_640x384_QP_37',
        'touchdown_pass_640x384_QP_38',
        'touchdown_pass_640x384_QP_40',
        'touchdown_pass_640x384_QP_42',
        # 'shields_rotated_504x504_QP_28',
        # 'shields_rotated_504x504_QP_30',
        # 'shields_rotated_504x504_QP_32',
    ]


def run_NAL_info():
    for clip_name in CLIPS:
        stream_path = f'{ORIGINAL_STREAMS}/{clip_name}.h265'
        nal_info = f'{NAL_UNITS_DESC}/original/{clip_name}.txt'
        command = f'{NAL_INFO} {stream_path} {nal_info}'
        print(command)
        os.system(command)


if __name__ == '__main__':
    run_NAL_info()

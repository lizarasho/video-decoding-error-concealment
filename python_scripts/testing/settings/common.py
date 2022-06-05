DIPLOMA_FOLDER = '/Users/lizarasho/itmo/diploma'

ADDITIONAL_INFO = f'{DIPLOMA_FOLDER}/additional_info'

T_APP_ENCODER = f'{DIPLOMA_FOLDER}/HM/bin/umake/clang-12.0/x86_64/debug/TAppEncoder'
T_APP_DECODER = f'{DIPLOMA_FOLDER}/source_full/bin/umake/clang-12.0/x86_64/debug/TAppDecoder'
LOSE_BITS = f'{DIPLOMA_FOLDER}/source_full/bin/umake/clang-12.0/x86_64/debug/LoseBits'
NAL_INFO = f'{DIPLOMA_FOLDER}/source_full/bin/umake/clang-12.0/x86_64/debug/NALinfo'

ERROR_ALGO_PARAMS = {
    2: '',
    3: '',
    4: f'-m {ADDITIONAL_INFO}/no-motion.csv',
    5: '',
    6: ''
}

RESOLUTION = {
    'Johnny_hd': (160, 128),
    'KristenAndSara_hd': (160, 128),
    'flower_cif': (160, 128),
    'grandma_qcif': (160, 128),
    'life_hd': (160, 128),
    'pamphlet_cif': (160, 128),
    'pedestrian_area_hd': (160, 128),
    'suzie_qcif': (160, 128),
    'touchdown_pass_hd': (160, 128),
    'tractor_hd': (160, 128),
    'flower_cif_6_reversed': (160, 128),
    'suzie_qcif_6_reversed': (160, 128),
    'tractor_hd_6_reversed': (160, 128),
    'tractor_640x384': (640, 384),
    'tractor_640x384_QP_28': (640, 384),
    'tractor_640x384_QP_30': (640, 384),
    'tractor_640x384_QP_32': (640, 384),
    'touchdown_pass_640x384': (640, 384),
    'touchdown_pass_640x384_QP_28': (640, 384),
    'touchdown_pass_640x384_QP_30': (640, 384),
    'touchdown_pass_640x384_QP_32': (640, 384),
    'shields_rotated_504x504': (504, 504),
    'shields_rotated_504x504_QP_28': (504, 504),
    'shields_rotated_504x504_QP_30': (504, 504),
    'shields_rotated_504x504_QP_32': (504, 504),
}

DISPLAY_NAMES = {
    'Johnny_hd': 'Johnny',
    'KristenAndSara_hd': 'KristenAndSara',
    'flower_cif': 'flower',
    'grandma_qcif': 'grandma',
    'life_hd': 'life',
    'pamphlet_cif': 'pamphlet',
    'pedestrian_area_hd': 'pedestrian_area',
    'suzie_qcif': 'suzie',
    'touchdown_pass_hd': 'touchdown_pass',
    'tractor_hd': 'tractor'
}

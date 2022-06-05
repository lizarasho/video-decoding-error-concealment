import os

from testing.settings.common import LOSE_BITS
from testing.settings.default import DAMAGED_STREAMS, ORIGINAL_STREAMS, NAL_UNITS_DESC

DIPLOMA_FOLDER = '/Users/lizarasho/itmo/diploma'

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
PROBS = [0.02, 0.04, 0.06, 0.08, 0.10, 0.12, 0.14, 0.16, 0.18, 0.20]


def damage_streams():
    for prob in PROBS:
        damaged_streams_path = f'{DAMAGED_STREAMS}/{prob}'
        if not os.path.exists(damaged_streams_path):
            os.makedirs(damaged_streams_path)
        for clip_name in CLIPS:
            original_stream = f'{ORIGINAL_STREAMS}/{clip_name}.h265'
            damaged_stream = f'{damaged_streams_path}/{clip_name}.h265'
            nal_units_desc = f'{NAL_UNITS_DESC}/{prob}/{clip_name}.txt'
            command = f'{LOSE_BITS} {original_stream} {nal_units_desc} {damaged_stream}'
            os.system(command)


if __name__ == '__main__':
    damage_streams()
